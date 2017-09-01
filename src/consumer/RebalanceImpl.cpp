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

#include "RebalanceImpl.h"
#include "AllocateMessageQueueStrategy.h"
#include "MQClientFactory.h"
#include "MixAll.h"
#include "LockBatchBody.h"
#include "MQClientAPIImpl.h"
#include "KPRUtil.h"
#include "ScopedLock.h"
#include "UtilAll.h"
#include "MQClientException.h"

RebalanceImpl::RebalanceImpl(const std::string& consumerGroup, 
	MessageModel messageModel,
	AllocateMessageQueueStrategy* pAllocateMessageQueueStrategy, 
	MQClientFactory* pMQClientFactory)
	:m_consumerGroup(consumerGroup),
	m_messageModel (messageModel),
	m_pAllocateMessageQueueStrategy (pAllocateMessageQueueStrategy),
	m_pMQClientFactory( pMQClientFactory)
{

}

RebalanceImpl::~RebalanceImpl()
{
}

void RebalanceImpl::unlock(MessageQueue& mq, bool oneway)
{
	FindBrokerResult findBrokerResult =
		m_pMQClientFactory->findBrokerAddressInSubscribe(mq.getBrokerName(), MixAll::MASTER_ID, true);
	if (!findBrokerResult.brokerAddr.empty())
	{
		UnlockBatchRequestBody* requestBody = new UnlockBatchRequestBody();
		requestBody->setConsumerGroup(m_consumerGroup);
		requestBody->setClientId(m_pMQClientFactory->getClientId());
		requestBody->getMqSet().insert(mq);

		try
		{
			m_pMQClientFactory->getMQClientAPIImpl()->unlockBatchMQ(findBrokerResult.brokerAddr,
				requestBody, 1000, oneway);
		}
		catch (...)
		{
			// log.error("unlockBatchMQ exception, " + mq, e);
		}

        //add by lin.qs@2017-4-6，要删除 new 的指针对象
        delete requestBody;
        requestBody = NULL;
	}
}

void RebalanceImpl::unlockAll(bool oneway)
{
	std::map<std::string, std::set<MessageQueue> > brokerMqs = buildProcessQueueTableByBrokerName();
	std::map<std::string, std::set<MessageQueue> >::iterator it = brokerMqs.begin();

	for (; it != brokerMqs.end(); it++)
	{
		std::string brokerName = it->first;
		std::set<MessageQueue> mqs = it->second;

		if (mqs.empty())
		{
			continue;
		}

		FindBrokerResult findBrokerResult =
			m_pMQClientFactory->findBrokerAddressInSubscribe(brokerName, MixAll::MASTER_ID, true);

		if (!findBrokerResult.brokerAddr.empty())
		{
			UnlockBatchRequestBody* requestBody = new UnlockBatchRequestBody();
			requestBody->setConsumerGroup(m_consumerGroup);
			requestBody->setClientId(m_pMQClientFactory->getClientId());
			requestBody->setMqSet(mqs);

			try
			{
				m_pMQClientFactory->getMQClientAPIImpl()->unlockBatchMQ(findBrokerResult.brokerAddr,
					requestBody, 1000, oneway);
				std::set<MessageQueue>::iterator itm = mqs.begin();
				for (;itm!=mqs.end();itm++)
				{
					std::map<MessageQueue, ProcessQueue*>::iterator itp = m_processQueueTable.find(*itm);
					if (itp != m_processQueueTable.end())
					{
						itp->second->setLocked(false);
						//TODO log.info("the message queue unlock OK, Group: {} {}", this.consumerGroup, mq);
					}
				}
			}
			catch (...)
			{
				//TODO log.error("unlockBatchMQ exception, " + mqs, e);
			}

            //add by lin.qs@2017-4-6，要删除 new 的指针对象
            delete requestBody;
            requestBody = NULL;
		}
	}
}

