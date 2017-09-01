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

#include "MQClientFactory.h"

#include <math.h>
#include <set>
#include <string>
#include <vector>

#include "RemoteClientConfig.h"
#include "ClientRemotingProcessor.h"
#include "MQClientAPIImpl.h"
#include "MQAdminImpl.h"
#include "DefaultMQProducer.h"
#include "PullMessageService.h"
#include "RebalanceService.h"
#include "ScopedLock.h"
#include "KPRUtil.h"
#include "DefaultMQProducerImpl.h"
#include "DefaultMQPushConsumerImpl.h"
#include "MQClientException.h"
#include "MQConsumerInner.h"
#include "MQProducerInner.h"
#include "UtilAll.h"
#include "PermName.h"
#include "MQClientManager.h"
#include "ConsumerStatManage.h"
#include "TopicPublishInfo.h"

long MQClientFactory::LockTimeoutMillis = 3000;

MQClientFactory::MQClientFactory(ClientConfig& clientConfig, int factoryIndex, const std::string& clientId)
{
	m_tcpTimeoutMillseconds = MixAll::DEFAULT_TCP_TIMEOUT_MILLISECONDS;

	m_clientConfig = clientConfig;
	m_factoryIndex = factoryIndex;
	m_pRemoteClientConfig = new RemoteClientConfig();
	m_pRemoteClientConfig->clientCallbackExecutorThreads = clientConfig.getClientCallbackExecutorThreads();
	m_pClientRemotingProcessor = new ClientRemotingProcessor(this);
	m_pMQClientAPIImpl = new MQClientAPIImpl(*m_pRemoteClientConfig, m_pClientRemotingProcessor);

	if (!m_clientConfig.getNamesrvAddr().empty())
	{
		m_pMQClientAPIImpl->updateNameServerAddressList(m_clientConfig.getNamesrvAddr());
	}

	m_clientId = clientId;

	m_pMQAdminImpl = new MQAdminImpl(this);
	m_pPullMessageService = new PullMessageService(this);
	m_pRebalanceService = new RebalanceService(this);
	m_pDefaultMQProducer = new DefaultMQProducer(MixAll::CLIENT_INNER_PRODUCER_GROUP);
	m_pDefaultMQProducer->resetClientConfig(clientConfig);
	m_bootTimestamp = GetCurrentTimeMillis();

	m_pFetchNameServerAddr = new ScheduledTask(this,&MQClientFactory::fetchNameServerAddr);
	m_pUpdateTopicRouteInfoFromNameServerTask = 
		new ScheduledTask(this,&MQClientFactory::updateTopicRouteInfoFromNameServerTask);
	m_pCleanBroker = new ScheduledTask(this,&MQClientFactory::cleanBroker);
	m_pPersistAllConsumerOffsetTask = new ScheduledTask(this,&MQClientFactory::persistAllConsumerOffsetTask);
	m_pRecordSnapshotPeriodicallyTask = new ScheduledTask(this,&MQClientFactory::recordSnapshotPeriodicallyTask);
	m_pLogStatsPeriodicallyTask = new ScheduledTask(this,&MQClientFactory::logStatsPeriodicallyTask);

	m_serviceState = CREATE_JUST;
}

MQClientFactory::~MQClientFactory()
{
	delete m_pRemoteClientConfig;
	delete m_pClientRemotingProcessor;
	delete m_pMQClientAPIImpl;
	delete m_pMQAdminImpl;
	delete m_pPullMessageService;
	delete m_pRebalanceService;
	delete m_pDefaultMQProducer;
	delete m_pFetchNameServerAddr;
	delete m_pUpdateTopicRouteInfoFromNameServerTask;
	delete m_pCleanBroker;
	delete m_pPersistAllConsumerOffsetTask;
	delete m_pRecordSnapshotPeriodicallyTask;
	delete m_pLogStatsPeriodicallyTask;
}

void MQClientFactory::start()
{
	kpr::ScopedLock<kpr::Mutex> lock(m_mutex);
	switch (m_serviceState)
	{
	case CREATE_JUST:
		makesureInstanceNameIsOnly(m_clientConfig.getInstanceName());

		m_serviceState = START_FAILED;
		if (m_clientConfig.getNamesrvAddr().empty())
		{
			m_clientConfig.setNamesrvAddr(m_pMQClientAPIImpl->fetchNameServerAddr());
		}

		m_pMQClientAPIImpl->start();
		m_timerTaskManager.Init(5,1000);
		startScheduledTask();
		m_pPullMessageService->Start();
		m_pRebalanceService->Start();

		m_pDefaultMQProducer->getDefaultMQProducerImpl()->start(false);
		

		m_serviceState = RUNNING;
		break;
	case RUNNING:
		break;
	case SHUTDOWN_ALREADY:
		break;
	case START_FAILED:
		THROW_MQEXCEPTION(MQClientException,"The Factory object start failed",-1);
	default:
		break;
	}
}

void MQClientFactory::sendHeartbeatToAllBrokerWithLock()
{
	if (m_lockHeartbeat.TryLock())
	{
		try
		{
			sendHeartbeatToAllBroker();
		}
		catch (...)
		{

		}
		m_lockHeartbeat.Unlock();
	}
	else
	{
		
	}
}

