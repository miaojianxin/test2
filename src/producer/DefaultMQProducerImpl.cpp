/**
* Copyright (C) 2013 kangliqiang ,kangliq@163.com
*
* Licensed under the Apache License, Version 2.0 (the "License")
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

#include "DefaultMQProducerImpl.h"
#include "DefaultMQProducer.h"
#include "MessageExt.h"
#include "QueryResult.h"
#include "TopicPublishInfo.h"
#include "MQClientException.h"
#include "LocalTransactionExecuter.h"
#include "SendMessageHook.h"
#include "MQClientManager.h"
#include "MQClientFactory.h"
#include "Validators.h"
#include "MQAdminImpl.h"
#include "MQClientAPIImpl.h"
#include "MessageSysFlag.h"
#include "CommandCustomHeader.h"
#include "KPRUtil.h"
#include "MessageDecoder.h"
#include "MQProtos.h"
#include "RemotingCommand.h"
#include "UtilAll.h"
#include "ScopedLock.h"

DefaultMQProducerImpl::DefaultMQProducerImpl(DefaultMQProducer* pDefaultMQProducer)
	:m_pDefaultMQProducer(pDefaultMQProducer),
	m_serviceState(CREATE_JUST)
{

}

DefaultMQProducerImpl::~DefaultMQProducerImpl()
{
    std::map<std::string, TopicPublishInfo>::iterator it = m_topicPublishInfoTable.begin();

	for(; it!=m_topicPublishInfoTable.end(); it++)
	{
		it->second.clearMessageQueue();
	}
    m_topicPublishInfoTable.clear();
}

void DefaultMQProducerImpl::initTransactionEnv()
{
	//TODO
}

void DefaultMQProducerImpl::destroyTransactionEnv()
{
	//TODO
}

bool DefaultMQProducerImpl::hasHook()
{
	return !m_hookList.empty();
}

void DefaultMQProducerImpl::registerHook(SendMessageHook* pHook)
{
	m_hookList.push_back(pHook);
}

void DefaultMQProducerImpl::executeHookBefore(const SendMessageContext& context)
{
	std::list<SendMessageHook*>::iterator it = m_hookList.begin();
	for(; it!=m_hookList.end(); it++)
	{
		try
		{
			(*it)->sendMessageBefore(context);
		}
		catch (...)
		{
		}
	}
}

void DefaultMQProducerImpl::executeHookAfter(const SendMessageContext& context)
{
	std::list<SendMessageHook*>::iterator it = m_hookList.begin();
	for(; it!=m_hookList.end(); it++)
	{
		try
		{
			(*it)->sendMessageAfter(context);
		}
		catch (...)
		{
		}
	}
}

void DefaultMQProducerImpl::start()
{
	start(true);
}

void DefaultMQProducerImpl::start(bool startFactory)
{
	switch (m_serviceState)
	{
	case CREATE_JUST:
	{
		m_serviceState = START_FAILED;

		checkConfig();

		bool isClientInnerProduerGroup = false;	//add@2017-3-29, @SEE 2017-3-28 Producer 注册流程问题
		if (m_pDefaultMQProducer->getProducerGroup() == MixAll::CLIENT_INNER_PRODUCER_GROUP) 
		{
			m_pDefaultMQProducer->changeInstanceNameToPID();
			isClientInnerProduerGroup = true;
		}

		m_pMQClientFactory = MQClientManager::getInstance()->getAndCreateMQClientFactory(*m_pDefaultMQProducer);

		/** 2017-3-28 Producer 注册流程问题

			Tips:
				1.MQClientFactory 是通过 MQClientManager 管理的，使用 clientId 进行区分。即 clientId 相同的 DefaultMQProducer，DefaultMQPushConsumer 和 DefaultMQPullConsumer 会注册到同一个 MQClientFactory 实例对象
				2.应用进程实例化一个 DefaultMQProducer，其 clientId 默认为 ip@DEFAULT，注册到对应的 MQClientFactory 对象后，会调用该 MQClientFactory 的 start 方法
				3.MQClientFactory 内部包含一个 DefaultMQProducer 对象（group 是 MixAll::CLIENT_INNER_PRODUCER_GROUP），当调用 MQClientFactory 的 start 方法时，会再次调用该内部的 DefaultMQProducer 的 start 方法
				4.DefaultMQProducer 在 start 方法中，会判断自身的 GroupName 是不是 MixAll::CLIENT_INNER_PRODUCER_GROUP，如果是的话，会将 clientId 替换为 ip@pid
				5.DefaultMQPushConsumer, DefaultMQPullConsumer 在集群模式下，会将自身的 clientId 替换为 ip@pid
				6.同一个 MQClientFactory 可以重复调用 start 方法，正常情况下不会有副作用
				7.DefaultMQProducer, DefaultMQPushConsumer, DefaultMQPullConsumer 在 start 方法中，会将自身注册到关联的 MQClientFactory 对象（根据 clientId 管理）
				8.MQClientFactory 的内置 DefaultMQProducer 对象 start 时，会通过参数指定，内置 DefaultMQProducer 对象 start 方法中，不会再调用关联的 MQClientFactory 对象的 start【避免无限递归 start】

			异常场景：
				实例化并 start 一个 DefaultMQPullConsumer 对象后，再实例化并 start 一个 DefaultMQProducer 对象，会失败，提示 MixAll::CLIENT_INNER_PRODUCER_GROUP 这个 producer 重复诸恶
			原因：
				1.实例化一个 DefaultMQPullConsumer 对象，使用集群模式，其 clientId 是 ip@pid。
					此时 DefaultMQPullConsumer 执行 start 方法时，会调用其关联的 MQClientFactory(clientId=ip@pid) 执行 start
					MQClientFactory(clientId=ip@pid) 在 start 时，会调用内置的 DefaultMQProducer 对象执行 start
					其内置的 DefaultMQProducer 对象（见 Tips.4）的 clientId 也是 ip@pid，该 DefaultMQProducer 对象会注册到 MQClientFactory(clientId=ip@pid)
				2.当实例化一个 DefaultMQProducer 对象时，通常应用程序会设置一个 ProducerGroup，假定为 “PG”，其 clientId 是 ip@PG，此时调用 start，会同时执行其关联的 MQClientFactory(clientId=ip@PG) 的 start
					MQClientFactory(clientId=ip@PG) 在 start 时，会调用其内置的 DefaultMQProducer 对象的 start 方法
					其内置的 DefaultMQProducer 对象的 clientId 也是 ip@pid, 在 start 时，会注册到 MQClientFactory(clientId=ip@pid)
					由于 start DefaultMQPullConsumer 时， MixAll::CLIENT_INNER_PRODUCER_GROUP 已经注册了，会出现重复注册的错误
				3.理论上分析，只要有两个 clientId 不同的 Producer 或 Consumer 实例，执行了 start，都会出现这种错误
					因为不同的 clientId 关联的不同的 MQClientFactory 对象，都至少会 start 一次，在 start 时会先注册内置的 DefaultMQProducer 对象
					而不同的 MQClientFactory 对象，内置的 DefaultMQProducer 的 clientId 是相同的，因此会注册到同一个 MQClientFactory 对象，导致重复注册错误
			修改方案：
				如果发现 DefaultMQProducer 的 producerGroup 是 MixAll::CLIENT_INNER_PRODUCER_GROUP，注册失败不抛出异常，忽略即可
		*/
		bool registerOK = m_pMQClientFactory->registerProducer(m_pDefaultMQProducer->getProducerGroup(), this);
		if (!registerOK && !isClientInnerProduerGroup) //modify@2017-3-29, @SEE 2017-3-28 Producer 注册流程问题
		{
			m_serviceState = CREATE_JUST;

			THROW_MQEXCEPTION(MQClientException,"The producer group[" + m_pDefaultMQProducer->getProducerGroup()
							  + "] has been created before, specify another name please.",-1);
		}

		// 默认Topic注册
		m_topicPublishInfoTable[m_pDefaultMQProducer->getCreateTopicKey()] = TopicPublishInfo();

		if (startFactory)
		{
			m_pMQClientFactory->start();
		}

		m_serviceState = RUNNING;
	}
	break;
	case RUNNING:
	case START_FAILED:
	case SHUTDOWN_ALREADY:
		THROW_MQEXCEPTION(MQClientException,"The producer service state not OK, maybe started once, ",-1);
	default:
		break;
	}

	m_pMQClientFactory->sendHeartbeatToAllBrokerWithLock();
}

