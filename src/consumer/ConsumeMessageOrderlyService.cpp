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

#include "ConsumeMessageOrderlyService.h"

#include <list>
#include <string>

#include "ScopedLock.h"
#include "DefaultMQPushConsumerImpl.h"
#include "MessageListener.h"
#include "MessageQueue.h"
#include "RebalanceImpl.h"
#include "DefaultMQPushConsumer.h"
#include "OffsetStore.h"
#include "KPRUtil.h"
#include "MixAll.h"
#include "UtilAll.h"

class LockMq : public kpr::TimerHandler
{
public:
	LockMq(ConsumeMessageOrderlyService* pService)
		:m_pService(pService)
	{

	}

	void OnTimeOut(unsigned int timerID)
	{
	    MqLogDebug("LockMq::OnTimeOut() begin ...");
		m_pService->lockMQPeriodically();
        MqLogDebug("LockMq::OnTimeOut() end ...");
	}

private:
	ConsumeMessageOrderlyService* m_pService;
};

long ConsumeMessageOrderlyService::s_MaxTimeConsumeContinuously = 60000;

ConsumeMessageOrderlyService::ConsumeMessageOrderlyService(DefaultMQPushConsumerImpl* pDefaultMQPushConsumerImpl,
	MessageListenerOrderly* pMessageListener)
{
	m_pDefaultMQPushConsumerImpl = pDefaultMQPushConsumerImpl;
	m_pMessageListener = pMessageListener;
	m_pDefaultMQPushConsumer = m_pDefaultMQPushConsumerImpl->getDefaultMQPushConsumer();
	m_consumerGroup = m_pDefaultMQPushConsumer->getConsumerGroup();
	m_pConsumeExecutor = new kpr::ThreadPool(10,m_pDefaultMQPushConsumer->getConsumeThreadMin(),
		m_pDefaultMQPushConsumer->getConsumeThreadMax());
	m_scheduledExecutorService = new kpr::TimerThread("ConsumeMessageConcurrentlyService",1000);
}

void ConsumeMessageOrderlyService::start()
{
	m_scheduledExecutorService->Start();

	LockMq* lm = new LockMq(this);

	m_scheduledExecutorService->RegisterTimer(0,ProcessQueue::s_RebalanceLockInterval,lm);
}

void ConsumeMessageOrderlyService::shutdown()
{
    MqLogWarn("ConsumeMessageOrderlyService::shutdown()");
	m_stoped = true;
	m_pConsumeExecutor->Destroy();
	m_scheduledExecutorService->Close();
	unlockAllMQ();
}

void ConsumeMessageOrderlyService::unlockAllMQ()
{
	m_pDefaultMQPushConsumerImpl->getRebalanceImpl()->unlockAll(false);
}

void ConsumeMessageOrderlyService::lockMQPeriodically()
{
	if (!m_stoped)
	{
		m_pDefaultMQPushConsumerImpl->getRebalanceImpl()->lockAll();
	}
}

bool ConsumeMessageOrderlyService::lockOneMQ(MessageQueue& mq)
{
	if (!m_stoped)
	{
		return m_pDefaultMQPushConsumerImpl->getRebalanceImpl()->lock(mq);
	}

	return false;
}

class TryLockLaterAndReconsume : public kpr::TimerHandler
{
public:
	TryLockLaterAndReconsume(ProcessQueue* pProcessQueue,
		MessageQueue* pMessageQueue,
		ConsumeMessageOrderlyService* pService)
		:m_pProcessQueue(pProcessQueue),
		m_pMessageQueue(pMessageQueue),
		m_pService(pService)
	{

	}