//mjx cunsumer_table_lock bug fix
void MQClientFactory::updateTopicRouteInfoFromNameServer(bool bLocked)
{
	std::set<std::string> topicList;

	// Consumer对象
	{
		//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
		kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

		std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();

		for (; it!=m_consumerTable.end(); it++)
		{
			MQConsumerInner* inner = it->second;
			std::set<SubscriptionData> subList = inner->subscriptions();
			std::set<SubscriptionData>::iterator it1 = subList.begin();
			for (; it1!=subList.end(); it1++)
			{
				topicList.insert((*it1).getTopic());
			}
		}
	}

	// Producer
	{
		//add by lin.qiongshan, 2016年8月30日, m_producerTable 操作应该加锁
		kpr::ScopedLock<kpr::Mutex> lock(m_producerTableLock);

		std::map<std::string, MQProducerInner*>::iterator it = m_producerTable.begin();

		for (; it!=m_producerTable.end(); it++)
		{
			MQProducerInner* inner = it->second;
			std::set<std::string> pubList = inner->getPublishTopicList();
			topicList.insert(pubList.begin(),pubList.end());
		}
	}

	std::set<std::string>::iterator it2 = topicList.begin();
	for (; it2!=topicList.end(); it2++)
	{
		updateTopicRouteInfoFromNameServer(*it2,bLocked);
	}
}

bool MQClientFactory::updateTopicRouteInfoFromNameServer(const std::string& topic, bool bLocked)
{
	return updateTopicRouteInfoFromNameServer(topic, false, NULL,bLocked);
}