void DefaultMQProducerImpl::shutdown()
{
	shutdown(true);
}

void DefaultMQProducerImpl::shutdown(bool shutdownFactory)
{
	switch (m_serviceState)
	{
	case CREATE_JUST:
		break;
	case RUNNING:
		m_pMQClientFactory->unregisterProducer(m_pDefaultMQProducer->getProducerGroup());
		if (shutdownFactory)
		{
			m_pMQClientFactory->shutdown();
		}

		m_serviceState = SHUTDOWN_ALREADY;
		break;
	case SHUTDOWN_ALREADY:
		break;
	default:
		break;
	}
}

//父类接口实现
std::set<std::string> DefaultMQProducerImpl::getPublishTopicList()
{
	std::set<std::string> toplist;
    kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	std::map<std::string, TopicPublishInfo>::iterator it = m_topicPublishInfoTable.begin();

	for(; it!=m_topicPublishInfoTable.end(); it++)
	{
		toplist.insert(it->first);
	}

	return toplist;
}

bool DefaultMQProducerImpl::isPublishTopicNeedUpdate(const std::string& topic)
{
    kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	std::map<std::string, TopicPublishInfo>::iterator it = m_topicPublishInfoTable.find(topic);
	if (it!=m_topicPublishInfoTable.end())
	{
		return !it->second.ok();
	}

	return true;
}