	void OnTimeOut(unsigned int timerID)
	{
		bool lockOK = m_pService->lockOneMQ(*m_pMessageQueue);
        MqLogNotice("TryLockLaterAndReconsume::OnTimeOut(): topic=%s,broker=%s--%d,lockOK=%s",
            m_pMessageQueue->getTopic().c_str(),m_pMessageQueue->getBrokerName().c_str(),
            m_pMessageQueue->getQueueId(),lockOK?"true":"false");
		if (lockOK)
		{
			m_pService->submitConsumeRequestLater(m_pProcessQueue, m_pMessageQueue, 10);
		}
		else
		{
			m_pService->submitConsumeRequestLater(m_pProcessQueue, m_pMessageQueue, 3000);
		}

		delete this;
	}

private:
	ProcessQueue* m_pProcessQueue;
	MessageQueue* m_pMessageQueue;
	ConsumeMessageOrderlyService* m_pService;
};

void ConsumeMessageOrderlyService::tryLockLaterAndReconsume(MessageQueue* pMessageQueue,
															ProcessQueue* pProcessQueue,
															long long delayMills)
{
	TryLockLaterAndReconsume* consume = new TryLockLaterAndReconsume(pProcessQueue, pMessageQueue,this);

	m_scheduledExecutorService->RegisterTimer(0, int(delayMills), consume, false);
}

ConsumerStat& ConsumeMessageOrderlyService::getConsumerStat()
{
	return m_pDefaultMQPushConsumerImpl->getConsumerStatManager()->getConsumertat();
}

class SubmitConsumeRequestLaterOrderly : public kpr::TimerHandler
{
public:
	SubmitConsumeRequestLaterOrderly(ProcessQueue* pProcessQueue,
		MessageQueue* pMessageQueue,
		ConsumeMessageOrderlyService* pService)
		:m_pProcessQueue(pProcessQueue),
		m_pMessageQueue(pMessageQueue),
		m_pService(pService)
	{

	}

	void OnTimeOut(unsigned int timerID)
	{
		m_pService->submitConsumeRequest(NULL,m_pProcessQueue,m_pMessageQueue,true);

		delete this;
	}

private:
	ProcessQueue* m_pProcessQueue;
	MessageQueue* m_pMessageQueue;
	ConsumeMessageOrderlyService* m_pService;
};

void ConsumeMessageOrderlyService::submitConsumeRequestLater(ProcessQueue* pProcessQueue,
																MessageQueue* pMessageQueue,
																long long suspendTimeMillis)
{
	long timeMillis = long (suspendTimeMillis);
	if (timeMillis < 10)
	{
		timeMillis = 10;
	}
	else if (timeMillis > 30000)
	{
		timeMillis = 30000;
	}
    MqLogVerb("submitConsumeRequestLater(): topic=%s,broker=%s--%d",
            pMessageQueue->getTopic().c_str(),
            pMessageQueue->getBrokerName().c_str(),
            pMessageQueue->getQueueId());
	SubmitConsumeRequestLaterOrderly* sc = new SubmitConsumeRequestLaterOrderly(pProcessQueue, pMessageQueue,this);

	m_scheduledExecutorService->RegisterTimer(0, timeMillis, sc, false);
}

void ConsumeMessageOrderlyService::submitConsumeRequest(std::list<MessageExt*>* pMsgs,
														ProcessQueue* pProcessQueue,
														MessageQueue* pMessageQueue,
														bool dispathToConsume)
{
	if (dispathToConsume)
	{
	    MqLogVerb("submitConsumeRequest(),topic=%s,broker=%s--%d,msgs=%d",
            pMessageQueue->getTopic().c_str(),pMessageQueue->getBrokerName().c_str(),
            pMessageQueue->getQueueId(), (pMsgs!=NULL)?pMsgs->size():0);
		ConsumeOrderlyRequest* consumeRequest = new ConsumeOrderlyRequest(pProcessQueue, pMessageQueue,this);
		m_pConsumeExecutor->AddWork(consumeRequest);
	}
    /* modified by yu.guangjie at 2015-09-29, reason: clear pMsgs */
    if(pMsgs != NULL)
    {
        pMsgs->clear();
    }
}

