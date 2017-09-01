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

#include "DefaultMQPushConsumerImpl.h"
#include <string>
#include <set>

#include "DefaultMQPushConsumer.h"
#include "ConsumerStatManage.h"
#include "DefaultMQPullConsumer.h"
#include "DefaultMQProducer.h"
#include "MQClientFactory.h"
#include "MQAdminImpl.h"
#include "RebalancePushImpl.h"
#include "MQClientAPIImpl.h"
#include "OffsetStore.h"
#include "MixAll.h"
#include "MQClientManager.h"
#include "LocalFileOffsetStore.h"
#include "RemoteBrokerOffsetStore.h"
#include "PullSysFlag.h"
#include "FilterAPI.h"
#include "PullAPIWrapper.h"
#include "MQClientException.h"
#include "Validators.h"
#include "MessageListener.h"
#include "ConsumeMessageHook.h"
#include "PullMessageService.h"
#include "ConsumeMessageOrderlyService.h"
#include "ConsumeMessageConcurrentlyService.h"
#include "KPRUtil.h"

// 拉消息异常时，延迟一段时间再拉
long long DefaultMQPushConsumerImpl::s_PullTimeDelayMillsWhenException = 3000;
long long DefaultMQPushConsumerImpl::s_PullTimeDelayMillsWhenFlowControl = 100;
long long DefaultMQPushConsumerImpl::s_PullTimeDelayMillsWhenSuspend = 1000;
// 长轮询模式，Consumer连接在Broker挂起最长时间
long long DefaultMQPushConsumerImpl::s_BrokerSuspendMaxTimeMillis = 1000 * 15;
// 长轮询模式，Consumer超时时间（必须要大于brokerSuspendMaxTimeMillis）
long long DefaultMQPushConsumerImpl::s_ConsumerTimeoutMillisWhenSuspend = 1000 * 30;

DefaultMQPushConsumerImpl::DefaultMQPushConsumerImpl(DefaultMQPushConsumer* pDefaultMQPushConsumer)
{
	m_tcpTimeoutMilliSeconds = MixAll::DEFAULT_TCP_TIMEOUT_MILLISECONDS;

	m_pDefaultMQPushConsumer = pDefaultMQPushConsumer;
	m_serviceState = CREATE_JUST;
	flowControlTimes1 = 0;
	flowControlTimes2 = 0;

    /* modified by yu.guangjie at 2015-10-21, reason: */
    m_pause = false;
    
    m_pPullAPIWrapper = NULL;
    m_pOffsetStore = NULL;
    m_pMQClientFactory = NULL;

    m_pConsumerStatManager = new ConsumerStatManager();
	m_pRebalanceImpl = new RebalancePushImpl(this);
    m_pullLaterService = new kpr::TimerThread("PullLaterServiceTimer",1000);
    m_pullLaterService->Start();
}

DefaultMQPushConsumerImpl::~DefaultMQPushConsumerImpl()
{
    /* modified by yu.guangjie at 2015-08-28, reason: */
    if(m_pConsumerStatManager != NULL)
    {
        delete m_pConsumerStatManager;
        m_pConsumerStatManager = NULL;
    }
    if(m_pRebalanceImpl != NULL)
    {
        delete m_pRebalanceImpl;
        m_pRebalanceImpl = NULL;
    }

    if(m_pPullAPIWrapper != NULL)
    {
        delete m_pPullAPIWrapper;
    }
    if(m_pOffsetStore != NULL)
    {
        delete m_pOffsetStore;
    }
}

bool DefaultMQPushConsumerImpl::hasHook()
{
	return !m_hookList.empty();
}

void DefaultMQPushConsumerImpl::registerHook(ConsumeMessageHook* pHook)
{
	m_hookList.push_back(pHook);
}

void DefaultMQPushConsumerImpl::executeHookBefore(ConsumeMessageContext& context)
{
	std::list<ConsumeMessageHook*>::iterator it = m_hookList.begin();
	for (;it!=m_hookList.end();it++)
	{
		try
		{
			(*it)->consumeMessageBefore(context);
		}
		catch (...)
		{
		}
	}
}

void DefaultMQPushConsumerImpl::executeHookAfter(ConsumeMessageContext& context)
{
	std::list<ConsumeMessageHook*>::iterator it = m_hookList.begin();
	for (;it!=m_hookList.end();it++)
	{
		try
		{
			(*it)->consumeMessageAfter(context);
		}
		catch (...)
		{
		}
	}
}

void DefaultMQPushConsumerImpl::createTopic(const std::string& key, const std::string& newTopic, int queueNum)
{
	m_pMQClientFactory->getMQAdminImpl()->createTopic(key, newTopic, queueNum);
}