bool MQClientFactory::updateTopicRouteInfoFromNameServer(const std::string& topic,
		bool isDefault,
		DefaultMQProducer* pDefaultMQProducer,bool bLocked)
{
	try
	{
	    MqLogVerb("update topic[%s] routeinfo, isDefault:%s.", 
            topic.c_str(), isDefault?"TRUE":"FALSE");
		if (m_lockNamesrv.Lock(MQClientFactory::LockTimeoutMillis))
		{
			try
			{
				TopicRouteData* topicRouteData;
				if (isDefault && pDefaultMQProducer != NULL)
				{
					topicRouteData =
						m_pMQClientAPIImpl->getDefaultTopicRouteInfoFromNameServer(
							//pDefaultMQProducer->getCreateTopicKey(), 1000 * 3); mdy by lin.qiongshan, 2016-9-2，TCP 操作超时配置化
							pDefaultMQProducer->getCreateTopicKey(), getTcpTimeoutMilliseconds());
					if (topicRouteData != NULL)
					{
						//mdy by lin.qiongshan, 2016-11-22，bug: dataList 要声明为引用（&），否则对其修改不会影响 topicRouteData 内的数据
						std::list<QueueData>& dataList = topicRouteData->getQueueDatas();

						std::list<QueueData>::iterator it= dataList.begin();
						for(; it!=dataList.end(); it++)
						{
							//mdy by lin.qiongshan, 2016-11-22，bug: data 要声明为引用（&），否则对其修改不会影响 topicRouteData 内的数据
							QueueData& data = *it;
							// 读写分区个数是一致，故只做一次判断
							int queueNums =
								std::min<int>(pDefaultMQProducer->getDefaultTopicQueueNums(),
									data.readQueueNums);
							data.readQueueNums = (queueNums);
							data.writeQueueNums = (queueNums);
						}
					}
				}
				else
				{
					topicRouteData =
						m_pMQClientAPIImpl->getTopicRouteInfoFromNameServer(topic, 1000 * 3);
				}

				if (topicRouteData != NULL)
				{
					std::map<std::string, TopicRouteData>::iterator it = m_topicRouteTable.find(topic);
					bool changed = false;

					if (it!=m_topicRouteTable.end())
					{
						changed = topicRouteDataIsChange(it->second, *topicRouteData);
						if (!changed)
						{
							changed = isNeedUpdateTopicRouteInfo(topic);
						}
						else
						{
							//TODO log?
						}
					}
					else
					{
						changed=true;
					}

                    MqLogDebug("update topic[%s] routeinfo, changed:%s.", 
                        topic.c_str(), isDefault?"TRUE":"FALSE");

					if (changed)
					{
						// 后面排序会影响下次的equal逻辑判断，所以先clone一份
						TopicRouteData cloneTopicRouteData = *topicRouteData;

						// 更新Broker地址信息
						std::list<BrokerData> dataList = topicRouteData->getBrokerDatas();

						std::list<BrokerData>::iterator it= dataList.begin();
						for(; it!=dataList.end(); it++)
						{
                            /* modified by yu.guangjie at 2015-11-04, reason: */
                            kpr::ScopedLock<kpr::Mutex> lock(m_brokerAddrTableLock);
							m_brokerAddrTable[(*it).brokerName]=(*it).brokerAddrs;
						}

						// 更新发布队列信息
						{
							//add by lin.qiongshan, 2016年8月30日, m_producerTable 操作应该加锁
							kpr::ScopedLock<kpr::Mutex> lock(m_producerTableLock);

							//[bug] mdy by lin.qiongshan，2016-11-22
							//
							//	在 DefaultMQProducerImpl::updateTopicPublishInfo 方法内部，DefaultMQProducerImpl 对象会将传入的路由信息（TopicPublishInfo）保存到其成员中（m_topicPublishInfoTable）
							//	保存过程中，m_topicPublishInfoTable 保存了传入的 TopicPublishInfo 对象的一份复制
							//	TopicPublishInfo 对象内部保存了队列信息（MessageQueue），是通过 vector 容器保存了一组 MessageQueue 的指针
							//	虽然 m_topicPublishInfoTable 保存的是传入的 TopicPublishInfo 对象的复制，但复制的时候，MessageQueue 的指针也一起复制了过去，而不是创建新的 MessageQueue
							//	这就导致，此处会删除 TopicPublishInfo，而 DefaultMQProducerImpl 的一些流程也会清理 TopicPublishInfo 内部的 MessageQueue 数据（TopicPublishInfo::clearMessageQueue，此处也会删除 MessageQueue 对象），这就导致对同一个指针重复删除，程序 coredump
							//
							//  而且，这里如果有多个 Producer 的话，该 TopicPublishInfo 会复制到多个 Producer 对象中
							//
							//修改方案：
							//	修改 TopicPublishInfo，增加复制构造函数和赋值操作符重载，拷贝 MessageQueue 对象时创建新的 MessageQueue 对象
							//	修改 TopicPublishInfo，增加析构函数，析构时要删除 MessageQueue 指针对象
							TopicPublishInfo* publishInfo =
								topicRouteData2TopicPublishInfo(topic, *topicRouteData);
							std::map<std::string, MQProducerInner*>::iterator it = m_producerTable.begin();
							for(; it!= m_producerTable.end(); it++)
							{
								MQProducerInner* impl = it->second;
								if (impl)
								{
									impl->updateTopicPublishInfo(topic, *publishInfo);
								}
							}
                            delete publishInfo;
						}

						//mjx consumer_table_lock bug fix
						if(!bLocked)
						// 更新订阅队列信息
						{
							//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
							kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

							std::set<MessageQueue>* subscribeInfo =
								topicRouteData2TopicSubscribeInfo(topic, *topicRouteData);
							std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();
							for(; it!= m_consumerTable.end(); it++)
							{
								MQConsumerInner* impl = it->second;
								if (impl)
								{
									impl->updateTopicSubscribeInfo(topic, *subscribeInfo);
								}
							}
                            delete subscribeInfo;
						}

						else
						//mjx consumer_table_lock bug fix
						//外层已经加过锁
						{	

						 // kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);
							std::set<MessageQueue>* subscribeInfo =
								topicRouteData2TopicSubscribeInfo(topic, *topicRouteData);
							std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();
							for(; it!= m_consumerTable.end(); it++)
							{
								MQConsumerInner* impl = it->second;
								if (impl)
								{
									impl->updateTopicSubscribeInfo(topic, *subscribeInfo);
								}
							}
                            delete subscribeInfo;


						}

						m_topicRouteTable[topic]= cloneTopicRouteData;						
					}
                    /* modified by yu.guangjie at 2015-08-28, reason: */
                    delete topicRouteData;	
				}
				else
				{
					//TODO log?
					MqLogWarn("Can't find topic[%s] routeinfo!", topic.c_str());
				}
                m_lockNamesrv.Unlock();
                return true;
			}
			catch (...)
			{
				m_lockNamesrv.Unlock();
				//TODO log?
			}
		}
		else
		{
			//TODO log?
			MqLogWarn("lock Namesrv failed!");
		}
	}
	catch (...)
	{
	    MqLogWarn("updateTopicRouteInfoFromNameServer error!");
		//TODO log?
	}

	return false;
}

TopicPublishInfo*  MQClientFactory::topicRouteData2TopicPublishInfo(const std::string& topic,
		TopicRouteData& route)
{
	TopicPublishInfo* info = new TopicPublishInfo();
	// 顺序消息
	if (!route.getOrderTopicConf().empty())
	{
		std::vector<std::string> brokers;
		UtilAll::Split(brokers,route.getOrderTopicConf(),";");
		for(size_t i=0; i<brokers.size(); i++)
		{
			std::vector<std::string> item;
			UtilAll::Split(item,brokers[i],":");
			int nums =atoi(item[1].c_str());
			for (int i = 0; i < nums; i++)
			{
				MessageQueue* mq = new MessageQueue(topic, item[0], i);
				info->getMessageQueueList().push_back(mq);
			}
		}

		info->setOrderTopic(true);
	}
	// 非顺序消息
	else
	{
		std::list<QueueData> qds = route.getQueueDatas();
		// 排序原因：即使没有配置顺序消息模式，默认队列的顺序同配置的一致。
		qds.sort();
		std::list<QueueData>::iterator it = qds.begin();
		for (; it!=qds.end(); it++)
		{
			QueueData& qd=(*it);
			if (PermName::isWriteable(qd.perm))
			{
				// 这里需要判断BrokerName对应的Master是否存在，因为只能向Master发送消息
				bool find = false;
				BrokerData brokerData;
				std::list<BrokerData> bds = route.getBrokerDatas();
				std::list<BrokerData>::iterator it1 = bds.begin();

				for (; it1!=bds.end(); it1++)
				{
					BrokerData& bd = (*it1);
					if (bd.brokerName==qd.brokerName)
					{
						brokerData = bd;
						find = true;
						break;
					}
				}

				if (!find)
				{
					continue;
				}

				if (brokerData.brokerAddrs.find(MixAll::MASTER_ID)==brokerData.brokerAddrs.end())
				{
					continue;
				}

				for (int i = 0; i < qd.writeQueueNums; i++)
				{
					MessageQueue* mq = new MessageQueue(topic, qd.brokerName, i);
					info->getMessageQueueList().push_back(mq);
				}
			}
		}

		info->setOrderTopic(false);
	}

	return info;
}