void DefaultMQProducerImpl::checkTransactionState(const std::string& addr, //
		const MessageExt& msg, //
		const CheckTransactionStateRequestHeader& checkRequestHeader)
{
	//TODO
}

void DefaultMQProducerImpl::updateTopicPublishInfo(const std::string& topic,
		 TopicPublishInfo& info)
{
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
    	std::map<std::string, TopicPublishInfo>::iterator it = m_topicPublishInfoTable.find(topic);

    	if (it!=m_topicPublishInfoTable.end())
    	{
    		info.getSendWhichQueue()=it->second.getSendWhichQueue();
            it->second.clearMessageQueue();
            m_topicPublishInfoTable[topic]=info;
    	}
        else
        {
            m_topicPublishInfoTable[topic]=info;
        }
    }
    
	
    MqLogNotice("Update publish topic[%s] routeinfo: MessageQueue=%d", 
        topic.c_str(), info.getMessageQueueList().size());
}

//父类接口实现 end

void DefaultMQProducerImpl::createTopic(const std::string& key, const std::string& newTopic, int queueNum)
{
	makeSureStateOK();
	// topic 有效性检查
	Validators::checkTopic(newTopic);

	m_pMQClientFactory->getMQAdminImpl()->createTopic(key, newTopic, queueNum);
}

std::vector<MessageQueue>* DefaultMQProducerImpl::fetchPublishMessageQueues(const std::string& topic)
{
	makeSureStateOK();
	return m_pMQClientFactory->getMQAdminImpl()->fetchPublishMessageQueues(topic);
}

long long DefaultMQProducerImpl::searchOffset(const MessageQueue& mq, long long timestamp)
{
	makeSureStateOK();
	return m_pMQClientFactory->getMQAdminImpl()->searchOffset(mq,timestamp);
}

long long DefaultMQProducerImpl::maxOffset(const MessageQueue& mq)
{
	makeSureStateOK();
	return m_pMQClientFactory->getMQAdminImpl()->maxOffset(mq);
}