std::set<MessageQueue>* DefaultMQPushConsumerImpl::fetchSubscribeMessageQueues(const std::string& topic)
{
	std::map<std::string, std::set<MessageQueue> >& mqs =  m_pRebalanceImpl->getTopicSubscribeInfoTable();
	std::map<std::string, std::set<MessageQueue> >::iterator it = mqs.find(topic);

	if (it==mqs.end())
	{
		m_pMQClientFactory->updateTopicRouteInfoFromNameServer(topic);
		mqs =  m_pRebalanceImpl->getTopicSubscribeInfoTable();
		it = mqs.find(topic);
	}

	if (it==mqs.end())
	{
		THROW_MQEXCEPTION(MQClientException,"The topic[" + topic + "] not exist", -1);
	}

	std::set<MessageQueue>* result = new std::set<MessageQueue>(it->second.begin(),it->second.end());
	return result;
}

DefaultMQPushConsumer* DefaultMQPushConsumerImpl::getDefaultMQPushConsumer()
{
	return m_pDefaultMQPushConsumer;
}

long long DefaultMQPushConsumerImpl::earliestMsgStoreTime(const MessageQueue& mq)
{
	return m_pMQClientFactory->getMQAdminImpl()->earliestMsgStoreTime(mq);
}

long long DefaultMQPushConsumerImpl::maxOffset(const MessageQueue& mq)
{
	return m_pMQClientFactory->getMQAdminImpl()->maxOffset(mq);
}

long long DefaultMQPushConsumerImpl::minOffset(const MessageQueue& mq)
{
	return m_pMQClientFactory->getMQAdminImpl()->minOffset(mq);
}

OffsetStore* DefaultMQPushConsumerImpl::getOffsetStore()
{
	return m_pOffsetStore;
}

void DefaultMQPushConsumerImpl::setOffsetStore(OffsetStore* pOffsetStore)
{
	m_pOffsetStore = pOffsetStore;
}

void DefaultMQPushConsumerImpl::setTcpTimeoutMilliseconds(int milliseconds)
{
	m_tcpTimeoutMilliSeconds = milliseconds;

	NULL == m_pMQClientFactory ?
		NULL:
		(m_pMQClientFactory->setTcpTimeoutMilliseconds(milliseconds), NULL);
}

int DefaultMQPushConsumerImpl::getTcpTimeoutMilliseconds()
{
	return m_tcpTimeoutMilliSeconds;
}

//MQConsumerInner
std::string DefaultMQPushConsumerImpl::groupName()
{
	return m_pDefaultMQPushConsumer->getConsumerGroup();
}

MessageModel DefaultMQPushConsumerImpl::messageModel()
{
	return m_pDefaultMQPushConsumer->getMessageModel();
}

ConsumeType DefaultMQPushConsumerImpl::consumeType()
{
	return CONSUME_PASSIVELY;
}

ConsumeFromWhere DefaultMQPushConsumerImpl::consumeFromWhere()
{
	return m_pDefaultMQPushConsumer->getConsumeFromWhere();
}

std::set<SubscriptionData> DefaultMQPushConsumerImpl::subscriptions()
{
	std::set<SubscriptionData> sds;
    /* modified by yu.guangjie at 2015-08-20, reason: */
	std::map<std::string, SubscriptionData> subscription = m_pRebalanceImpl->getSubscriptionInner();
	std::map<std::string, SubscriptionData>::iterator it = subscription.begin();

	for (;it!=subscription.end();it++)
	{
		sds.insert(it->second);
	}

	return sds;
}

void DefaultMQPushConsumerImpl::doRebalance()
{
	if (m_pRebalanceImpl != NULL)
	{
		m_pRebalanceImpl->doRebalance();
	}
}

void DefaultMQPushConsumerImpl::persistConsumerOffset()
{
	try
	{
		makeSureStateOK();

		std::set<MessageQueue> mqs;
		std::map<MessageQueue, ProcessQueue*>& mqps = m_pRebalanceImpl->getProcessQueueTable();
		std::map<MessageQueue, ProcessQueue*>::iterator it = mqps.begin();
		for (;it!= mqps.end();it++)
		{
			mqs.insert(it->first);
		}

		m_pOffsetStore->persistAll(mqs);
	}
	catch (MQException& e)
	{
	    MqLogWarn("group[%s] persistConsumerOffset exception: %s", 
            m_pDefaultMQPushConsumer->getConsumerGroup().c_str(), e.what());
	}
}