bool RebalanceImpl::lock(MessageQueue& mq)
{
	FindBrokerResult findBrokerResult =
		m_pMQClientFactory->findBrokerAddressInSubscribe(mq.getBrokerName(), MixAll::MASTER_ID, true);
	if (!findBrokerResult.brokerAddr.empty())
	{
        //mdy by lin.qs@2017-4-6，指针未删除，内存泄漏；修改为局部变量的形式
#if 0
		LockBatchRequestBody* requestBody = new LockBatchRequestBody();
		requestBody->setConsumerGroup(m_consumerGroup);
		requestBody->setClientId(m_pMQClientFactory->getClientId());
		requestBody->getMqSet().insert(mq);
#else
        LockBatchRequestBody requestBody;
        requestBody.setConsumerGroup(m_consumerGroup);
        requestBody.setClientId(m_pMQClientFactory->getClientId());
        requestBody.getMqSet().insert(mq);
#endif

		try
		{
			std::set<MessageQueue> lockedMq =
				m_pMQClientFactory->getMQClientAPIImpl()->lockBatchMQ(
				findBrokerResult.brokerAddr, &requestBody, 1000);

			std::set<MessageQueue>::iterator it = lockedMq.begin();
			for (; it != lockedMq.end(); it++)
			{
				MessageQueue mmqq = *it;
				std::map<MessageQueue, ProcessQueue*>::iterator itt = m_processQueueTable.find(mmqq);
				if (itt != m_processQueueTable.end())
				{
					itt->second->setLocked(true);
					itt->second->setLastLockTimestamp(GetCurrentTimeMillis());
				}
			}

			it = lockedMq.find(mq);
			if (it != lockedMq.end())
			{
				return true;
			}

			//TODO log.info("the message queue lock {}, {} {}",//
			//	(lockOK ? "OK" : "Failed"), //
			//	this.consumerGroup, //
			//	mq);
			return false;
		}
		catch (...)
		{
			//TODO log.error("lockBatchMQ exception, " + mq, e);
		}
	}

	return false;
}

void RebalanceImpl::lockAll()
{
	std::map<std::string, std::set<MessageQueue> > brokerMqs = buildProcessQueueTableByBrokerName();

	std::map<std::string, std::set<MessageQueue> >::iterator it = brokerMqs.begin();
	for (;it != brokerMqs.end();it++)
	{
		std::string brokerName = it->first;
		std::set<MessageQueue> mqs = it->second;

		if (mqs.empty())
		{
			continue;
		}

		FindBrokerResult findBrokerResult =
			m_pMQClientFactory->findBrokerAddressInSubscribe(brokerName, MixAll::MASTER_ID, true);
		if (!findBrokerResult.brokerAddr.empty())
		{
			LockBatchRequestBody* requestBody = new LockBatchRequestBody();
			requestBody->setConsumerGroup(m_consumerGroup);
			requestBody->setClientId(m_pMQClientFactory->getClientId());
			requestBody->setMqSet(mqs);

			try
			{
				std::set<MessageQueue> lockOKMQSet =
					m_pMQClientFactory->getMQClientAPIImpl()->lockBatchMQ(
					findBrokerResult.brokerAddr, requestBody, 1000);

				// 锁定成功的队列
				std::set<MessageQueue>::iterator its = lockOKMQSet.begin();
				for (;its != lockOKMQSet.end();its++)
				{
					MessageQueue mq = *its;
					std::map<MessageQueue, ProcessQueue*>::iterator itt = m_processQueueTable.find(mq);
					if (itt != m_processQueueTable.end())
					{
						ProcessQueue* processQueue = itt->second;
						if (!processQueue->isLocked())
						{
							//TODO log.info("the message queue locked OK, Group: {} {}", this.consumerGroup, mq);
						}

						processQueue->setLocked(true);
						processQueue->setLastLockTimestamp(GetCurrentTimeMillis());
					}
				}

				// 锁定失败的队列
				its = mqs.begin();
				for (;its != mqs.end();its++)
				{
					MessageQueue mq = *its;
					std::set<MessageQueue>::iterator itf = lockOKMQSet.find(mq);
					if (itf == lockOKMQSet.end())
					{
						std::map<MessageQueue, ProcessQueue*>::iterator itt = m_processQueueTable.find(mq);
						if (itt != m_processQueueTable.end())
						{
							itt->second->setLocked(false);
							//TODO log.warn("the message queue locked Failed, Group: {} {}", this.consumerGroup,
							//	mq);
						}
					}
				}
			}
            /* modified by yu.guangjie at 2015-11-04, reason: add exception log*/
            catch (MQClientException& e)
            {
                MqLogWarn("MQClientException:: %s!", e.what());  
            }
			catch (...)
			{
				MqLogWarn("RebalanceImpl::lockAll unkown exception!");  
			}
            delete requestBody;
            requestBody = NULL;
		}
	}
}

