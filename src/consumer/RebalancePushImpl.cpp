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
	// �ɷ�PullRequest
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
			// ��ǰ�������ڷ�����û�ж�Ӧ��Offset
			// ˵���ǵ�һ������
			else if (-1 == lastOffset)
			{
				// ��������Զ��У���Ҫ��0��ʼ
                if (mq.getTopic().find(MixAll::RETRY_GROUP_TOPIC_PREFIX) == 0 )
				{
					result = 0L;
				}
				// �����������ĩβ��ʼ
				else
				{
					/**Note by lin.qiongshan, 2016-11-10, ΪʲôҪ�����޸�
						����Ҫ�����������������ѿ�ʼλ�ú󣬻ᰴ��λ��������������ȡָ��λ�õ���Ϣ
						��Σ�����˻ᶨ��������ڵ���Ϣ�����һ�����������ڵ���Ϣ���ܻᱻɾ������ʱ�ö��е� minOffset ������0����ɾ����Ϣ�������� offset

						���������ǰ�����Ϣ��ɾ����������Ŀǰ��ʹ�������consumer_group �״�ע�ᣬ������ offset��code 14��ʱ������˷��ش�����22������ MQClientAPIImpl::queryConsumerOffset ע���е�˵����
						��ʱ��RemoteBrokerOffsetStore::readOffset �ᷢ�� -1 �ߵ��˴�

						�� CONSUME_FROM_LAST_OFFSET Ҫ��Ӧ�ôӶ���β��ʼ���ѣ�֮ǰ��ֱ�ӷ��� LLONG_MAX����ȡ��λ�õ���Ϣ������λ�ò�������Ϣ������˻᷵�ش��󣺣������� 21��pull request offset illegal
							��ʱ��ȡ offset ʧ�ܣ��ᵼ���޷���ȡ��Ϣ
							����˷��ش���ͬʱЯ���ж��е� offset ���ݣ������ʱû������Ϣ���룬nextBeginOffset ָʾ��λ�������һ����Ϣ����һ��λ��
								{"code":21,"extFields":{"maxOffset":"6510","minOffset":"3672","nextBeginOffset":"6510","suggestWhichBroker        Id":"0"},"flag":1,"language":"JAVA","opaque":136,"remark":"OFFSET_OVERFLOW_BADLY","version":61}
							��Ȼ��ȡ��Ϣʧ�ܣ�������Ϣ��ȡ��������DefaultMQPushConsumerImplCallback::onSuccess���У��ᱨ���ص���Ϣ�е� nextBeginOffset �����ȴ�10s����»� Broker
								{"code":15,"language":"CPP","version":19,"opaque":139,"flag":0,"remark":"","extFields":{"consumerGrou        p":"g_1005","topic":"topic_1","queueId":"0","commitOffset":"6614"}}
							offset ���³ɹ�������ȡ��Ϣ���Ϳ��Ի�ȡ��ȷ�� offset ֵ����ʱ�����������Ϣ���룬�Ϳ��Լ���������Ϣ��

						�޸ĺ󣬵� QUERY_CONSUMER_OFFSET_VALUE��code 14�� ��������ʧ�� �ӷ����������е� maxOffset�����һ����Ϣ����һ��λ�ã���code 30����Ϊ������ʼλ��
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
			// ������������
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
            // �ڶ��������������ϴε�����λ�㿪ʼ����
            if (lastOffset >= 0) 
            {
                result = lastOffset;
            }
            // ��һ��������û�м�¼����λ��
            else if (-1 == lastOffset) 
            {
                result = 0L;
            }
            // ��������
            else 
            {
                result = -1;
            }
            break;
        }
    case CONSUME_FROM_TIMESTAMP: 
        {
            long lastOffset = offsetStore->readOffset(mq, READ_FROM_STORE);
            // �ڶ��������������ϴε�����λ�㿪ʼ����
            if (lastOffset >= 0) 
            {
                result = lastOffset;
            }
            // ��һ��������û�м�¼����Ϊ��
            else if (-1 == lastOffset) 
            {
                // ���Զ�����Ӷ���β����ʼ
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
                // �����������ָ��ʱ��㿪ʼ
                else 
                {
                    result = -1;
                    try 
                    {					
                        // ʱ�����Ҫ��������
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
            // ��������
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