std::set<MessageQueue>* MQClientFactory::topicRouteData2TopicSubscribeInfo(const std::string& topic,
		TopicRouteData& route)
{
	std::set<MessageQueue>* mqList = new std::set<MessageQueue>();
	std::list<QueueData> qds = route.getQueueDatas();
	std::list<QueueData>::iterator it = qds.begin();
	for (; it!=qds.end(); it++)
	{
		QueueData& qd=(*it);
		if (PermName::isReadable(qd.perm))
		{
			for (int i = 0; i < qd.readQueueNums; i++)
			{
				mqList->insert(MessageQueue(topic, qd.brokerName, i));
			}
		}
	}

	return mqList;
}


void MQClientFactory::shutdown()
{
	// Consumer
	if (!m_consumerTable.empty())
	{
		return;
	}

	// AdminExt
	if (!m_adminExtTable.empty())
	{
		return;
	}

	// Producer
	if (m_producerTable.size() > 1)
	{
		return;
	}

	{
		kpr::ScopedLock<kpr::Mutex> lock(m_mutex);
		switch (m_serviceState)
		{
		case CREATE_JUST:
			break;
		case RUNNING:
			m_pDefaultMQProducer->getDefaultMQProducerImpl()->shutdown(false);

			for (int i=0;i<6;i++)
			{
				m_timerTaskManager.UnRegisterTimer(m_scheduledTaskIds[i]);
			}

			m_timerTaskManager.Close();
			
			m_pPullMessageService->stop();
			m_pPullMessageService->Join();
			
			m_pMQClientAPIImpl->shutdown();
			m_pRebalanceService->stop();
			m_pRebalanceService->Join();
			
			closesocket(m_datagramSocket);

			MQClientManager::getInstance()->removeClientFactory(m_clientId);
			m_serviceState = SHUTDOWN_ALREADY;
			break;
		case SHUTDOWN_ALREADY:
			break;
		default:
			break;
		}
	}
}

bool MQClientFactory::registerConsumer(const std::string& group, MQConsumerInner* pConsumer)
{
	if (group.empty() || pConsumer==NULL)
	{
		return false;
	}

    kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);
    
	if (m_consumerTable.find(group)!=m_consumerTable.end())
	{
		return false;
	}

	m_consumerTable[group] = pConsumer;

	return true;
}

void MQClientFactory::unregisterConsumer(const std::string& group)
{
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);
        m_consumerTable.erase(group);
    }
	
	unregisterClientWithLock("", group);
}

bool MQClientFactory::registerProducer(const std::string& group, DefaultMQProducerImpl* pProducer)
{
	if (group.empty() || pProducer==NULL)
	{
		return false;
	}

	//add by lin.qiongshan, 2016年8月30日, m_producerTable 操作应该加锁
	kpr::ScopedLock<kpr::Mutex> lock(m_producerTableLock);

	if (m_producerTable.find(group)!=m_producerTable.end())
	{
		return false;
	}

	m_producerTable[group] = pProducer;

	return true;
}

void MQClientFactory::unregisterProducer(const std::string& group)
{
	{
		//add by lin.qiongshan, 2016年8月30日, m_producerTable 操作应该加锁
		kpr::ScopedLock<kpr::Mutex> lock(m_producerTableLock);
		m_producerTable.erase(group);
	}
	
	unregisterClientWithLock(group, "");
}

bool MQClientFactory::registerAdminExt(const std::string& group, MQAdminExtInner* pAdmin)
{
	if (group.empty() || pAdmin==NULL)
	{
		return false;
	}

	if (m_adminExtTable.find(group)!=m_adminExtTable.end())
	{
		return false;
	}

	m_adminExtTable[group] = pAdmin;

	return true;
}

void MQClientFactory::unregisterAdminExt(const std::string& group)
{
	m_adminExtTable.erase(group);
}

void MQClientFactory::rebalanceImmediately()
{
	m_pRebalanceService->wakeup();
}

void MQClientFactory::doRebalance()
{
	//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
	//	Mark by lin.qiongshan, 2016-11-3, 此处不能加锁。在 doRebalance 方法会递归调用到 updateTopicRouteInfoFromNameServer 方法，而该方法也会对该锁对象加锁，导致死锁
	//		保留更底层的加锁，删除此处的加锁
	//mjx consumer_table_lock bug fix
	//不加锁会core，通过函数掉tag避免在同一个线程中嵌套加锁
	//或者通过加个线程的独有全局变量__thread XX控制下
	kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

	std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();

	for (; it!=m_consumerTable.end(); it++)
	{
		MQConsumerInner* impl =it->second;
		
		if (impl != NULL)
		{
			try
			{
				impl->doRebalance();
			}
			catch (...)
			{

			}
		}
	}
}