void DefaultMQPushConsumerImpl::updateTopicSubscribeInfo(const std::string& topic, const std::set<MessageQueue>& info)
{
	if (m_pRebalanceImpl->hasSubscribe(topic))
	{
        /* modified by yu.guangjie at 2015-08-14, reason: */
		//m_pRebalanceImpl->getTopicSubscribeInfoTable().insert(std::pair<std::string, std::set<MessageQueue> >(topic, info));
        m_pRebalanceImpl->getTopicSubscribeInfoTable()[topic] = info;
        MqLogNotice("Update subscribe topic[%s] ruoteinfo: MessageQueue=%d", 
            topic.c_str(), info.size());
	}
}


bool DefaultMQPushConsumerImpl::isSubscribeTopicNeedUpdate(const std::string& topic)
{
    /* modified by yu.guangjie at 2015-08-20, reason: */
	if (m_pRebalanceImpl->hasSubscribe(topic))
	{
		std::map<std::string, std::set<MessageQueue> >& mqs=
					m_pRebalanceImpl->getTopicSubscribeInfoTable();

		return mqs.find(topic)==mqs.end();
	}

	return false;
}

bool DefaultMQPushConsumerImpl::isPause()
{
	return m_pause;
}

void DefaultMQPushConsumerImpl::setPause(bool pause)
{
	m_pause = pause;
}

/**
* 通过Tag过滤时，会存在offset不准确的情况，需要纠正
*/
void DefaultMQPushConsumerImpl::correctTagsOffset(PullRequest& pullRequest)
{
	// 说明本地没有可消费的消息
	if (pullRequest.getProcessQueue()->getMsgCount().Get() == 0)
	{
		 m_pOffsetStore->updateOffset(*(pullRequest.getMessageQueue()), pullRequest.getNextOffset(), true);
	}
}

void DefaultMQPushConsumerImpl::pullMessage(PullRequest* pPullRequest)
{
    /* added by yu.guangjie at 2015-08-14, reason: */
    ProcessQueue * processQueue = pPullRequest->getProcessQueue();
    if (processQueue->isDroped()) 
    {
        MessageQueue* pMessque = pPullRequest->getMessageQueue();
        MqLogNotice("The pull request is droped: [topic=%s,broker=%s,queue=%d]",
            pMessque->getTopic().c_str(),pMessque->getBrokerName().c_str(),
            pMessque->getQueueId());
        /* modified by yu.guangjie at 2015-08-20, reason: delete PullRequest* */
        if(processQueue->getMsgTreeMap().empty())
        {
            MqLogNotice("Delete pull request: [topic=%s,broker=%s,queue=%d]",
                pMessque->getTopic().c_str(),pMessque->getBrokerName().c_str(),
                pMessque->getQueueId());
            delete pPullRequest;
        }
        else
        {/* wait for message consume over */
            executePullRequestLater(*pPullRequest, s_PullTimeDelayMillsWhenException);
        }
        return;
    }

    // 检测Consumer是否启动
    try 
    {
        makeSureStateOK();
    }
    catch (MQClientException& e) 
    {
        MqLogWarn("pullMessage exception, consumer state not ok: %s", e.what());
        executePullRequestLater(*pPullRequest, s_PullTimeDelayMillsWhenException);
        return;
    }

    // 检测Consumer是否被挂起
    if (isPause()) 
    {
        MqLogNotice("consumer was paused, execute pull request later");
        executePullRequestLater(*pPullRequest, s_PullTimeDelayMillsWhenSuspend);
        return;
    }

    // 流量控制，队列中消息总数
    long size = processQueue->getMsgCount();
    if (size > m_pDefaultMQPushConsumer->getPullThresholdForQueue()) 
    {
        executePullRequestLater(*pPullRequest, s_PullTimeDelayMillsWhenFlowControl);
        if ((flowControlTimes1++ % 1000) == 0) 
        {
            MqLogNotice("the consumer message buffer is full, so do flow control");
        }
        return;
    }

    // 流量控制，队列中消息最大跨度
    if (!m_consumeOrderly) 
    {
        if (processQueue->getMaxSpan() > m_pDefaultMQPushConsumer->getConsumeConcurrentlyMaxSpan()) 
        {
            executePullRequestLater(*pPullRequest, s_PullTimeDelayMillsWhenFlowControl);
            if ((flowControlTimes2++ % 1000) == 0) 
            {
                MqLogNotice("the queue's messages, span too long, so do flow control");
            }
            return;
        }
    }

    //
    MessageQueue *pMesQueue = pPullRequest->getMessageQueue();

    // 查询订阅关系    
    /* modified by yu.guangjie at 2015-08-20, reason: */
    SubscriptionData subscriptionData; 
    if(!m_pRebalanceImpl->hasSubscribe(pMesQueue->getTopic(), &subscriptionData))
    {
        // 由于并发关系，即使找不到订阅关系，也要重试下，防止丢失PullRequest
        executePullRequestLater(*pPullRequest, s_PullTimeDelayMillsWhenException);
        MqLogWarn("find the consumer's subscription failed!");
        return;
    }

    PullCallback* pullCallback = new DefaultMQPushConsumerImplCallback(subscriptionData,
        this, pPullRequest, GetCurrentTimeMillis());

    bool commitOffsetEnable = false;
    long commitOffsetValue = 0L;
    if (CLUSTERING == m_pDefaultMQPushConsumer->getMessageModel()) {
        commitOffsetValue =
                m_pOffsetStore->readOffset(*pMesQueue, READ_FROM_MEMORY);
        if (commitOffsetValue > 0) {
            commitOffsetEnable = true;
        }
    }

    int sysFlag = PullSysFlag::buildSysFlag(//
        commitOffsetEnable, // commitOffset
        true, // suspend
        false // subscription
        );
    try {
        m_pPullAPIWrapper->pullKernelImpl(//
            *pMesQueue, // 1
            "", // 2
            subscriptionData.getSubVersion(), // 3
            pPullRequest->getNextOffset(), // 4
            m_pDefaultMQPushConsumer->getPullBatchSize(), // 5
            sysFlag, // 6
            commitOffsetValue,// 7
            DefaultMQPushConsumerImpl::s_BrokerSuspendMaxTimeMillis, // 8
            DefaultMQPushConsumerImpl::s_ConsumerTimeoutMillisWhenSuspend, // 9
            ASYNC, // 10
            pullCallback// 11
            );
    }
    catch (MQException& e) 
    {
        MqLogWarn("pullKernelImpl exception: %s", e.what());
        executePullRequestLater(*pPullRequest, DefaultMQPushConsumerImpl::s_PullTimeDelayMillsWhenException);
    }
    
}