void RebalanceImpl::doRebalance()
{
	std::map<std::string, SubscriptionData> subTable = getSubscriptionInner();
	std::map<std::string, SubscriptionData>::iterator it = subTable.begin();

	//mjx modify add
	m_downBrokerName.clear();
	
	for (; it != subTable.end(); it++)
	{
		std::string topic = it->first;
		try
		{
			rebalanceByTopic(topic);
		}
		catch (...)
		{
			if (topic.find(MixAll::RETRY_GROUP_TOPIC_PREFIX) != 0)
			{
				MqLogWarn("rebalanceByTopic(%s) Exception", topic.c_str());
			}
		}
	}

	truncateMessageQueueNotMyTopic();
}

std::map<std::string, SubscriptionData> RebalanceImpl::getSubscriptionInner()
{
    /* modified by yu.guangjie at 2015-10-13, reason: */
    std::map<std::string, SubscriptionData> mapSubData;
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_subsMutex);
        mapSubData = m_subscriptionInner;
    }    
	return mapSubData;
}

void RebalanceImpl::subscribe(std::string topic, SubscriptionData& subData)
{
    kpr::ScopedLock<kpr::Mutex> lock(m_subsMutex);
    m_subscriptionInner[topic] = subData;
}

void RebalanceImpl::unsubscribe(std::string topic)
{
    kpr::ScopedLock<kpr::Mutex> lock(m_subsMutex);
    m_subscriptionInner.erase(topic);
}

bool RebalanceImpl::hasSubscribe(std::string topic, SubscriptionData *psubData)
{
    bool bHasFlag = true;
    
    kpr::ScopedLock<kpr::Mutex> lock(m_subsMutex);
    std::map<std::string, SubscriptionData>::iterator it = m_subscriptionInner.find(topic);
    if(it != m_subscriptionInner.end())
    {
        bHasFlag = true;
        if(psubData != NULL)
        {
            *psubData = it->second;
        }
    }
    else
    {
        bHasFlag = false;
    }	

    return bHasFlag;
}

std::map<MessageQueue, ProcessQueue*>& RebalanceImpl::getProcessQueueTable()
{
	return m_processQueueTable;
}

std::map<std::string, std::set<MessageQueue> >& RebalanceImpl::getTopicSubscribeInfoTable()
{
	return m_topicSubscribeInfoTable;
}

std::string& RebalanceImpl::getConsumerGroup()
{
	return m_consumerGroup;
}

void RebalanceImpl::setConsumerGroup(const std::string& consumerGroup)
{
	m_consumerGroup = consumerGroup;
}

MessageModel RebalanceImpl::getMessageModel()
{
	return m_messageModel;
}

void RebalanceImpl::setMessageModel(MessageModel messageModel)
{
	m_messageModel = messageModel;
}

AllocateMessageQueueStrategy* RebalanceImpl::getAllocateMessageQueueStrategy()
{
	return m_pAllocateMessageQueueStrategy;
}

void RebalanceImpl::setAllocateMessageQueueStrategy(AllocateMessageQueueStrategy* pAllocateMessageQueueStrategy)
{
	m_pAllocateMessageQueueStrategy = pAllocateMessageQueueStrategy;
}

MQClientFactory* RebalanceImpl::getmQClientFactory()
{
	return m_pMQClientFactory;
}

void RebalanceImpl::setmQClientFactory(MQClientFactory* pMQClientFactory)
{
	m_pMQClientFactory = pMQClientFactory;
}