long long DefaultMQProducerImpl::minOffset(const MessageQueue& mq)
{
	makeSureStateOK();
	return m_pMQClientFactory->getMQAdminImpl()->minOffset(mq);
}

long long DefaultMQProducerImpl::earliestMsgStoreTime(const MessageQueue& mq)
{
	makeSureStateOK();
	return m_pMQClientFactory->getMQAdminImpl()->earliestMsgStoreTime(mq);
}

MessageExt* DefaultMQProducerImpl::viewMessage(const std::string& msgId)
{
	makeSureStateOK();
	return m_pMQClientFactory->getMQAdminImpl()->viewMessage(msgId);
}

QueryResult DefaultMQProducerImpl::queryMessage(const std::string& topic, const std::string& key, int maxNum, long long begin, long long end)
{
	makeSureStateOK();
	return m_pMQClientFactory->getMQAdminImpl()->queryMessage(topic,key,maxNum,begin,end);
}

SendResult DefaultMQProducerImpl::send(Message& msg)
{
	return sendDefaultImpl(msg, SYNC, NULL);
}

void DefaultMQProducerImpl::send(Message& msg, SendCallback* pSendCallback)
{
	try
	{
		sendDefaultImpl(msg, ASYNC, pSendCallback);
	}
	catch (MQBrokerException e)
	{
		
	}
}

void DefaultMQProducerImpl::sendOneway(Message& msg)
{
	try
	{
		sendDefaultImpl(msg, ONEWAY,NULL);
	}
	catch (MQBrokerException e)
	{

	}
}

SendResult DefaultMQProducerImpl::send(Message& msg, MessageQueue& mq)
{
	// 有效性检查
	makeSureStateOK();
	Validators::checkMessage(msg, m_pDefaultMQProducer->getMaxMessageSize());

	if (msg.getTopic()!=mq.getTopic())
	{
		THROW_MQEXCEPTION(MQClientException,"message's topic not equal mq's topic",-1);
	}

	return sendKernelImpl(msg, mq, SYNC, NULL);
}

void DefaultMQProducerImpl::send(Message& msg, MessageQueue& mq, SendCallback* pSendCallback)
{
	// 有效性检查
	makeSureStateOK();
	Validators::checkMessage(msg, m_pDefaultMQProducer->getMaxMessageSize());

	if (msg.getTopic()!=mq.getTopic())
	{
		THROW_MQEXCEPTION(MQClientException,"message's topic not equal mq's topic",-1);
	}

	try
	{
		sendKernelImpl(msg, mq, ASYNC, pSendCallback);
	}
	catch(...)
	{

	}
}

void DefaultMQProducerImpl::sendOneway(Message& msg, MessageQueue& mq)
{
	// 有效性检查
	makeSureStateOK();
	Validators::checkMessage(msg, m_pDefaultMQProducer->getMaxMessageSize());

	if (msg.getTopic()!=mq.getTopic())
	{
		THROW_MQEXCEPTION(MQClientException,"message's topic not equal mq's topic",-1);
	}

	try
	{
		sendKernelImpl(msg, mq, ONEWAY, NULL);
	}
	catch(...)
	{

	}
}

SendResult DefaultMQProducerImpl::send(Message& msg, MessageQueueSelector* pSelector, void* arg)
{
	return sendSelectImpl(msg, pSelector, arg, SYNC, NULL);
}

void DefaultMQProducerImpl::send(Message& msg,
								 MessageQueueSelector* pSelector,
								 void* arg,
								 SendCallback* pSendCallback)
{
	try
	{
		sendSelectImpl(msg, pSelector, arg, ASYNC, pSendCallback);
	}
	catch (MQBrokerException e)
	{

	}
}

void DefaultMQProducerImpl::sendOneway(Message& msg, MessageQueueSelector* pSelector, void* arg)
{
	try
	{
		sendSelectImpl(msg, pSelector, arg, ASYNC, NULL);
	}
	catch (MQBrokerException e)
	{

	}
}