/**
* 立刻执行这个PullRequest
*/
void DefaultMQPushConsumerImpl::executePullRequestImmediately(PullRequest& pullRequest)
{
	m_pMQClientFactory->getPullMessageService()->executePullRequestImmediately(&pullRequest);
}

/**
* 稍后再执行这个PullRequest
*/
void DefaultMQPushConsumerImpl::executePullRequestLater(PullRequest& pullRequest, long timeDelay)
{
	m_pMQClientFactory->getPullMessageService()->executePullRequestLater(&pullRequest, timeDelay);
}


void DefaultMQPushConsumerImpl::executeTaskLater(PullRequest& pullRequest, long timeDelay)
{
    ExecutePullTaskLater* pullLater = new ExecutePullTaskLater(&pullRequest, this);

	m_pullLaterService->RegisterTimer(0,timeDelay,pullLater,false);
}


void DefaultMQPushConsumerImpl::makeSureStateOK()
{
	if (m_serviceState != RUNNING)
	{
		THROW_MQEXCEPTION(MQClientException,"The consumer service state not OK, ", -1);
	}
}

ConsumerStatManager* DefaultMQPushConsumerImpl::getConsumerStatManager()
{
	return m_pConsumerStatManager;
}

QueryResult DefaultMQPushConsumerImpl::queryMessage(const std::string& topic,
		const std::string&  key,
		int maxNum,
		long long begin,
		long long end)
{
	return m_pMQClientFactory->getMQAdminImpl()->queryMessage(topic, key, maxNum, begin, end);
}

void DefaultMQPushConsumerImpl::registerMessageListener(MessageListener* pMessageListener)
{
	m_pMessageListenerInner = pMessageListener;
}

void DefaultMQPushConsumerImpl::resume()
{
	m_pause = false;
}

long long DefaultMQPushConsumerImpl::searchOffset(const MessageQueue& mq, long long timestamp)
{
	return m_pMQClientFactory->getMQAdminImpl()->searchOffset(mq, timestamp);
}

void DefaultMQPushConsumerImpl::sendMessageBack(MessageExt& msg, int delayLevel)
{
	try 
	{
		m_pMQClientFactory->getMQClientAPIImpl()->consumerSendMessageBack(msg,
			m_pDefaultMQPushConsumer->getConsumerGroup(), 
			delayLevel, 
			//3000);	mdy by lin.qiongshan, 2016-9-2, 写死的时间改为从属性中获取
			getTcpTimeoutMilliseconds());
	}
	catch (...)
	{
		Message newMsg(MixAll::getRetryTopic(m_pDefaultMQPushConsumer->getConsumerGroup()),
			msg.getBody(),msg.getBodyLen());

		newMsg.setFlag(msg.getFlag());
		newMsg.setProperties(msg.getProperties());
		newMsg.putProperty(Message::PROPERTY_RETRY_TOPIC, msg.getTopic());

		m_pMQClientFactory->getDefaultMQProducer()->send(newMsg);
	}
}