std::map<std::string, std::set<MessageQueue> > RebalanceImpl::buildProcessQueueTableByBrokerName()
{
	std::map<std::string, std::set<MessageQueue> > result ;
	std::map<MessageQueue, ProcessQueue*>::iterator it = m_processQueueTable.begin();

    /* modified by yu.guangjie at 2015-08-27, reason: add ++it */
	for ( ; it != m_processQueueTable.end(); ++it)
	{
		MessageQueue mq = it->first;
		std::map<std::string, std::set<MessageQueue> >::iterator itm = result.find(mq.getBrokerName());
		if (itm == result.end())
		{
			std::set<MessageQueue> mqs ;
			mqs.insert(mq);
			result[mq.getBrokerName()] = mqs;
		}
		else
		{
			itm->second.insert(mq);
		}
	}

	return result;
}

void RebalanceImpl::rebalanceByTopic(const std::string& topic)
{
    MqLogDebug("rebalanceByTopic:: start to doRebalance consumerGroup[%s], topic[%s].", 
        m_consumerGroup.c_str(), topic.c_str());

	//mjx modify add
	std::string brokerName = "";
	std:: string brokerAddr = "";
	//static bool bFind = true;
	
	switch (m_messageModel)
	{
	case BROADCASTING:
		{
			std::map<std::string, std::set<MessageQueue> >::iterator it = m_topicSubscribeInfoTable.find(topic);

			if (it != m_topicSubscribeInfoTable.end())
			{
				std::set<MessageQueue> mqSet = it->second;
				bool changed = updateProcessQueueTableInRebalance(topic, mqSet);
				if (changed)
				{
					messageQueueChanged(topic, mqSet, mqSet);
					//TODO log.info("messageQueueChanged {} {} {} {}",//
					//	consumerGroup,//
					//	topic,//
					//	mqSet,//
					//	mqSet);
				}
			}
			else
			{
				//TODO log.warn("doRebalance, {}, but the topic[{}] not exist.", consumerGroup, topic);
			}
			break;
		}
	case CLUSTERING:
		{
			std::map<std::string, std::set<MessageQueue> >::iterator it = m_topicSubscribeInfoTable.find(topic);

			if (it == m_topicSubscribeInfoTable.end())
			{
				if (topic.find(MixAll::RETRY_GROUP_TOPIC_PREFIX) != 0 )
				{
					MqLogWarn("doRebalance consumerGroup[%s], but the topic[%s] not exist.", 
                        m_consumerGroup.c_str(), topic.c_str());
				}
			}

			std::list<std::string> cidAll = m_pMQClientFactory->findConsumerIdList(topic, m_consumerGroup);

			if (cidAll.empty())
			{
				MqLogWarn("doRebalance[consumerGroup=%s, topic=%s]: get consumer id list failed!", 
                    m_consumerGroup.c_str(), topic.c_str());
				
				
			}

			if (it != m_topicSubscribeInfoTable.end() && !cidAll.empty())
			{

				
				std::vector<MessageQueue> mqAll;
				std::set<MessageQueue> mqSet = it->second;
				std::set<MessageQueue>::iterator its = mqSet.begin();

				

				
				//set 本身已经排序
				for (; its != mqSet.end();its++)
				{
					mqAll.push_back(*its);
				}

				// 排序
				cidAll.sort();

				AllocateMessageQueueStrategy* strategy = m_pAllocateMessageQueueStrategy;

				// 执行分配算法
				std::vector<MessageQueue>* allocateResult;
				try
				{
					
					allocateResult = strategy->allocate(m_pMQClientFactory->getClientId(), mqAll, cidAll);
				}
				catch (MQException& e)
				{
					MqLogError("AllocateMessageQueueStrategy.allocate Exception:%s", e.what());
				}

				std::set<MessageQueue> allocateResultSet;
				if (allocateResult != NULL)
				{

					for(size_t i=0;i<allocateResult->size();i++)
					{
						allocateResultSet.insert(allocateResult->at(i));                        
					}

					delete allocateResult;
				}

				// 更新本地队列
				bool changed = updateProcessQueueTableInRebalance(topic, allocateResultSet);

				
				if (changed)
				{
					
					messageQueueChanged(topic, mqSet, allocateResultSet);

                    std::stringstream ssMqs;
					std::set<MessageQueue>::iterator itMq = allocateResultSet.begin();
                    for (; itMq != allocateResultSet.end(); itMq++)
    				{
    				    if(itMq != allocateResultSet.begin())
                        {
                            ssMqs<<",";
                        }
                        ssMqs<<itMq->getBrokerName()<<"["<<itMq->getQueueId()<<"]";
    				}
                    MqLogNotice("Message queue changed[topic:%s, consumerId:%s], MESSAGE_QUEUE:\n{%s}", 
                        m_pMQClientFactory->getClientId().c_str(), topic.c_str(), ssMqs.str().c_str());

					std::stringstream ssCids;
					std::list<std::string>::iterator itCid = cidAll.begin();
                    for (; itCid != cidAll.end(); itCid++)
    				{
    				    if(itCid != cidAll.begin())
                        {
                           ssCids<<","; 
                        }
                        ssCids<<*itCid;
    				}
					
                    MqLogNotice("Message queue changed[topic:%s], CONSUMER_LIST:\n{%s}", 
                        topic.c_str(), ssCids.str().c_str());
				}
			}
			break;
		}
	default:
		break;
	}
}