TransactionSendResult DefaultMQProducerImpl::sendMessageInTransaction(Message& msg,
		LocalTransactionExecuter* tranExecuter, void* arg)
{
	//TODO
	TransactionSendResult result;

	return result;
}

void DefaultMQProducerImpl::endTransaction(//
	SendResult sendResult, //
	LocalTransactionState localTransactionState, //
	MQClientException localException)
{
	//TODO
}

std::map<std::string, TopicPublishInfo> DefaultMQProducerImpl::getTopicPublishInfoTable()
{
	return m_topicPublishInfoTable;
}

MQClientFactory* DefaultMQProducerImpl::getmQClientFactory()
{
	return m_pMQClientFactory;
}

int DefaultMQProducerImpl::getZipCompressLevel()
{
	return m_zipCompressLevel;
}

void DefaultMQProducerImpl::setZipCompressLevel(int zipCompressLevel)
{
	m_zipCompressLevel = zipCompressLevel;
}

void DefaultMQProducerImpl::setTcpTimeoutMilliseconds(int milliseconds)
{
	m_tcpTimeoutMilliseconds = milliseconds;

	NULL == m_pMQClientFactory ?
		NULL:
		(m_pMQClientFactory->setTcpTimeoutMilliseconds(milliseconds), NULL);
}

int DefaultMQProducerImpl::getTcpTimeoutMilliseconds()
{
	return m_tcpTimeoutMilliseconds;
}