void ConsumeMessageOrderlyService::updateCorePoolSize(int corePoolSize)
{
}


std::string& ConsumeMessageOrderlyService::getConsumerGroup()
{
	return m_consumerGroup;
}

MessageListenerOrderly* ConsumeMessageOrderlyService::getMessageListener()
{
	return m_pMessageListener;
}

DefaultMQPushConsumerImpl* ConsumeMessageOrderlyService::getDefaultMQPushConsumerImpl()
{
	return m_pDefaultMQPushConsumerImpl;
}

bool ConsumeMessageOrderlyService::processConsumeResult( std::list<MessageExt*>* pMsgs,
	ConsumeOrderlyStatus status,
	ConsumeOrderlyContext& context,
	ConsumeOrderlyRequest& consumeRequest )
{
	bool continueConsume = true;
	long long commitOffset = -1L;
	int msgsSize = pMsgs->size();

	// ������ʽ���Զ��ύ
	if (context.autoCommit)
	{
		switch (status)
		{
		case COMMIT:
		case ROLLBACK:
			// log.warn("the message queue consume result is illegal, we think you want to ack these message {}",
			//	consumeRequest.getMessageQueue());
		case SUCCESS:
			commitOffset = consumeRequest.getProcessQueue()->commit();
			// ͳ����Ϣ
			getConsumerStat().consumeMsgOKTotal.fetchAndAdd(msgsSize);
			break;
		case SUSPEND_CURRENT_QUEUE_A_MOMENT:
			consumeRequest.getProcessQueue()->makeMessageToCosumeAgain(*pMsgs);
			submitConsumeRequestLater(consumeRequest.getProcessQueue(),
				consumeRequest.getMessageQueue(),
				context.suspendCurrentQueueTimeMillis);
			continueConsume = false;

			// ͳ����Ϣ
			getConsumerStat().consumeMsgFailedTotal.fetchAndAdd(msgsSize);
			break;
		default:
			break;
		}
	}
	// ����ʽ�����û��������ύ�ع�
	else
	{
		switch (status)
		{
		case SUCCESS:
			// ͳ����Ϣ
			getConsumerStat().consumeMsgOKTotal.fetchAndAdd(msgsSize);
			break;
		case COMMIT:
			commitOffset = consumeRequest.getProcessQueue()->commit();
			// ͳ����Ϣ
			getConsumerStat().consumeMsgOKTotal.fetchAndAdd(msgsSize);
			break;
		case ROLLBACK:
			// ���Rollback�����suspendһ��������ѣ���ֹӦ������Rollback��ȥ
			consumeRequest.getProcessQueue()->rollback();
			submitConsumeRequestLater(consumeRequest.getProcessQueue(),
				consumeRequest.getMessageQueue(),
				context.suspendCurrentQueueTimeMillis);
			continueConsume = false;
			// ͳ����Ϣ
			getConsumerStat().consumeMsgFailedTotal.fetchAndAdd(msgsSize);
			break;
		case SUSPEND_CURRENT_QUEUE_A_MOMENT:
			consumeRequest.getProcessQueue()->makeMessageToCosumeAgain(*pMsgs);
			submitConsumeRequestLater(consumeRequest.getProcessQueue(),
				consumeRequest.getMessageQueue(),
				context.suspendCurrentQueueTimeMillis);
			continueConsume = false;
			// ͳ����Ϣ
			getConsumerStat().consumeMsgFailedTotal.fetchAndAdd(msgsSize);
			break;
		default:
			break;
		}
	}

	if (commitOffset >= 0)
	{
		m_pDefaultMQPushConsumerImpl->getOffsetStore()->updateOffset(*consumeRequest.getMessageQueue(),
			commitOffset, false);
	}

	return continueConsume;
}

MessageQueueLock& ConsumeMessageOrderlyService::getMessageQueueLock()
{
	return m_messageQueueLock;
}