bool RebalanceImpl::updateProcessQueueTableInRebalance(const std::string& topic, std::set<MessageQueue>& mqSet)
{
	bool changed = false;

	// 将多余的队列删除
	//mjx modify add
	//比如双主0备，1主挂了??
	std:: string masterAddr = "";
	std::map<MessageQueue, ProcessQueue*>::iterator it = m_processQueueTable.begin();

	while(it != m_processQueueTable.end())
	{
		MessageQueue mq = it->first;
		if (mq.getTopic() == topic)
		{
			std::set<MessageQueue>::iterator its = mqSet.find(mq);
			//mjx modify add
			//该消息队列已经不存在了，从处理列表中删除关于该消息队列的所有信息
			
			if (its == mqSet.end())
			{
				changed = true;
				ProcessQueue* pq = it->second;
				if (pq != NULL)
				{
					pq->setDroped(true);
					removeUnnecessaryMessageQueue(mq, *pq);					
				}
                MqLogNotice("doRebalance[topic=%s], remove unnecessary mq[broker=%s,queue=%d]",
                    topic.c_str(), mq.getBrokerName().c_str(), mq.getQueueId());
				
				m_processQueueTable.erase(it++);
			}

			
			else
			{
				//mjx modify add start
				masterAddr = m_pMQClientFactory->findMasterBrokerAddrByTopicAndName(mq.getTopic(),mq.getBrokerName());
				std::map<std::string,std::string>::iterator itor = m_brokerMaster.find(mq.getBrokerName()) ;
				if(itor !=  m_brokerMaster.end())
				{
					int i = 0;
					
					if(itor->second != masterAddr)  //主备发生了切换，以前存储的broker master地址跟现在不同了
					{
						for(i=0; i<m_downBrokerName.size(); i++)
						if(mq.getBrokerName() == m_downBrokerName[i])
							break;
					
						if(i == m_downBrokerName.size() )
						{
							m_downBrokerName.push_back(mq.getBrokerName());
							MqLogNotice("doRebalance: broker master %s is down,slave change to master",
		                    		mq.getBrokerName().c_str());
						}

					}

					
					for(i=0; i<m_downBrokerName.size(); i++)
						if(mq.getBrokerName() == m_downBrokerName[i])
							break;
					
					if(i < m_downBrokerName.size() )
						
					{	//该messagequeue删除，重新从切过的master上面重新获取下
						changed = true;
						ProcessQueue* pq = it->second;
						if (pq != NULL)
						{
							pq->setDroped(true);
							removeUnnecessaryMessageQueue(mq, *pq);					
						}

						
		                MqLogNotice("doRebalance:topic:%s,broker:%s,master and slave changed,before addr:%s,now addr:%s ",
		                    topic.c_str(), mq.getBrokerName().c_str(),itor->second.c_str(),masterAddr.c_str() );
						
		                MqLogNotice("doRebalance[topic=%s], master and slave changed, get message queue from new master[broker=%s,queue=%d]",
		                    topic.c_str(), mq.getBrokerName().c_str(), mq.getQueueId());
						
						m_processQueueTable.erase(it++);

						continue;

					}
				}

				//mjx modify add start add end
					
				it++;
			}
		}
		else
		{
			it++;
		}
	}

	// 增加新增的队列
	std::list<PullRequest*> pullRequestList;

	std::set<MessageQueue>::iterator its = mqSet.begin();
	for (; its != mqSet.end(); its++)
	{
		MessageQueue mq = *its;
		std::map<MessageQueue, ProcessQueue*>::iterator itm = m_processQueueTable.find(mq);

		if (itm == m_processQueueTable.end())
		{
			PullRequest* pullRequest = new PullRequest();
			pullRequest->setConsumerGroup(m_consumerGroup);
			pullRequest->setMessageQueue(new MessageQueue(mq.getTopic(),mq.getBrokerName(),mq.getQueueId()));
			pullRequest->setProcessQueue(new ProcessQueue());

			// 这个需要根据策略来设置
			long long nextOffset = computePullFromWhere(mq);
			
			if (nextOffset >= 0)
			{
				pullRequest->setNextOffset(nextOffset);
				pullRequestList.push_back(pullRequest);
				changed = true;
				m_processQueueTable[mq] = pullRequest->getProcessQueue();

				//mjx modify add start
				masterAddr = m_pMQClientFactory->findMasterBrokerAddrByTopicAndName(mq.getTopic(),mq.getBrokerName());

				if(masterAddr != "")
					m_brokerMaster[mq.getBrokerName()] = masterAddr;
				
				MqLogNotice("doRebalance[topic=%s], broker=%s,ipaddr=%s",
                    topic.c_str(), mq.getBrokerName().c_str(),masterAddr.c_str());
				
				//mjx modify add end
					
				MqLogNotice("doRebalance[topic=%s], add a new mq[broker=%s,queue=%d]",
                    topic.c_str(), mq.getBrokerName().c_str(), mq.getQueueId());
			}
			else
			{
				// 等待此次Rebalance做重试
				delete pullRequest;
				MqLogWarn("doRebalance[topic=%s], add a new mq failed[broker=%s,queue=%d]: can't get offset!",
                    topic.c_str(), mq.getBrokerName().c_str(), mq.getQueueId());
			}
		}
	}

	dispatchPullRequest(pullRequestList);

	return changed;
}