MQProducerInner* MQClientFactory::selectProducer(const std::string& group)
{
	//add by lin.qiongshan, 2016年8月30日, m_producerTable 操作应该加锁
	kpr::ScopedLock<kpr::Mutex> lock(m_producerTableLock);

	std::map<std::string, MQProducerInner*>::iterator it = m_producerTable.find(group);
	if (it!=m_producerTable.end())
	{
		return it->second;
	}

	return NULL;
}

MQConsumerInner* MQClientFactory::selectConsumer(const std::string& group)
{
	//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
	kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

	std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.find(group);
	if (it!=m_consumerTable.end())
	{
		return it->second;
	}

	return NULL;
}

FindBrokerResult MQClientFactory::findBrokerAddressInAdmin(const std::string& brokerName)
{

    std::string brokerAddr = "";
    bool slave = false;
    bool found = false;

    /* modified by yu.guangjie at 2015-11-04, reason: */
    kpr::ScopedLock<kpr::Mutex> lock(m_brokerAddrTableLock);
    
    std::map<std::string, std::map<int, std::string> >::iterator it = m_brokerAddrTable.find(brokerName);

	if (it != m_brokerAddrTable.end())
	{
		std::map<int, std::string>::iterator it1 = it->second.find(MixAll::MASTER_ID);
		for (; it1 != it->second.end(); it1++)
		{
		    brokerAddr = it1->second;
		    if(brokerAddr != "")
            {
                found = true;
                if (MixAll::MASTER_ID == it1->first) {
                    slave = false;
                }
                else 
                {
                    slave = true;
                }
                break;
            }
		}
	}

    FindBrokerResult result;
    
    if (found) 
    {
        result.brokerAddr = brokerAddr;
        result.slave = slave;
    }
	return result;
}

std::string MQClientFactory::findBrokerAddressInPublish(const std::string& brokerName)
{
    /* modified by yu.guangjie at 2015-11-04, reason: */
    kpr::ScopedLock<kpr::Mutex> lock(m_brokerAddrTableLock);
    
	std::map<std::string, std::map<int, std::string> >::iterator it = m_brokerAddrTable.find(brokerName);

	if (it!=m_brokerAddrTable.end())
	{
		std::map<int, std::string>::iterator it1 = it->second.find(MixAll::MASTER_ID);
		if (it1!=it->second.end())
		{
			return it1->second;
		}
	}

	return "";
}

FindBrokerResult MQClientFactory::findBrokerAddressInSubscribe(const std::string& brokerName,
																long brokerId,
																bool onlyThisBroker)
{
	std::string brokerAddr="";
	bool slave = false;
	bool found = false;

    /* modified by yu.guangjie at 2015-11-04, reason: */
    kpr::ScopedLock<kpr::Mutex> lock(m_brokerAddrTableLock);
    
	std::map<std::string, std::map<int, std::string> >::iterator it = m_brokerAddrTable.find(brokerName);

	if (it!=m_brokerAddrTable.end())
	{
		std::map<int, std::string>::iterator it1 = it->second.find(brokerId);
		if (it1!=it->second.end())
		{
			brokerAddr = it1->second;
			slave = (brokerId != MixAll::MASTER_ID);
			found = true;
		}
		/* modified by yu.guangjie at 2015-08-25, reason: check onlyThisBroker */
		else if(!onlyThisBroker)
		{
			it1 = it->second.begin();
			brokerAddr =it1->second;
			slave = (brokerId != MixAll::MASTER_ID);
			found = true;
		}
	}

	FindBrokerResult result;
	result.brokerAddr = brokerAddr;
	result.slave = slave;

	return result;
}

std::list<std::string> MQClientFactory::findConsumerIdList(const std::string& topic, const std::string& group)
{
	std::string brokerAddr = findBrokerAddrByTopic(topic);

	if (brokerAddr.empty())
	{
		//mjx consumer_table_lock bug fix
		//
		updateTopicRouteInfoFromNameServer(topic,true);
		brokerAddr = findBrokerAddrByTopic(topic);
	}

	if (!brokerAddr.empty())
	{
		try
		{
			return m_pMQClientAPIImpl->getConsumerIdListByGroup(brokerAddr, group, getTcpTimeoutMilliseconds()/*3000 mdy by lin.qiongshan, 2016-9-2，TCP 操作超时配置化*/);
		}
		catch (...)
		{
			
		}
	}

	std::list<std::string> ids;

	return ids;
}

std::string MQClientFactory::findBrokerAddrByTopic(const std::string& topic)
{
	std::map<std::string, TopicRouteData>::iterator it = m_topicRouteTable.find(topic);

	if (it!=m_topicRouteTable.end())
	{
		const std::list<BrokerData>& brokers = it->second.getBrokerDatas();

		if (!brokers.empty())
		{
			BrokerData bd = brokers.front();
			return TopicRouteData::selectBrokerAddr(bd);
		}
	}

	return "";
}


//mjx modify add