SendResult DefaultMQProducerImpl::sendDefaultImpl(Message& msg,
												  CommunicationMode communicationMode,
												  SendCallback* pSendCallback)
{
	// 有效性检查
	makeSureStateOK();
	Validators::checkMessage(msg, m_pDefaultMQProducer->getMaxMessageSize());

	long long beginTimestamp = GetCurrentTimeMillis();
	long long endTimestamp = beginTimestamp;

    /* modified by yu.guangjie at 2015-11-24, reason: */
    SendResult sendResult;
    int times = 0;
    std::string lastBrokerName = "";
    for (; times < 3
		&& int(endTimestamp - beginTimestamp) < m_pDefaultMQProducer->getSendMsgTimeout(); times++) 
	{
	    MessageQueue tmpmq = tryToFindTopicPublishMq(msg.getTopic(), lastBrokerName);
        if (tmpmq.getTopic().empty())
        {
            break;
        }
        lastBrokerName = tmpmq.getBrokerName();
        
		try 
		{
			sendResult = sendKernelImpl(msg, tmpmq, communicationMode, pSendCallback);
			endTimestamp =GetCurrentTimeMillis();
			switch (communicationMode) 
			{
			case ASYNC:
				return sendResult;
			case ONEWAY:
				return sendResult;
			case SYNC:
				if (sendResult.getSendStatus() != SEND_OK) 
				{
					if (m_pDefaultMQProducer->isRetryAnotherBrokerWhenNotStoreOK())
					{
						continue;
					}
				}

				return sendResult;
			default:
				break;
			}
		}
		catch (RemotingException& /*e*/)
		{
			endTimestamp = GetCurrentTimeMillis();
			continue;
		}
		catch (MQClientException& /*e*/)
		{
			endTimestamp = GetCurrentTimeMillis();
			continue;
		}
		catch (MQBrokerException& e) 
		{
			endTimestamp =GetCurrentTimeMillis();
			switch (e.GetError()) {
			case TOPIC_NOT_EXIST_VALUE:
			case SERVICE_NOT_AVAILABLE_VALUE:
			case SYSTEM_ERROR_VALUE:
			case NO_PERMISSION_VALUE:
				continue;
			default:
    			/* modified by yu.guangjie at 2016-04-18, reason: */
				//return sendResult;
				
				throw;
			}
		}
		catch (InterruptedException& /*e*/) 
		{
			throw;
		}

	} // end of for
	if (times >= 3)
    {
        THROW_MQEXCEPTION(MQClientException,"Retry many times, still failed",-1);
    }
#if 0
	TopicPublishInfo& topicPublishInfo = tryToFindTopicPublishInfo(msg.getTopic());
	SendResult sendResult;
	if (topicPublishInfo.ok()) 
	{
		MessageQueue* mq=NULL;
		
		for (int times = 0; times < 3
			&& int(endTimestamp - beginTimestamp) < m_pDefaultMQProducer->getSendMsgTimeout(); times++) 
		{
				std::string lastBrokerName = NULL == mq ? "" : mq->getBrokerName();
				MessageQueue* tmpmq = topicPublishInfo.selectOneMessageQueue(lastBrokerName);
				if (tmpmq != NULL) 
				{
					mq = tmpmq;
					try 
					{
						sendResult = sendKernelImpl(msg, *mq, communicationMode, pSendCallback);
						endTimestamp =GetCurrentTimeMillis();
						switch (communicationMode) 
						{
						case ASYNC:
							return sendResult;
						case ONEWAY:
							return sendResult;
						case SYNC:
							if (sendResult.getSendStatus() != SEND_OK) 
							{
								if (m_pDefaultMQProducer->isRetryAnotherBrokerWhenNotStoreOK())
								{
									continue;
								}
							}

							return sendResult;
						default:
							break;
						}
					}
					catch (RemotingException& /*e*/)
					{
						endTimestamp = GetCurrentTimeMillis();
						continue;
					}
					catch (MQClientException& /*e*/)
					{
						endTimestamp = GetCurrentTimeMillis();
						continue;
					}
					catch (MQBrokerException& e) 
					{
						endTimestamp =GetCurrentTimeMillis();
						switch (e.GetError()) {
						case TOPIC_NOT_EXIST_VALUE:
						case SERVICE_NOT_AVAILABLE_VALUE:
						case SYSTEM_ERROR_VALUE:
						case NO_PERMISSION_VALUE:
							continue;
						default:
							return sendResult;

							throw;
						}
					}
					catch (InterruptedException& /*e*/) 
					{
						throw;
					}
				}
				else
				{
					break;
				}
		} // end of for

		THROW_MQEXCEPTION(MQClientException,"Retry many times, still failed",-1);

		return sendResult;
	}
#endif

	std::list<std::string> nsList = getmQClientFactory()->getMQClientAPIImpl()->getNameServerAddressList();
	if (nsList.empty()) 
	{
		// 说明没有设置Name Server地址
		THROW_MQEXCEPTION(MQClientException,"No name server address, please set it.",-1);
	}

	THROW_MQEXCEPTION(MQClientException,"No route info of this topic, ",-1);

	return sendResult;
}