void DefaultMQPushConsumerImpl::shutdown()
{
	switch (m_serviceState)
	{
	case CREATE_JUST:
		break;
	case RUNNING:
		m_pConsumeMessageService->shutdown();
		persistConsumerOffset();
		m_pMQClientFactory->unregisterConsumer(m_pDefaultMQPushConsumer->getConsumerGroup());
		m_pMQClientFactory->shutdown();

		m_serviceState = SHUTDOWN_ALREADY;
		break;
	case SHUTDOWN_ALREADY:
		break;
	default:
		break;
	}
}

void DefaultMQPushConsumerImpl::start()
{
    MqLogNotice("MQPushConsumer start...");
	switch (m_serviceState)
	{
	case CREATE_JUST:
		{
			m_serviceState = START_FAILED;

			checkConfig();

			// 复制订阅关系
			copySubscription();

			if (m_pDefaultMQPushConsumer->getMessageModel() == CLUSTERING)
			{
				m_pDefaultMQPushConsumer->changeInstanceNameToPID();
			}

			m_pMQClientFactory = MQClientManager::getInstance()->getAndCreateMQClientFactory(*m_pDefaultMQPushConsumer);

			// 初始化Rebalance变量
			m_pRebalanceImpl->setConsumerGroup(m_pDefaultMQPushConsumer->getConsumerGroup());
			m_pRebalanceImpl->setMessageModel(m_pDefaultMQPushConsumer->getMessageModel());
			m_pRebalanceImpl->setAllocateMessageQueueStrategy(m_pDefaultMQPushConsumer->getAllocateMessageQueueStrategy());
			m_pRebalanceImpl->setmQClientFactory(m_pMQClientFactory);

			m_pPullAPIWrapper = new PullAPIWrapper(m_pMQClientFactory, m_pDefaultMQPushConsumer->getConsumerGroup());

            
			if (m_pDefaultMQPushConsumer->getOffsetStore() != NULL)
			{   
				m_pOffsetStore = m_pDefaultMQPushConsumer->getOffsetStore();
			}
			else
			{
				// 广播消费/集群消费
				switch (m_pDefaultMQPushConsumer->getMessageModel())
				{
				case BROADCASTING:
					m_pOffsetStore = new LocalFileOffsetStore(m_pMQClientFactory, m_pDefaultMQPushConsumer->getConsumerGroup());
					break;
				case CLUSTERING:
					m_pOffsetStore = new RemoteBrokerOffsetStore(m_pMQClientFactory, m_pDefaultMQPushConsumer->getConsumerGroup());
					break;
				default:
					break;
				}
			}
			// 加载消费进度
			m_pOffsetStore->load();

			// 启动消费消息服务
			if (dynamic_cast<MessageListenerOrderly*>(m_pMessageListenerInner) != NULL)
			{
				m_consumeOrderly = true;
				m_pConsumeMessageService =
					new ConsumeMessageOrderlyService(this,(MessageListenerOrderly*)m_pMessageListenerInner);
			}
			else if (dynamic_cast<MessageListenerConcurrently*>(m_pMessageListenerInner) != NULL)
			{
				m_consumeOrderly = false;
				m_pConsumeMessageService =
					new ConsumeMessageConcurrentlyService(this, (MessageListenerConcurrently*)m_pMessageListenerInner);
			}
			m_pConsumeMessageService->start();
			bool registerOK =m_pMQClientFactory->registerConsumer(m_pDefaultMQPushConsumer->getConsumerGroup(), this);
			if (!registerOK)
			{
				m_serviceState = CREATE_JUST;
				m_pConsumeMessageService->shutdown();
				std::string str = "The consumer group["+ m_pDefaultMQPushConsumer->getConsumerGroup();
				str += "] has been created before, specify another name please.";
				THROW_MQEXCEPTION(MQClientException,str,-1);
			}
			m_pMQClientFactory->start();

			m_serviceState = RUNNING;
		}
		break;
	case RUNNING:
	case START_FAILED:
	case SHUTDOWN_ALREADY:
		THROW_MQEXCEPTION(MQClientException,"The PullConsumer service state not OK, maybe started once, ",-1);
	default:
		break;
	}
	updateTopicSubscribeInfoWhenSubscriptionChanged();
	m_pMQClientFactory->sendHeartbeatToAllBrokerWithLock();
	m_pMQClientFactory->rebalanceImmediately();
	// 不阻塞，由应用主线程控制
}