std::string MQClientFactory::findBrokerNameByTopic(const std::string& topic)
{
	std::map<std::string, TopicRouteData>::iterator it = m_topicRouteTable.find(topic);

	if (it!=m_topicRouteTable.end())
	{
		const std::list<BrokerData>& brokers = it->second.getBrokerDatas();

		if (!brokers.empty())
		{
			BrokerData bd = brokers.front();
			return bd.brokerName;
		}
	}

	return "";
}

//mjx modify add

std::string MQClientFactory::findMasterBrokerAddrByTopicAndName(const std::string& topic,const std::string & brokerName)
{
	std::map<std::string, TopicRouteData>::iterator it = m_topicRouteTable.find(topic);

	if (it!=m_topicRouteTable.end())
	{
		const std::list<BrokerData>& brokers = it->second.getBrokerDatas();

		if (!brokers.empty())
		{
			std::list<BrokerData>::const_iterator itor = brokers.begin();
			for(; itor != brokers.end(); itor++)
				if(itor->brokerName == brokerName)
				{
					return TopicRouteData::selectBrokerAddr(const_cast<BrokerData&>(*itor));

				}
			
		}
	}

	return "";
}





TopicRouteData MQClientFactory::getAnExistTopicRouteData(const std::string& topic)
{
	std::map<std::string, TopicRouteData>::iterator it = m_topicRouteTable.find(topic);
	
	if (it!=m_topicRouteTable.end())
	{
		return it->second;
	}

	TopicRouteData data;
	return data;
}

MQClientAPIImpl* MQClientFactory::getMQClientAPIImpl()
{
	return m_pMQClientAPIImpl;
}

MQAdminImpl* MQClientFactory::getMQAdminImpl()
{
	return m_pMQAdminImpl;
}

std::string MQClientFactory::getClientId()
{
	return m_clientId;
}

long long MQClientFactory::getBootTimestamp()
{
	return m_bootTimestamp;
}

PullMessageService* MQClientFactory::getPullMessageService()
{
	return m_pPullMessageService;
}


DefaultMQProducer* MQClientFactory::getDefaultMQProducer()
{
	return m_pDefaultMQProducer;
}

void MQClientFactory::setTcpTimeoutMilliseconds(int milliseconds)
{
	m_tcpTimeoutMillseconds = milliseconds;

	//2nd operand, 3rd operand 的返回类型必须是兼容的，因此将 3rd operand 使用逗号表达式，返回 NULL
	//	因为操作符优先级问题，3rd operand 的逗号表达式必须使用圆括号包围
	NULL == m_pMQAdminImpl ?
		NULL:
		(m_pMQAdminImpl->setTcpTimeoutMillseconds(milliseconds), NULL);
}

int MQClientFactory::getTcpTimeoutMilliseconds()
{
	return m_tcpTimeoutMillseconds;
}

void MQClientFactory::sendHeartbeatToAllBroker()
{
	HeartbeatData* heartbeatData = prepareHeartbeatData();
	bool producerEmpty = heartbeatData->getProducerDataSet().empty();
	bool consumerEmpty = heartbeatData->getConsumerDataSet().empty();
	if (producerEmpty && consumerEmpty)
	{
		return;
	}

    /* modified by yu.guangjie at 2015-11-04, reason: */
    std::map<std::string, std::map<int, std::string> > tmpBroker;
    {        
        kpr::ScopedLock<kpr::Mutex> lock(m_brokerAddrTableLock);
        tmpBroker = m_brokerAddrTable;
    }

	std::map<std::string, std::map<int, std::string> >::iterator it = tmpBroker.begin();

	for (; it!=tmpBroker.end(); it++)
	{
		std::map<int, std::string>::iterator it1 = it->second.begin();
		for (; it1!=it->second.end(); it1++)
		{
			std::string& addr = it1->second;
			if (!addr.empty())
			{
				// 说明只有Producer，则不向Slave发心跳
				if (consumerEmpty)
				{
					if (it1->first != MixAll::MASTER_ID)
						continue;
				}

				try
				{
					m_pMQClientAPIImpl->sendHearbeat(addr, heartbeatData, getTcpTimeoutMilliseconds()/*3000 mdy by lin.qiongshan, 2016-9-2，TCP 操作超时配置化 */);
				}
				catch (...)
				{
				}
			}
		}
	}
    /* modified by yu.guangjie at 2015-08-28, reason: delete heartbeatData */
    delete heartbeatData;
}

HeartbeatData* MQClientFactory::prepareHeartbeatData()
{
	HeartbeatData* heartbeatData = new HeartbeatData();

	// clientID
	heartbeatData->setClientID(m_clientId);

	// Consumer
	{
		//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
		kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

		std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();

		for (; it!=m_consumerTable.end(); it++)
		{
			MQConsumerInner* inner = it->second;
			if (inner)
			{
				ConsumerData consumerData;
				consumerData.groupName = inner->groupName();
				consumerData.consumeType = inner->consumeType();
				consumerData.messageModel = inner->messageModel();
				consumerData.consumeFromWhere = inner->consumeFromWhere();
				consumerData.subscriptionDataSet=inner->subscriptions();

				heartbeatData->getConsumerDataSet().insert(consumerData);
			}
		}
	}

	// Producer
	{
		//add by lin.qiongshan, 2016年8月30日, m_producerTable 操作应该加锁
		kpr::ScopedLock<kpr::Mutex> lock(m_producerTableLock);

		std::map<std::string, MQProducerInner*>::iterator it = m_producerTable.begin();

		for (; it!=m_producerTable.end(); it++)
		{
			MQProducerInner* inner = it->second;
			if (inner)
			{
				ProducerData producerData;
				producerData.groupName = (it->first);

				heartbeatData->getProducerDataSet().insert(producerData);
			}
		}
	}


	return heartbeatData;
}

