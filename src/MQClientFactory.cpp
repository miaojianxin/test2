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

	// Consumer����
	{
		//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
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
		//add by lin.qiongshan, 2016��8��30��, m_producerTable ����Ӧ�ü���
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
							//pDefaultMQProducer->getCreateTopicKey(), 1000 * 3); mdy by lin.qiongshan, 2016-9-2��TCP ������ʱ���û�
							pDefaultMQProducer->getCreateTopicKey(), getTcpTimeoutMilliseconds());
					if (topicRouteData != NULL)
					{
						//mdy by lin.qiongshan, 2016-11-22��bug: dataList Ҫ����Ϊ���ã�&������������޸Ĳ���Ӱ�� topicRouteData �ڵ�����
						std::list<QueueData>& dataList = topicRouteData->getQueueDatas();

						std::list<QueueData>::iterator it= dataList.begin();
						for(; it!=dataList.end(); it++)
						{
							//mdy by lin.qiongshan, 2016-11-22��bug: data Ҫ����Ϊ���ã�&������������޸Ĳ���Ӱ�� topicRouteData �ڵ�����
							QueueData& data = *it;
							// ��д����������һ�£���ֻ��һ���ж�
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
						// ���������Ӱ���´ε�equal�߼��жϣ�������cloneһ��
						TopicRouteData cloneTopicRouteData = *topicRouteData;

						// ����Broker��ַ��Ϣ
						std::list<BrokerData> dataList = topicRouteData->getBrokerDatas();

						std::list<BrokerData>::iterator it= dataList.begin();
						for(; it!=dataList.end(); it++)
						{
                            /* modified by yu.guangjie at 2015-11-04, reason: */
                            kpr::ScopedLock<kpr::Mutex> lock(m_brokerAddrTableLock);
							m_brokerAddrTable[(*it).brokerName]=(*it).brokerAddrs;
						}

						// ���·���������Ϣ
						{
							//add by lin.qiongshan, 2016��8��30��, m_producerTable ����Ӧ�ü���
							kpr::ScopedLock<kpr::Mutex> lock(m_producerTableLock);

							//[bug] mdy by lin.qiongshan��2016-11-22
							//
							//	�� DefaultMQProducerImpl::updateTopicPublishInfo �����ڲ���DefaultMQProducerImpl ����Ὣ�����·����Ϣ��TopicPublishInfo�����浽���Ա�У�m_topicPublishInfoTable��
							//	��������У�m_topicPublishInfoTable �����˴���� TopicPublishInfo �����һ�ݸ���
							//	TopicPublishInfo �����ڲ������˶�����Ϣ��MessageQueue������ͨ�� vector ����������һ�� MessageQueue ��ָ��
							//	��Ȼ m_topicPublishInfoTable ������Ǵ���� TopicPublishInfo ����ĸ��ƣ������Ƶ�ʱ��MessageQueue ��ָ��Ҳһ�����˹�ȥ�������Ǵ����µ� MessageQueue
							//	��͵��£��˴���ɾ�� TopicPublishInfo���� DefaultMQProducerImpl ��һЩ����Ҳ������ TopicPublishInfo �ڲ��� MessageQueue ���ݣ�TopicPublishInfo::clearMessageQueue���˴�Ҳ��ɾ�� MessageQueue ���󣩣���͵��¶�ͬһ��ָ���ظ�ɾ�������� coredump
							//
							//  ���ң���������ж�� Producer �Ļ����� TopicPublishInfo �Ḵ�Ƶ���� Producer ������
							//
							//�޸ķ�����
							//	�޸� TopicPublishInfo�����Ӹ��ƹ��캯���͸�ֵ���������أ����� MessageQueue ����ʱ�����µ� MessageQueue ����
							//	�޸� TopicPublishInfo��������������������ʱҪɾ�� MessageQueue ָ�����
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
						// ���¶��Ķ�����Ϣ
						{
							//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
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
						//����Ѿ��ӹ���
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
	// ˳����Ϣ
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
	// ��˳����Ϣ
	else
	{
		std::list<QueueData> qds = route.getQueueDatas();
		// ����ԭ�򣺼�ʹû������˳����Ϣģʽ��Ĭ�϶��е�˳��ͬ���õ�һ�¡�
		qds.sort();
		std::list<QueueData>::iterator it = qds.begin();
		for (; it!=qds.end(); it++)
		{
			QueueData& qd=(*it);
			if (PermName::isWriteable(qd.perm))
			{
				// ������Ҫ�ж�BrokerName��Ӧ��Master�Ƿ���ڣ���Ϊֻ����Master������Ϣ
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

	//add by lin.qiongshan, 2016��8��30��, m_producerTable ����Ӧ�ü���
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
		//add by lin.qiongshan, 2016��8��30��, m_producerTable ����Ӧ�ü���
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
	//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
	//	Mark by lin.qiongshan, 2016-11-3, �˴����ܼ������� doRebalance ������ݹ���õ� updateTopicRouteInfoFromNameServer ���������÷���Ҳ��Ը��������������������
	//		�������ײ�ļ�����ɾ���˴��ļ���
	//mjx consumer_table_lock bug fix
	//��������core��ͨ��������tag������ͬһ���߳���Ƕ�׼���
	//����ͨ���Ӹ��̵߳Ķ���ȫ�ֱ���__thread XX������
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
	//add by lin.qiongshan, 2016��8��30��, m_producerTable ����Ӧ�ü���
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
	//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
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
			return m_pMQClientAPIImpl->getConsumerIdListByGroup(brokerAddr, group, getTcpTimeoutMilliseconds()/*3000 mdy by lin.qiongshan, 2016-9-2��TCP ������ʱ���û�*/);
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

	//2nd operand, 3rd operand �ķ������ͱ����Ǽ��ݵģ���˽� 3rd operand ʹ�ö��ű��ʽ������ NULL
	//	��Ϊ���������ȼ����⣬3rd operand �Ķ��ű��ʽ����ʹ��Բ���Ű�Χ
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
				// ˵��ֻ��Producer������Slave������
				if (consumerEmpty)
				{
					if (it1->first != MixAll::MASTER_ID)
						continue;
				}

				try
				{
					m_pMQClientAPIImpl->sendHearbeat(addr, heartbeatData, getTcpTimeoutMilliseconds()/*3000 mdy by lin.qiongshan, 2016-9-2��TCP ������ʱ���û� */);
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
		//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
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
		//add by lin.qiongshan, 2016��8��30��, m_producerTable ����Ӧ�ü���
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
	//mdy by lin.qs, ��ʱ��ѯ������ nameserver ��ַ����Ӧ�üӷǿ��жϣ������ַ��ȡ�ɹ�һ�κ�һֱ�ǿգ����޷���̬���� nameserver ��ַ��
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
	// ��ʱ��ȡName Server��ַ
	m_scheduledTaskIds[0] = m_timerTaskManager.RegisterTimer(1000 * 10, 1000 * 60 * 2,m_pFetchNameServerAddr);

	// ��ʱ��Name Server��ȡTopic·����Ϣ
	m_scheduledTaskIds[1] = m_timerTaskManager.RegisterTimer(10, m_clientConfig.getPollNameServerInteval(),m_pUpdateTopicRouteInfoFromNameServerTask);

	// ��ʱ�������ߵ�Broker
	// ������Broker����������Ϣ���������Ĺ�ϵ�ȣ�
	m_scheduledTaskIds[2] = m_timerTaskManager.RegisterTimer(1000, m_clientConfig.getHeartbeatBrokerInterval(),m_pCleanBroker);

	// ��ʱ�־û�Consumer���ѽ��ȣ��㲥�洢�����أ���Ⱥ�洢��Broker��
	m_scheduledTaskIds[3] = m_timerTaskManager.RegisterTimer(1000 * 10, m_clientConfig.getPersistConsumerOffsetInterval(),m_pPersistAllConsumerOffsetTask);

	// ͳ����Ϣ���
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
	//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
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
	//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
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
	//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
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
    
	// �鿴���������Ƿ���Ҫ����
	{
		//add by lin.qiongshan, 2016��8��30��, m_producerTable ����Ӧ�ü���
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

	// �鿴���Ķ����Ƿ���Ҫ����
	{
		//add by lin.qiongshan, 2016��8��30��, m_consumerTable ����Ӧ�ü���
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
														 //consumerGroup, 3000);	mdy by lin.qiongshan, 2016-9-2��TCP ������ʱ���û�
														consumerGroup, getTcpTimeoutMilliseconds());
				}
				catch (...)
				{

				}
			}
		}
	}
}