DefaultMQPushConsumer* ConsumeMessageOrderlyService::getDefaultMQPushConsumer()
{
	return m_pDefaultMQPushConsumer;
}

ConsumeOrderlyRequest::ConsumeOrderlyRequest(ProcessQueue* pProcessQueue,
											 MessageQueue* pMessageQueue,
											 ConsumeMessageOrderlyService* pService)
	:m_pProcessQueue(pProcessQueue),
	m_pMessageQueue(pMessageQueue),
	m_pService(pService)
{

}

ConsumeOrderlyRequest::~ConsumeOrderlyRequest()
{

}

void ConsumeOrderlyRequest::Do()
{
	// ��֤�ڵ�ǰConsumer�ڣ�ͬһ���д�������
	kpr::Mutex* objLock = m_pService->getMessageQueueLock().fetchLockObject(*m_pMessageQueue);
	{
		kpr::ScopedLock<kpr::Mutex> lock(*objLock);

        MqLogVerb("ConsumeOrderlyRequest::Do(),topic=%s,broker=%s--%d",
            m_pMessageQueue->getTopic().c_str(),m_pMessageQueue->getBrokerName().c_str(),
            m_pMessageQueue->getQueueId());

		// ��֤��Consumer��Ⱥ��ͬһ���д�������
        /* modified by yu.guangjie at 2015-08-26, reason: change || to && */
		/** mdy by lin.qs, 2016-12-30, bug-fix 
			�����߼�Ҫ���ּ�Ⱥ���Ѻ͹㲥���ѡ��㲥���Ѳ���Ҫ������
		*/
		if (m_pService->getDefaultMQPushConsumer()->getMessageModel() == BROADCASTING  
			|| (m_pProcessQueue->isLocked() && !m_pProcessQueue->isLockExpired())
			)
		{
			long long beginTime = GetCurrentTimeMillis();
			for (bool continueConsume = true; continueConsume;)
			{
				if (m_pProcessQueue->isDroped())
				{
					MqLogWarn("the message queue not be able to consume, because it's droped: "
                        "[topic=%s, broker=%s, queue=%d]",
					    m_pMessageQueue->getTopic().c_str(),
					    m_pMessageQueue->getBrokerName().c_str(),
					    m_pMessageQueue->getQueueId());
					break;
				}
				//mdy by lin.qs, 2016-12-30, ��Ⱥģʽ����Ҫ���������ѣ��㲥ģʽ����Ҫ
				if (m_pService->getDefaultMQPushConsumer()->getMessageModel() == CLUSTERING 
					&& !m_pProcessQueue->isLocked())
				{
					MqLogWarn("the message queue not locked, so consume later: "
                        "[topic=%s, broker=%s, queue=%d]", 
                        m_pMessageQueue->getTopic().c_str(),
					    m_pMessageQueue->getBrokerName().c_str(),
					    m_pMessageQueue->getQueueId());
					m_pService->tryLockLaterAndReconsume(m_pMessageQueue, m_pProcessQueue, 10);
					break;
				}
				//mdy by lin.qs, 2016-12-30, ��Ⱥģʽ����Ҫ���������ѣ��㲥ģʽ����Ҫ
				if (m_pService->getDefaultMQPushConsumer()->getMessageModel() == CLUSTERING 
					&& m_pProcessQueue->isLockExpired())
				{
					MqLogWarn("the message queue lock expired, so consume later: "
                        "[topic=%s, broker=%s, queue=%d]", 
                        m_pMessageQueue->getTopic().c_str(),
					    m_pMessageQueue->getBrokerName().c_str(),
					    m_pMessageQueue->getQueueId());
					m_pService->tryLockLaterAndReconsume(m_pMessageQueue, m_pProcessQueue, 10);
					break;
				}

				// ���߳���С�ڶ���������£���ֹ������б�����
				long interval = long (GetCurrentTimeMillis() - beginTime);
				if (interval > ConsumeMessageOrderlyService::s_MaxTimeConsumeContinuously)
				{
					// ��10ms��������
					m_pService->submitConsumeRequestLater(m_pProcessQueue, m_pMessageQueue, 10);
					break;
				}

				int consumeBatchSize = 
					m_pService->getDefaultMQPushConsumer()->getConsumeMessageBatchMaxSize();

				std::list<MessageExt*> msgs = m_pProcessQueue->takeMessags(consumeBatchSize);
				if (!msgs.empty())
				{
					ConsumeOrderlyContext context(*m_pMessageQueue);

					ConsumeOrderlyStatus status = SUSPEND_CURRENT_QUEUE_A_MOMENT;

					// ִ��Hook
					ConsumeMessageContext consumeMessageContext;
					if (m_pService->getDefaultMQPushConsumerImpl()->hasHook())
					{
						consumeMessageContext.consumerGroup = m_pService->getConsumerGroup();
						consumeMessageContext.mq = *m_pMessageQueue;
						consumeMessageContext.msgList = msgs;
						consumeMessageContext.success = false;
						m_pService->getDefaultMQPushConsumerImpl()->executeHookBefore(consumeMessageContext);
					}

					long long beginTimestamp = GetCurrentTimeMillis();

					try
					{
						status = m_pService->getMessageListener()->consumeMessage(msgs, context);
					}
					catch (...)
					{
						MqLogWarn("consumeMessage exception: Group: {%s} "
                            "MQ: {topic=%s, broker=%s, queue=%d}",
                            m_pService->getConsumerGroup().c_str(),
                            m_pMessageQueue->getTopic().c_str(),
    					    m_pMessageQueue->getBrokerName().c_str(),
    					    m_pMessageQueue->getQueueId());
                        status = SUSPEND_CURRENT_QUEUE_A_MOMENT;
					}

					long consumeRT = long (GetCurrentTimeMillis() - beginTimestamp);


					// ִ��Hook
					if (m_pService->getDefaultMQPushConsumerImpl()->hasHook())
					{
						consumeMessageContext.success = (SUCCESS == status
														|| COMMIT == status);
						m_pService->getDefaultMQPushConsumerImpl()->executeHookAfter(consumeMessageContext);
					}

					// ��¼ͳ����Ϣ
					m_pService->getConsumerStat().consumeMsgRTTotal.fetchAndAdd(consumeRT);
					MixAll::compareAndIncreaseOnly(m_pService->getConsumerStat()
						.consumeMsgRTMax, consumeRT);

					continueConsume = m_pService->processConsumeResult(&msgs, status, context, *this);
                    /* modified by yu.guangjie at 2015-08-26, reason: delete MessageExt* */
                    if(continueConsume)
                    {
                        std::list<MessageExt*>::iterator it = msgs.begin();
                        for (; it != msgs.end(); it++)
                        {
                            MqLogVerb("ConsumeOrderlyRequest: delete msg offset=%lld", (*it)->getQueueOffset());
                        	delete *it;
                        }
                        msgs.clear();
                    }
				}
				else
				{
				    MqLogVerb("takeMessags() empty: topic=%s,broker=%s--%d",
                        m_pMessageQueue->getTopic().c_str(),m_pMessageQueue->getBrokerName().c_str(),
                        m_pMessageQueue->getQueueId());
					continueConsume = false;
				}
			}
		}
		// û���õ���ǰ���е������Ժ�������
		else
		{
		    MqLogVerb("tryLockLaterAndReconsume() : topic=%s,broker=%s--%d",
                m_pMessageQueue->getTopic().c_str(),m_pMessageQueue->getBrokerName().c_str(),
                m_pMessageQueue->getQueueId());
			m_pService->tryLockLaterAndReconsume(m_pMessageQueue, m_pProcessQueue, 10);

		}
	}

	delete this;
}