void MQClientFactory::makesureInstanceNameIsOnly(const std::string& instanceName)
{
	//TODO
}

void MQClientFactory::fetchNameServerAddr()
{
	//mdy by lin.qs, 定时查询并更新 nameserver 地址，不应该加非空判断；否则地址获取成功一次后一直非空，就无法动态更新 nameserver 地址了
	//if (m_clientConfig.getNamesrvAddr().empty())
	{
		//1000 * 10, 1000 * 60 * 2
		try
		{
			m_pMQClientAPIImpl->fetchNameServerAddr();
		}
		catch (...)
		{
		}
	}
}

void MQClientFactory::updateTopicRouteInfoFromNameServerTask()
{
	//10, m_clientConfig.getPollNameServerInteval()
	try
	{
		updateTopicRouteInfoFromNameServer();
	}
	catch (...)
	{

	}
}

void MQClientFactory::cleanBroker()
{
	//1000, m_clientConfig.getHeartbeatBrokerInterval()
	try
	{
		cleanOfflineBroker();
		sendHeartbeatToAllBrokerWithLock();
	}
	catch (...)
	{

	}
}

void MQClientFactory::persistAllConsumerOffsetTask()
{
	//1000 * 10, m_clientConfig.getPersistConsumerOffsetInterval()
	try
	{
		MqLogDebug("prepare to persist consumer offset ...");
		persistAllConsumerOffset();
	}
	catch (...)
	{

	}
}

void MQClientFactory::recordSnapshotPeriodicallyTask()
{
	// 1000 * 10, 1000,
	try
	{
		recordSnapshotPeriodically();
	}
	catch (...)
	{

	}
}

void MQClientFactory::logStatsPeriodicallyTask()
{
	//  1000 * 10, 1000 * 60
	try
	{
		logStatsPeriodically();
	}
	catch (...)
	{

	}
}

void MQClientFactory::startScheduledTask()
{
	// 定时获取Name Server地址
	m_scheduledTaskIds[0] = m_timerTaskManager.RegisterTimer(1000 * 10, 1000 * 60 * 2,m_pFetchNameServerAddr);

	// 定时从Name Server获取Topic路由信息
	m_scheduledTaskIds[1] = m_timerTaskManager.RegisterTimer(10, m_clientConfig.getPollNameServerInteval(),m_pUpdateTopicRouteInfoFromNameServerTask);

	// 定时清理下线的Broker
	// 向所有Broker发送心跳信息（包含订阅关系等）
	m_scheduledTaskIds[2] = m_timerTaskManager.RegisterTimer(1000, m_clientConfig.getHeartbeatBrokerInterval(),m_pCleanBroker);

	// 定时持久化Consumer消费进度（广播存储到本地，集群存储到Broker）
	m_scheduledTaskIds[3] = m_timerTaskManager.RegisterTimer(1000 * 10, m_clientConfig.getPersistConsumerOffsetInterval(),m_pPersistAllConsumerOffsetTask);

	// 统计信息打点
	m_scheduledTaskIds[4] = m_timerTaskManager.RegisterTimer(1000 * 10, 1000,m_pRecordSnapshotPeriodicallyTask);
	m_scheduledTaskIds[5] = m_timerTaskManager.RegisterTimer(1000 * 10, 1000 * 60,m_pLogStatsPeriodicallyTask);
}

void MQClientFactory::cleanOfflineBroker()
{
    /* modified by yu.guangjie at 2015-11-04, reason: */
    kpr::ScopedLock<kpr::Mutex> lock(m_brokerAddrTableLock);
    
	std::map<std::string, std::map<int, std::string> >::iterator it = m_brokerAddrTable.begin();
	std::map<std::string, std::map<int, std::string> > updatedTable;

	for (; it!=m_brokerAddrTable.end(); it++)
	{
		std::map<int, std::string> cloneTable = it->second;

		std::map<int, std::string>::iterator it1 = cloneTable.begin();

		for (; it1!=cloneTable.end();)
		{
			std::string& addr = it1->second;
			if (!isBrokerAddrExistInTopicRouteTable(addr))
			{
				std::map<int, std::string>::iterator itTmp = it1;
				it1++;
				cloneTable.erase(itTmp);
				continue;
			}

			it1++;
		}

		if (!cloneTable.empty())
		{
			updatedTable[it->first] = cloneTable;
		}
	}

	m_brokerAddrTable.clear();
	m_brokerAddrTable = updatedTable;
}

