/**
* Copyright (C) 2013 kangliqiang ,kangliq@163.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "RebalancePushImpl.h"

#include <string.h>
#include <limits.h>

#include "DefaultMQPushConsumerImpl.h"
#include "AllocateMessageQueueStrategy.h"
#include "MQClientFactory.h"
#include "MessageQueueListener.h"
#include "OffsetStore.h"
#include "DefaultMQPushConsumer.h"
#include "MQAdminImpl.h"
#include "MQClientException.h"
#include "DateInterface.h"

RebalancePushImpl::RebalancePushImpl(DefaultMQPushConsumerImpl* pDefaultMQPushConsumerImpl)
	:RebalanceImpl("",BROADCASTING,NULL,NULL),
	m_pDefaultMQPushConsumerImpl(pDefaultMQPushConsumerImpl)
{
}

RebalancePushImpl::RebalancePushImpl(const std::string& consumerGroup,
	MessageModel messageModel,
	AllocateMessageQueueStrategy* pAllocateMessageQueueStrategy,
	MQClientFactory* pMQClientFactory,
	DefaultMQPushConsumerImpl* pDefaultMQPushConsumerImpl)
	:RebalanceImpl(consumerGroup, messageModel, pAllocateMessageQueueStrategy, pMQClientFactory),
	m_pDefaultMQPushConsumerImpl(pDefaultMQPushConsumerImpl)
{
}

void RebalancePushImpl::dispatchPullRequest(std::list<PullRequest*>& pullRequestList)
{
	std::list<PullRequest*>::iterator it = pullRequestList.begin();
	// 派发PullRequest
	for (;it!=pullRequestList.end();it++)
	{
		m_pDefaultMQPushConsumerImpl->executePullRequestImmediately(*(*it));
	}
}

long long RebalancePushImpl::computePullFromWhere(MessageQueue& mq)
{
	long long result = -1;
	ConsumeFromWhere consumeFromWhere =
		m_pDefaultMQPushConsumerImpl->getDefaultMQPushConsumer()->getConsumeFromWhere();
	OffsetStore* offsetStore = m_pDefaultMQPushConsumerImpl->getOffsetStore();

	switch (consumeFromWhere)
	{
	case CONSUME_FROM_LAST_OFFSET:
		{
			long long lastOffset = offsetStore->readOffset(mq, READ_FROM_STORE);
			if (lastOffset >= 0)
			{
				result = lastOffset;
			}
			// 当前订阅组在服务器没有对应的Offset
			// 说明是第一次启动
			else if (-1 == lastOffset)
			{
				// 如果是重试队列，需要从0开始
                if (mq.getTopic().find(MixAll::RETRY_GROUP_TOPIC_PREFIX) == 0 )
				{
					result = 0L;
				}
				// 正常队列则从末尾开始
				else
				{
					/**Note by lin.qiongshan, 2016-11-10, 为什么要这样修改
						首先要清楚，这里计算完消费开始位置后，会按该位置向服务端请求拉取指定位置的消息
						其次，服务端会定期清除过期的消息，因此一个队列中早期的消息可能会被删除，此时该队列的 minOffset 将大于0，即删除消息不会重置 offset

						如果队列中前面的消息被删除过，根据目前的使用情况，consumer_group 首次注册，在请求 offset（code 14）时，服务端返回错误码22（见在 MQClientAPIImpl::queryConsumerOffset 注释中的说明）
						此时，RemoteBrokerOffsetStore::readOffset 会发回 -1 走到此处

						按 CONSUME_FROM_LAST_OFFSET 要求应该从队列尾开始消费，之前是直接发送 LLONG_MAX，拉取该位置的消息，而该位置不存在消息，服务端会返回错误：（错误码 21）pull request offset illegal
							此时获取 offset 失败，会导致无法拉取消息
							服务端返回错误，同时携带有队列的 offset 数据，如果此时没有新消息进入，nextBeginOffset 指示的位置是最后一条消息的下一个位置
								{"code":21,"extFields":{"maxOffset":"6510","minOffset":"3672","nextBeginOffset":"6510","suggestWhichBroker        Id":"0"},"flag":1,"language":"JAVA","opaque":136,"remark":"OFFSET_OVERFLOW_BADLY","version":61}
							虽然拉取消息失败，但在消息拉取服务函数（DefaultMQPushConsumerImplCallback::onSuccess）中，会报错返回的信息中的 nextBeginOffset ，并等待10s后更新会 Broker
								{"code":15,"language":"CPP","version":19,"opaque":139,"flag":0,"remark":"","extFields":{"consumerGrou        p":"g_1005","topic":"topic_1","queueId":"0","commitOffset":"6614"}}
							offset 更新成功后，再拉取消息，就可以获取正确的 offset 值。此时，如果有新消息进入，就可以继续消费消息了

						修改后，当 QUERY_CONSUMER_OFFSET_VALUE（code 14） 请求消费失败 从服务端请求队列的 maxOffset（最后一条消息的下一个位置）（code 30）作为消费起始位置
					*/

					/* modified by yu.guangjie at 2016-11-10, reason: get max Offset */
					// result = LLONG_MAX;
					try
					{
						result = m_pMQClientFactory->getMQAdminImpl()->maxOffset(mq);
					}
					catch (MQClientException& e)
					{
						result = -1;
					}
				}
			}
			// 发生其他错误
			else
			{
				result = -1;
			}
			break;
		}
    /* modified by yu.guangjie at 2015-08-17, reason: add FIRST & TIMESTAMP*/
	case CONSUME_FROM_FIRST_OFFSET: 
        {
            long lastOffset = offsetStore->readOffset(mq, READ_FROM_STORE);
            // 第二次启动，根据上次的消费位点开始消费
            if (lastOffset >= 0) 
            {
                result = lastOffset;
            }
            // 第一次启动，没有记录消费位点
            else if (-1 == lastOffset) 
            {
                result = 0L;
            }
            // 其他错误
            else 
            {
                result = -1;
            }
            break;
        }
    case CONSUME_FROM_TIMESTAMP: 
        {
            long lastOffset = offsetStore->readOffset(mq, READ_FROM_STORE);
            // 第二次启动，根据上次的消费位点开始消费
            if (lastOffset >= 0) 
            {
                result = lastOffset;
            }
            // 第一次启动，没有记录消费为点
            else if (-1 == lastOffset) 
            {
                // 重试队列则从队列尾部开始
                if (mq.getTopic().find(MixAll::RETRY_GROUP_TOPIC_PREFIX) == 0 )
                {
                    try 
                    {
                        result = m_pMQClientFactory->getMQAdminImpl()->maxOffset(mq);
                    }
                    catch (MQClientException& e) 
                    {
                        result = -1;
                    }
                }
                // 正常队列则从指定时间点开始
                else 
                {
                    result = -1;
                    try 
                    {					
                        // 时间点需要参数配置
						int nRet = 0;
						MqLogDebug("GetTimeStamp [%s]", m_pDefaultMQPushConsumerImpl
								   ->getDefaultMQPushConsumer()->getConsumeTimestamp().c_str());
                        long timestamp = CDate::FormatDateTimeStoI(m_pDefaultMQPushConsumerImpl
										 ->getDefaultMQPushConsumer()->getConsumeTimestamp().c_str(),nRet);
                        result = m_pMQClientFactory->getMQAdminImpl()->searchOffset(mq, timestamp);		
                    }
                    catch (MQClientException& e) 
                    {
                        result = -1;
                    }
                }
            }
            // 其他错误
            else 
            {
                result = -1;
            }
            break;
        }
	default:
		break;
	}

	return result;
}

void RebalancePushImpl::messageQueueChanged(const std::string& topic,
	std::set<MessageQueue>& mqAll, 
	std::set<MessageQueue>& mqDivided)
{
}


void RebalancePushImpl::removeUnnecessaryMessageQueue(MessageQueue& mq, ProcessQueue& pq)
{
	m_pDefaultMQPushConsumerImpl->getOffsetStore()->persist(mq);
	m_pDefaultMQPushConsumerImpl->getOffsetStore()->removeOffset(mq);
	if (m_pDefaultMQPushConsumerImpl->isConsumeOrderly())
	{
		unlock(mq, true);
	}
}