void DefaultMQPushConsumerImpl::checkConfig()
{
	// consumerGroup 有效性检查
	Validators::checkGroup(m_pDefaultMQPushConsumer->getConsumerGroup());

	// consumerGroup
	if (m_pDefaultMQPushConsumer->getConsumerGroup()==MixAll::DEFAULT_CONSUMER_GROUP)
	{
		THROW_MQEXCEPTION(MQClientException,"consumerGroup can not equal "
			+ MixAll::DEFAULT_CONSUMER_GROUP //
			+ ", please specify another one.",-1);
	}

	if (m_pDefaultMQPushConsumer->getMessageModel()!=BROADCASTING
		&& m_pDefaultMQPushConsumer->getMessageModel()!=CLUSTERING)
	{
			THROW_MQEXCEPTION(MQClientException,"messageModel is invalid ",-1);
	}

	// allocateMessageQueueStrategy
	if (m_pDefaultMQPushConsumer->getAllocateMessageQueueStrategy()==NULL)
	{
		THROW_MQEXCEPTION(MQClientException,"allocateMessageQueueStrategy is null",-1);
	}

	// consumeFromWhereOffset
	if (m_pDefaultMQPushConsumer->getConsumeFromWhere()<CONSUME_FROM_LAST_OFFSET
		||m_pDefaultMQPushConsumer->getConsumeFromWhere()>CONSUME_FROM_TIMESTAMP)
	{
		THROW_MQEXCEPTION(MQClientException,"consumeFromWhere is invalid",-1);
	}

	// subscription
	//if (m_pDefaultMQPushConsumer->getSubscription()==NULL)
	//{
	//	THROW_MQEXCEPTION(MQClientException,"subscription is null" ,-1);
	//}

	// messageListener
	if (m_pDefaultMQPushConsumer->getMessageListener()==NULL)
	{
		THROW_MQEXCEPTION(MQClientException,"messageListener is null",-1);
	}

	MessageListener* listener = m_pDefaultMQPushConsumer->getMessageListener();
	MessageListener* orderly =  (dynamic_cast<MessageListenerOrderly*>(listener)) ;
	MessageListener* concurrently =(dynamic_cast<MessageListenerConcurrently*>(listener)) ;

	if (!orderly && !concurrently)
	{
		THROW_MQEXCEPTION(MQClientException,
			"messageListener must be instanceof MessageListenerOrderly or MessageListenerConcurrently" ,
			-1);
	}

	// consumeThreadMin
	if (m_pDefaultMQPushConsumer->getConsumeThreadMin() < 1 
		|| m_pDefaultMQPushConsumer->getConsumeThreadMin() > 1000
		|| m_pDefaultMQPushConsumer->getConsumeThreadMin() > m_pDefaultMQPushConsumer->getConsumeThreadMax()
		)
	{
		THROW_MQEXCEPTION(MQClientException,"consumeThreadMin Out of range [1, 1000]",-1);
	}

	// consumeThreadMax
	if (m_pDefaultMQPushConsumer->getConsumeThreadMax() < 1
		|| m_pDefaultMQPushConsumer->getConsumeThreadMax() > 1000)
	{
			THROW_MQEXCEPTION(MQClientException,"consumeThreadMax Out of range [1, 1000]",-1);
	}

	// consumeConcurrentlyMaxSpan
	if (m_pDefaultMQPushConsumer->getConsumeConcurrentlyMaxSpan() < 1
		|| m_pDefaultMQPushConsumer->getConsumeConcurrentlyMaxSpan() > 65535)
	{
			THROW_MQEXCEPTION(MQClientException,"consumeConcurrentlyMaxSpan Out of range [1, 65535]" ,-1);
	}

	// pullThresholdForQueue
	if (m_pDefaultMQPushConsumer->getPullThresholdForQueue() < 1
		|| m_pDefaultMQPushConsumer->getPullThresholdForQueue() > 65535)
	{
			THROW_MQEXCEPTION(MQClientException,"pullThresholdForQueue Out of range [1, 65535]",-1);
	}

	// pullInterval
	if (m_pDefaultMQPushConsumer->getPullInterval() < 0
		|| m_pDefaultMQPushConsumer->getPullInterval() > 65535)
	{
			THROW_MQEXCEPTION(MQClientException,"pullInterval Out of range [0, 65535]",-1);
	}

	// consumeMessageBatchMaxSize
	if (m_pDefaultMQPushConsumer->getConsumeMessageBatchMaxSize() < 1
		|| m_pDefaultMQPushConsumer->getConsumeMessageBatchMaxSize() > 1024)
	{
			THROW_MQEXCEPTION(MQClientException,"consumeMessageBatchMaxSize Out of range [1, 1024]",-1);
	}

	// pullBatchSize
	if (m_pDefaultMQPushConsumer->getPullBatchSize() < 1
		|| m_pDefaultMQPushConsumer->getPullBatchSize() > 1024)
	{
			THROW_MQEXCEPTION(MQClientException,"pullBatchSize Out of range [1, 1024]",-1);
	}
}