SendResult DefaultMQProducerImpl::sendKernelImpl(Message& msg,
												 const MessageQueue& mq,
												 CommunicationMode communicationMode,
												 SendCallback* sendCallback)
{
	std::string brokerAddr = m_pMQClientFactory->findBrokerAddressInPublish(mq.getBrokerName());

	//找不到，直接抛出异常
	if (brokerAddr.empty()) 
	{
		// 此处可能对Name Server压力过大，需要调优
		tryToFindTopicPublishInfo(mq.getTopic());
		brokerAddr = m_pMQClientFactory->findBrokerAddressInPublish(mq.getBrokerName());
	}

	SendMessageContext context;
	if (!brokerAddr.empty()) 
	{
		const char* prevBody = msg.getBody();
		int prevLen = msg.getBodyLen();

		try 
		{
			int sysFlag = 0;
			if (tryToCompressMessage(msg)) 
			{
				sysFlag |= MessageSysFlag::CompressedFlag;
			}

			std::string tranMsg = msg.getProperty(Message::PROPERTY_TRANSACTION_PREPARED);
			if (!tranMsg.empty() && tranMsg=="true")
			{
				sysFlag |= MessageSysFlag::TransactionPreparedType;
			}

			// 执行hook
			if (hasHook()) 
			{
				context.producerGroup=(m_pDefaultMQProducer->getProducerGroup());
				context.communicationMode=(communicationMode);
				context.brokerAddr=(brokerAddr);
				context.msg=(msg);
				context.mq=(mq);
				executeHookBefore(context);
			}

			SendMessageRequestHeader* requestHeader = new SendMessageRequestHeader();
			requestHeader->producerGroup=(m_pDefaultMQProducer->getProducerGroup());
			requestHeader->topic=(msg.getTopic());
			requestHeader->defaultTopic=(m_pDefaultMQProducer->getCreateTopicKey());
			requestHeader->defaultTopicQueueNums=(m_pDefaultMQProducer->getDefaultTopicQueueNums());
			requestHeader->queueId=(mq.getQueueId());
			requestHeader->sysFlag=(sysFlag);
			requestHeader->bornTimestamp=(GetCurrentTimeMillis());
			requestHeader->flag=(msg.getFlag());
			requestHeader->properties=(MessageDecoder::messageProperties2String(msg.getProperties()));

			SendResult sendResult = m_pMQClientFactory->getMQClientAPIImpl()->sendMessage(
				brokerAddr,
				mq.getBrokerName(),
				msg,
				requestHeader,
				m_pDefaultMQProducer->getSendMsgTimeout(),
				communicationMode,
				sendCallback
				);

			if (hasHook())
			{
				context.sendResult=(sendResult);
				executeHookAfter(context);
			}

			return sendResult;
		}
		catch (RemotingException& e)
		{
			if (hasHook())
			{
				context.pException=(&e);
				executeHookAfter(context);
			}
			throw;
		}
		catch (MQBrokerException& e)
		{
			if (hasHook())
			{
				context.pException=(&e);
				executeHookAfter(context);
			}
			throw;
		}
		catch (InterruptedException& e)
		{
			if (hasHook())
			{
				context.pException=(&e);
				executeHookAfter(context);
			}
			throw;
		}
	}

	THROW_MQEXCEPTION(MQClientException,"The broker[" + mq.getBrokerName() + "] not exist",-1);
}

SendResult DefaultMQProducerImpl::sendSelectImpl(Message& msg,
												 MessageQueueSelector* selector,
												 void* pArg,
												 CommunicationMode communicationMode,
												 SendCallback* sendCallback)
{
    /* modified by yu.guangjie at 2015-08-27, reason: add sendSelectImpl */
    // 有效性检查
    makeSureStateOK();
    Validators::checkMessage(msg, m_pDefaultMQProducer->getMaxMessageSize());

    TopicPublishInfo& topicPublishInfo = tryToFindTopicPublishInfo(msg.getTopic());
    if (topicPublishInfo.ok()) 
    {
        MessageQueue* mq = NULL;
        try 
        {
            mq = selector->select(topicPublishInfo.getMessageQueueList(), msg, pArg);
        }
        catch (...) 
        {
            THROW_MQEXCEPTION(MQClientException,"select message queue throwed exception.",-1);
        }

        if (mq != NULL) 
        {
            return sendKernelImpl(msg, *mq, communicationMode, sendCallback);
        }
        else 
        {
            THROW_MQEXCEPTION(MQClientException,"select message queue return null.",-1);
        }
    }
    
    std::string strMsg = "No route info for this topic: ";
    strMsg.append(msg.getTopic());
    THROW_MQEXCEPTION(MQClientException,strMsg.c_str(),-1);
}

void DefaultMQProducerImpl::makeSureStateOK()
{
	if (m_serviceState != RUNNING)
	{
		THROW_MQEXCEPTION(MQClientException,"The producer service state not OK, ",-1);
	}
}

void DefaultMQProducerImpl::checkConfig()
{
}