void RebalanceImpl::truncateMessageQueueNotMyTopic()
{
	std::map<std::string, SubscriptionData> subTable = getSubscriptionInner();
	std::map<MessageQueue, ProcessQueue*>::iterator it = m_processQueueTable.begin();

    /* modified by yu.guangjie at 2015-08-20, reason: use while loop */
    //for ( ; it != m_processQueueTable.end(); it++)
	while(it != m_processQueueTable.end())
	{
		MessageQueue mq = it->first;
		std::map<std::string, SubscriptionData>::iterator itt = subTable.find(mq.getTopic());

		if (itt == subTable.end())
		{
			ProcessQueue* pq = it->second;
			if (pq != NULL)
			{
				pq->setDroped(true);
				//TODO log.info("doRebalance, {}, truncateMessageQueueNotMyTopic remove unnecessary mq, {}",
				//	consumerGroup, mq);
			}
			m_processQueueTable.erase(it++);
		}
		else
		{
			it++;
		}
	}
}

void RebalanceImpl::removeProcessQueue(MessageQueue& mq)
{
    std::map<MessageQueue, ProcessQueue*>::iterator itt = m_processQueueTable.find(mq);
    if (itt != m_processQueueTable.end())
    {
        ProcessQueue* processQueue = itt->second;
        m_processQueueTable.erase(itt);
        processQueue->setDroped(true);

        removeUnnecessaryMessageQueue(mq, *processQueue);
        MqLogVerb("Fix Offset, remove unnecessary mq,  Droped: \n");
    }
}