void DefaultMQPushConsumerImpl::copySubscription()
{
	try
	{
		// 复制用户初始设置的订阅关系
		std::map<std::string, std::string>& sub = m_pDefaultMQPushConsumer->getSubscription();
		std::map<std::string, std::string>::iterator it = sub.begin();
		for (;it!=sub.end();it++)
		{
			SubscriptionData* subscriptionData = FilterAPI::buildSubscriptionData(it->first, it->second);
            /* modified by yu.guangjie at 2015-08-20, reason: */
            m_pRebalanceImpl->subscribe(it->first, *subscriptionData);
            delete subscriptionData;
		}

		if (m_pMessageListenerInner == NULL)
		{
			m_pMessageListenerInner = m_pDefaultMQPushConsumer->getMessageListener();
		}

		switch (m_pDefaultMQPushConsumer->getMessageModel())
		{
		case BROADCASTING:
			break;
		case CLUSTERING:
			{
				// 默认订阅消息重试Topic
				std::string retryTopic = MixAll::getRetryTopic(m_pDefaultMQPushConsumer->getConsumerGroup());
				SubscriptionData* subscriptionData =
					FilterAPI::buildSubscriptionData(retryTopic, SubscriptionData::SUB_ALL);
                /* modified by yu.guangjie at 2015-08-20, reason: */
                m_pRebalanceImpl->subscribe(retryTopic, *subscriptionData);
                delete subscriptionData;
			}

			break;
		default:
			break;
		}
	}
	catch (...)
	{
		THROW_MQEXCEPTION(MQClientException,"subscription exception", -1);
	}
}

void DefaultMQPushConsumerImpl::updateTopicSubscribeInfoWhenSubscriptionChanged()
{
    /* modified by yu.guangjie at 2015-08-20, reason: */
	std::map<std::string, SubscriptionData> subTable = m_pRebalanceImpl->getSubscriptionInner();

	std::map<std::string, SubscriptionData>::iterator it = subTable.begin();
	for (;it!=subTable.end();it++)
	{
		m_pMQClientFactory->updateTopicRouteInfoFromNameServer(it->first);
	}
}

MessageListener* DefaultMQPushConsumerImpl::getMessageListenerInner()
{
	return m_pMessageListenerInner;
}

void DefaultMQPushConsumerImpl::subscribe(const std::string& topic, const std::string& subExpression)
{
	try
	{
		SubscriptionData* subscriptionData = FilterAPI::buildSubscriptionData(topic, subExpression);
        /* modified by yu.guangjie at 2015-08-20, reason: */
        m_pRebalanceImpl->subscribe(topic, *subscriptionData);
        delete subscriptionData;

		// 发送心跳，将变更的订阅关系注册上去
		if (m_pMQClientFactory )
		{
			m_pMQClientFactory->sendHeartbeatToAllBrokerWithLock();
		}
	}
	catch (...)
	{
		THROW_MQEXCEPTION(MQClientException,"subscription exception", -1);
	}
}

void DefaultMQPushConsumerImpl::suspend()
{
	m_pause = true;
}

void DefaultMQPushConsumerImpl::unsubscribe(const std::string& topic)
{
    /* modified by yu.guangjie at 2015-08-20, reason: */
	 m_pRebalanceImpl->unsubscribe(topic);
}

void DefaultMQPushConsumerImpl::updateConsumeOffset(MessageQueue& mq, long long offset)
{
	m_pOffsetStore->updateOffset(mq, offset, false);
}

void DefaultMQPushConsumerImpl::updateCorePoolSize(int corePoolSize)
{
	m_pConsumeMessageService->updateCorePoolSize(corePoolSize);
}

MessageExt* DefaultMQPushConsumerImpl::viewMessage(const std::string& msgId)
{
	return m_pMQClientFactory->getMQAdminImpl()->viewMessage(msgId);
}

RebalanceImpl* DefaultMQPushConsumerImpl::getRebalanceImpl()
{
	return m_pRebalanceImpl;
}

/* modified by yu.guangjie at 2015-08-17, reason: add */
PullAPIWrapper* DefaultMQPushConsumerImpl::getPullAPIWrapper()
{
    return m_pPullAPIWrapper;
}

bool DefaultMQPushConsumerImpl::isConsumeOrderly()
{
	return m_consumeOrderly;
}

void DefaultMQPushConsumerImpl::setConsumeOrderly(bool consumeOrderly)
{
	m_consumeOrderly = consumeOrderly;
}


ExecutePullTaskLater::ExecutePullTaskLater(PullRequest* pPullRequest,
		DefaultMQPushConsumerImpl* pDefaultMQPushConsumerImpl)
		:m_pDefaultMQPushConsumerImpl(pDefaultMQPushConsumerImpl),
		m_pPullRequest(pPullRequest)
{
}