/* modified by yu.guangjie at 2015-08-21, reason: return TopicPublishInfo& */
TopicPublishInfo& DefaultMQProducerImpl::tryToFindTopicPublishInfo(const std::string& topic)
{
	TopicPublishInfo* info=NULL;
    std::map<std::string, TopicPublishInfo>::iterator it;
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	    it = m_topicPublishInfoTable.find(topic);
        if (it==m_topicPublishInfoTable.end()|| !it->second.ok())
    	{
    		m_topicPublishInfoTable[topic]= TopicPublishInfo();
    	}
        else
        {
            return (it->second);
        }
    
    }
    m_pMQClientFactory->updateTopicRouteInfoFromNameServer(topic);
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	    it = m_topicPublishInfoTable.find(topic);
        if (it==m_topicPublishInfoTable.end()|| !it->second.ok())
    	{
    	}
        else
        {
            return (it->second);
        }
    
    }
    m_pMQClientFactory->updateTopicRouteInfoFromNameServer(topic, true, m_pDefaultMQProducer);
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	    it = m_topicPublishInfoTable.find(topic);
        return (it->second);   
    }
    
/*
    kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	std::map<std::string, TopicPublishInfo>::iterator it = m_topicPublishInfoTable.find(topic);

	if (it==m_topicPublishInfoTable.end()|| !it->second.ok())
	{
		m_topicPublishInfoTable[topic]= TopicPublishInfo();
		m_pMQClientFactory->updateTopicRouteInfoFromNameServer(topic);
		it = m_topicPublishInfoTable.find(topic);
	}

	if (it==m_topicPublishInfoTable.end()||!it->second.ok())
	{
		m_pMQClientFactory->updateTopicRouteInfoFromNameServer(topic, true, m_pDefaultMQProducer);
		it = m_topicPublishInfoTable.find(topic);
	}


	return (it->second);
*/
}

/* modified by yu.guangjie at 2015-11-24, reason: */
MessageQueue DefaultMQProducerImpl::tryToFindTopicPublishMq(
    const std::string& topic, const std::string lastBrokerName)
{
    MessageQueue mqNull;
	TopicPublishInfo* info=NULL;
    std::map<std::string, TopicPublishInfo>::iterator it;
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	    it = m_topicPublishInfoTable.find(topic);
        if (it==m_topicPublishInfoTable.end()|| !it->second.ok())
    	{
    		m_topicPublishInfoTable[topic]= TopicPublishInfo();
    	}
        else
        {
            MessageQueue* tmpmq = it->second.selectOneMessageQueue(lastBrokerName);
            if (tmpmq != NULL)
            {
                return *tmpmq;
            }
            else 
            {
                return mqNull;
            }
        }
    
    }
    m_pMQClientFactory->updateTopicRouteInfoFromNameServer(topic);
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	    it = m_topicPublishInfoTable.find(topic);
        if (it==m_topicPublishInfoTable.end()|| !it->second.ok())
    	{
    	}
        else
        {
            MessageQueue* tmpmq = it->second.selectOneMessageQueue(lastBrokerName);
            if (tmpmq != NULL)
            {
                return *tmpmq;
            }
            else 
            {
                return mqNull;
            }
        }
    
    }
    m_pMQClientFactory->updateTopicRouteInfoFromNameServer(topic, true, m_pDefaultMQProducer);
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_topicTableLock);
	    it = m_topicPublishInfoTable.find(topic);
        MessageQueue* tmpmq = it->second.selectOneMessageQueue(lastBrokerName);
        if (tmpmq != NULL)
        {
            return *tmpmq;
        }
        else 
        {
            return mqNull;
        }
    }
}

bool DefaultMQProducerImpl::tryToCompressMessage(Message& msg)
{
	const char* body = msg.getBody();
	if (body != NULL)
	{
		if (msg.getBodyLen() >= m_pDefaultMQProducer->getCompressMsgBodyOverHowmuch())
		{
			unsigned char* pOut;
			int outLen;

			if (UtilAll::compress(body,msg.getBodyLen(),&pOut,&outLen,m_pDefaultMQProducer->getCompressLevel()))
			{
				msg.setBody((char*)pOut,outLen);
				free(pOut);

				return true;
			}
		}
	}

	return false;
}

TransactionCheckListener* DefaultMQProducerImpl::checkListener()
{
	return NULL;
}