bool MQClientFactory::isBrokerAddrExistInTopicRouteTable(const std::string& addr)
{
	std::map<std::string, TopicRouteData>::iterator it = m_topicRouteTable.begin();
	bool changed = false;

	for(; it!=m_topicRouteTable.end(); it++)
	{
		const std::list<BrokerData>& brokers = it->second.getBrokerDatas();
		std::list<BrokerData>::const_iterator it1 = brokers.begin();

		for (; it1!=brokers.end(); it1++)
		{
			std::map<int, std::string>::const_iterator it2= (*it1).brokerAddrs.begin();
			for (; it2!=(*it1).brokerAddrs.end(); it2++)
			{
				if (it2->second.find(addr)!=std::string::npos)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void MQClientFactory::recordSnapshotPeriodically()
{
	//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
	kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

	std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();

	for (; it!=m_consumerTable.end(); it++)
	{
		MQConsumerInner* inner = it->second;
		if (inner)
		{
			DefaultMQPushConsumerImpl* consumer = dynamic_cast<DefaultMQPushConsumerImpl*>(inner);
			if (consumer)
			{
				consumer->getConsumerStatManager()->recordSnapshotPeriodically();
			}
		}
	}
}

void MQClientFactory::logStatsPeriodically()
{
	//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
	kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

	std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();

	for (; it!=m_consumerTable.end(); it++)
	{
		MQConsumerInner* inner = it->second;
		if (inner)
		{
			DefaultMQPushConsumerImpl* consumer = dynamic_cast<DefaultMQPushConsumerImpl*>(inner);
			if (consumer)
			{
				std::string group = it->first;
				consumer->getConsumerStatManager()->logStatsPeriodically(group,m_clientId);
			}
		}
	}
}

void MQClientFactory::persistAllConsumerOffset()
{
	//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
	kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

	std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();

	for (; it!=m_consumerTable.end(); it++)
	{
		MQConsumerInner* inner = it->second;
		if (inner)
		{
			inner->persistConsumerOffset();
		}
	}
}

bool MQClientFactory::topicRouteDataIsChange(TopicRouteData& olddata, TopicRouteData& nowdata)
{
	TopicRouteData old = olddata;
	TopicRouteData now = nowdata;

	old.getQueueDatas().sort();
	old.getBrokerDatas().sort();
	now.getQueueDatas().sort();
	now.getBrokerDatas().sort();

	return !(old==now);

}

bool MQClientFactory::isNeedUpdateTopicRouteInfo(const std::string& topic)
{
	bool result = false;
    
    /* modified by yu.guangjie at 2015-08-14, reason: use result*/
    
	// 查看发布队列是否需要更新
	{
		//add by lin.qiongshan, 2016年8月30日, m_producerTable 操作应该加锁
		kpr::ScopedLock<kpr::Mutex> lock(m_producerTableLock);

		std::map<std::string, MQProducerInner*>::iterator it = m_producerTable.begin();

		for (; it!=m_producerTable.end(); it++)
		{
			MQProducerInner* inner = it->second;
			if (inner)
			{
				result = inner->isPublishTopicNeedUpdate(topic);
			}
		}
	}

	// 查看订阅队列是否需要更新
	{
		//add by lin.qiongshan, 2016年8月30日, m_consumerTable 操作应该加锁
		kpr::ScopedLock<kpr::Mutex> lock(m_consumerTableLock);

		std::map<std::string, MQConsumerInner*>::iterator it = m_consumerTable.begin();

		for (; it!=m_consumerTable.end(); it++)
		{
			MQConsumerInner* inner = it->second;
			if (inner)
			{
				result = inner->isSubscribeTopicNeedUpdate(topic);
			}
		}
	}

	return result;
}

void MQClientFactory::unregisterClientWithLock(const std::string& producerGroup, const std::string& consumerGroup)
{
	try
	{
		if (m_lockHeartbeat.TryLock())
		{
			try
			{
				unregisterClient(producerGroup, consumerGroup);
				m_lockHeartbeat.Unlock();
			}
			catch (...)
			{
				m_lockHeartbeat.Unlock();
			}
		}
		else
		{
		}
	}
	catch (...)
	{

	}
}

void MQClientFactory::unregisterClient(const std::string& producerGroup, const std::string& consumerGroup)
{
    MqLogNotice("unregister[producerGroup:%s , consumerGroup: %s]", 
        producerGroup.c_str(), consumerGroup.c_str());

    /* modified by yu.guangjie at 2015-11-04, reason: */
    std::map<std::string, std::map<int, std::string> > tmpBroker;
    {        
        kpr::ScopedLock<kpr::Mutex> lock(m_brokerAddrTableLock);
        tmpBroker = m_brokerAddrTable;
    }
	std::map<std::string, std::map<int, std::string> >::iterator it = tmpBroker.begin();

	for (; it!=tmpBroker.end(); it++)
	{
		std::map<int, std::string>::iterator it1 = it->second.begin();

		for (; it1!=it->second.end();it1++)
		{
			std::string& addr = it1->second;

			if (!addr.empty())
			{
				try
				{
					m_pMQClientAPIImpl->unregisterClient(addr, m_clientId, producerGroup,
														 //consumerGroup, 3000);	mdy by lin.qiongshan, 2016-9-2，TCP 操作超时配置化
														consumerGroup, getTcpTimeoutMilliseconds());
				}
				catch (...)
				{

				}
			}
		}
	}
}