void ExecutePullTaskLater::OnTimeOut(unsigned int timerID)
{

    try 
    {
        MessageQueue *pMesQueue = m_pPullRequest->getMessageQueue();
        // 第三步、纠正内部Offset
        m_pDefaultMQPushConsumerImpl->getOffsetStore()->updateOffset(
            *pMesQueue, m_pPullRequest->getNextOffset(), false);

        // 第四步、将最新的Offset更新到服务器
        m_pDefaultMQPushConsumerImpl->getOffsetStore()->persist(*pMesQueue);

        // 第五步、丢弃当前PullRequest，并且从Rebalabce结果里删除，等待下次Rebalance时，取纠正后的Offset
        m_pDefaultMQPushConsumerImpl->getRebalanceImpl()->removeProcessQueue(*pMesQueue);
    }
    catch (...) 
    {
        MqLogWarn("ExecutePullTaskLater Exception");
    }
    
	delete this;
}

DefaultMQPushConsumerImplCallback::DefaultMQPushConsumerImplCallback(
        SubscriptionData& subscriptionData,
		DefaultMQPushConsumerImpl* pDefaultMQPushConsumerImpl,
		PullRequest* pPullRequest, long long beginTimestamp)
		:m_subscriptionData(subscriptionData),
		m_pDefaultMQPushConsumerImpl(pDefaultMQPushConsumerImpl),
		m_pPullRequest(pPullRequest),
		m_beginTimestamp(beginTimestamp)
{
}

void DefaultMQPushConsumerImplCallback::onSuccess(PullResult& pullResult)
{
    ProcessQueue * processQueue = m_pPullRequest->getProcessQueue();
    MessageQueue * messQueue = m_pPullRequest->getMessageQueue();
    m_pDefaultMQPushConsumerImpl->getPullAPIWrapper()->processPullResult(
        *messQueue, pullResult, m_subscriptionData);

    switch (pullResult.getPullStatus()) {
    case FOUND:
        {
            m_pPullRequest->setNextOffset(pullResult.getNextBeginOffset());
            long long pullRT = GetCurrentTimeMillis() - m_beginTimestamp;
            m_pDefaultMQPushConsumerImpl->getConsumerStatManager()->getConsumertat().pullTimesTotal++;
            m_pDefaultMQPushConsumerImpl->getConsumerStatManager()->getConsumertat().pullRTTotal += pullRT;
            
            bool dispathToConsume = processQueue->putMessage(pullResult.getMsgFoundList());
            m_pDefaultMQPushConsumerImpl->getConsumeMessageService()->submitConsumeRequest(//
                &(pullResult.getMsgFoundList()), //
                processQueue, //
                messQueue, //
                dispathToConsume);

            // 流控
            if (m_pDefaultMQPushConsumerImpl->getDefaultMQPushConsumer()->getPullInterval() > 0) {
                m_pDefaultMQPushConsumerImpl->executePullRequestLater(*m_pPullRequest,
                    m_pDefaultMQPushConsumerImpl->getDefaultMQPushConsumer()->getPullInterval());
            }
            // 立刻拉消息
            else {
                m_pDefaultMQPushConsumerImpl->executePullRequestImmediately(*m_pPullRequest);
            }
        }
        break;
    case NO_NEW_MSG:
    case NO_MATCHED_MSG:
        {
            m_pPullRequest->setNextOffset(pullResult.getNextBeginOffset());
            m_pDefaultMQPushConsumerImpl->correctTagsOffset(*m_pPullRequest);
            m_pDefaultMQPushConsumerImpl->executePullRequestImmediately(*m_pPullRequest);
        }        
        break;
    case OFFSET_ILLEGAL:
        {
            MqLogWarn("the pull request offset illegal");
            m_pPullRequest->setNextOffset(pullResult.getNextBeginOffset());
            // 第一步、缓存队列里的消息全部废弃
            processQueue->setDroped(true);
            // 第二步、等待10s后再执行，防止Offset更新后又被覆盖
            m_pDefaultMQPushConsumerImpl->executeTaskLater(*m_pPullRequest, 10000);
        }        
        break;
    default:
        break;
    }

}
	
void DefaultMQPushConsumerImplCallback::onException(MQException& e)
{
    if (m_pPullRequest->getMessageQueue()->getTopic().find(MixAll::RETRY_GROUP_TOPIC_PREFIX) != 0)
    {
        MqLogNotice("execute the pull request exception: %s", e.what());
    }

    m_pDefaultMQPushConsumerImpl->executePullRequestLater(*m_pPullRequest,
        DefaultMQPushConsumerImpl::s_PullTimeDelayMillsWhenException);
}


