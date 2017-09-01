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

#include "DefaultMQPullConsumer.h"

#include <list>
#include <string>

#include "QueryResult.h"
#include "MessageQueue.h"
#include "MessageExt.h"
#include "ClientConfig.h"
#include "DefaultMQPullConsumerImpl.h"
#include "MixAll.h"
#include "MQClientException.h"

DefaultMQPullConsumer::DefaultMQPullConsumer()
	:m_consumerGroup(MixAll::DEFAULT_CONSUMER_GROUP),
	m_brokerSuspendMaxTimeMillis(1000 * 20),
	m_consumerTimeoutMillisWhenSuspend(1000 * 30),
	m_consumerPullTimeoutMillis(1000 * 10),
	m_messageModel(BROADCASTING),
	m_pMessageQueueListener(NULL),
	m_pOffsetStore(NULL),
	m_pAllocateMessageQueueStrategy(NULL)
{
    ons::ONSFactoryProperty factoryInfo;
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ConsumerId, m_consumerGroup.c_str());
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::PublishTopics, m_createTopicKey);
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::MessageModel, ons::ONSFactoryProperty::BROADCASTING);
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::AccessKey, "xxxxxxxxx");
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::SecretKey, "xxxxxxxxxxxxxxxxxxx" );
    //ons::OrderProducer *pProducer = ons::ONSFactory::getInstance()->createOrderProducer(factoryInfo);

	m_pDefaultMQPullConsumerImpl = new DefaultMQPullConsumerImpl();
    m_pDefaultMQPullConsumerImpl->setProperty(factoryInfo);
}

DefaultMQPullConsumer::DefaultMQPullConsumer(const std::string& consumerGroup)
	:m_consumerGroup(consumerGroup),
	m_brokerSuspendMaxTimeMillis(1000 * 20),
	m_consumerTimeoutMillisWhenSuspend(1000 * 30),
	m_consumerPullTimeoutMillis(1000 * 10),
	m_messageModel(BROADCASTING),
	m_pMessageQueueListener(NULL),
	m_pOffsetStore(NULL),
	m_pAllocateMessageQueueStrategy(NULL)
{
    ons::ONSFactoryProperty factoryInfo;
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ConsumerId, m_consumerGroup.c_str());
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::PublishTopics, m_createTopicKey);
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::MessageModel, ons::ONSFactoryProperty::BROADCASTING);
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::AccessKey, "xxxxxxxxx");
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::SecretKey, "xxxxxxxxxxxxxxxxxxx" );
    //ons::OrderProducer *pProducer = ons::ONSFactory::getInstance()->createOrderProducer(factoryInfo);

	m_pDefaultMQPullConsumerImpl = new DefaultMQPullConsumerImpl();
    m_pDefaultMQPullConsumerImpl->setProperty(factoryInfo);
}

DefaultMQPullConsumer::~DefaultMQPullConsumer()
{
    if (m_pDefaultMQPullConsumerImpl != NULL)
    {
        delete m_pDefaultMQPullConsumerImpl;
        m_pDefaultMQPullConsumerImpl = NULL;
    }
}

//MQAdmin
void DefaultMQPullConsumer::createTopic(const std::string& key, const std::string& newTopic, int queueNum)
{
	THROW_MQEXCEPTION(MQClientException, "createTopic(string&, string&, int) is unavailable",-1);
}

long long DefaultMQPullConsumer::searchOffset(const MessageQueue& mq, long long timestamp)
{
    ons::MessageQueueONS mqONS(mq.getTopic(),mq.getBrokerName(),mq.getQueueId());

    try {
	    return m_pDefaultMQPullConsumerImpl->getPullConsumer()->searchOffset(mqONS, timestamp);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

long long DefaultMQPullConsumer::maxOffset(const MessageQueue& mq)
{
    ons::MessageQueueONS mqONS(mq.getTopic(),mq.getBrokerName(),mq.getQueueId());
    
    try {
	    return m_pDefaultMQPullConsumerImpl->getPullConsumer()->maxOffset(mqONS);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

long long DefaultMQPullConsumer::minOffset(const MessageQueue& mq)
{
	ons::MessageQueueONS mqONS(mq.getTopic(),mq.getBrokerName(),mq.getQueueId());
    
    try {
	    return m_pDefaultMQPullConsumerImpl->getPullConsumer()->minOffset(mqONS);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

long long DefaultMQPullConsumer::earliestMsgStoreTime(const MessageQueue& mq)
{
	THROW_MQEXCEPTION(MQClientException, "earliestMsgStoreTime(MessageQueue&) is unavailable",-1);
	return -1;      
}

MessageExt* DefaultMQPullConsumer::viewMessage(const std::string& msgId)
{
	THROW_MQEXCEPTION(MQClientException, "viewMessage(string&) is unavailable",-1);
	return NULL;
}

QueryResult DefaultMQPullConsumer::queryMessage(const std::string& topic,
	const std::string&  key,
	int maxNum,
	long long begin,
	long long end)
{
	THROW_MQEXCEPTION(MQClientException, "queryMessage(string&,string&,int,llong,llong) is unavailable",-1);
}

AllocateMessageQueueStrategy* DefaultMQPullConsumer::getAllocateMessageQueueStrategy()
{
	return m_pAllocateMessageQueueStrategy;
}

void DefaultMQPullConsumer::setAllocateMessageQueueStrategy(AllocateMessageQueueStrategy* pAllocateMessageQueueStrategy)
{
	m_pAllocateMessageQueueStrategy = pAllocateMessageQueueStrategy;
}

int DefaultMQPullConsumer::getBrokerSuspendMaxTimeMillis()
{
	return m_brokerSuspendMaxTimeMillis;
}

void DefaultMQPullConsumer::setBrokerSuspendMaxTimeMillis(int brokerSuspendMaxTimeMillis)
{
	m_brokerSuspendMaxTimeMillis = brokerSuspendMaxTimeMillis;
}

std::string DefaultMQPullConsumer::getConsumerGroup()
{
	return m_consumerGroup;
}

void DefaultMQPullConsumer::setConsumerGroup(const std::string& consumerGroup)
{
	m_consumerGroup = consumerGroup;
    m_pDefaultMQPullConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::ConsumerId, m_consumerGroup.c_str());
}

int DefaultMQPullConsumer::getConsumerPullTimeoutMillis()
{
	return m_consumerPullTimeoutMillis;
}

void DefaultMQPullConsumer::setConsumerPullTimeoutMillis(int consumerPullTimeoutMillis)
{
	m_consumerPullTimeoutMillis = consumerPullTimeoutMillis;
}

int DefaultMQPullConsumer::getConsumerTimeoutMillisWhenSuspend()
{
	return m_consumerTimeoutMillisWhenSuspend;
}

void DefaultMQPullConsumer::setConsumerTimeoutMillisWhenSuspend(int consumerTimeoutMillisWhenSuspend)
{
	m_consumerTimeoutMillisWhenSuspend = consumerTimeoutMillisWhenSuspend;
}

MessageModel DefaultMQPullConsumer::getMessageModel()
{
	return m_messageModel;
}

void DefaultMQPullConsumer::setMessageModel(MessageModel messageModel)
{
	m_messageModel = messageModel;
    if (m_messageModel == BROADCASTING)
    {
        m_pDefaultMQPullConsumerImpl->getProperty()->setFactoryProperty(
            ons::ONSFactoryProperty::MessageModel, ons::ONSFactoryProperty::BROADCASTING);
    }
    else if (m_messageModel == CLUSTERING)
    {
        m_pDefaultMQPullConsumerImpl->getProperty()->setFactoryProperty(
            ons::ONSFactoryProperty::MessageModel, ons::ONSFactoryProperty::CLUSTERING);
    }
}

MessageQueueListener* DefaultMQPullConsumer::getMessageQueueListener()
{
	return m_pMessageQueueListener;
}

void DefaultMQPullConsumer::setMessageQueueListener(MessageQueueListener* pMessageQueueListener)
{
	m_pMessageQueueListener = pMessageQueueListener;
}

std::set<std::string> DefaultMQPullConsumer::getRegisterTopics()
{
	return m_registerTopics;
}

void DefaultMQPullConsumer::setRegisterTopics( std::set<std::string> registerTopics)
{
	m_registerTopics = registerTopics;
}

//MQConsumer
void DefaultMQPullConsumer::sendMessageBack(MessageExt& msg, int delayLevel)
{
    THROW_MQEXCEPTION(MQClientException, "sendMessageBack(MessageExt&,int) is unavailable",-1);
}

std::set<MessageQueue>* DefaultMQPullConsumer::fetchSubscribeMessageQueues(const std::string& topic)
{
    std::vector<ons::MessageQueueONS> mqs;
	
    try {
	    m_pDefaultMQPullConsumerImpl->getPullConsumer()->fetchSubscribeMessageQueues(topic, mqs);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
	//mjx test modify 内部new，返回应用层，要释放
    std::set<MessageQueue>* mqList = new std::set<MessageQueue>();
    std::vector<ons::MessageQueueONS>::iterator iter = mqs.begin();
    for (; iter != mqs.end(); ++iter)
    {
        MessageQueue mq(iter->getTopic(), iter->getBrokerName(), iter->getQueueId());
        mqList->insert(mq);
    }

    return mqList;
}

void DefaultMQPullConsumer::start()
{
    m_pDefaultMQPullConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::AccessKey, getAccessKey().c_str());
    m_pDefaultMQPullConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::SecretKey, getSecretKey().c_str());

	//mjx namesrv,onsaddr modify
	if(getNamesrvAddr() != "")
	{
		m_pDefaultMQPullConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::NAMESRV_ADDR, getNamesrvAddr().c_str());
	}
	else
	{
		
		m_pDefaultMQPullConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::ONSAddr, getNSAddr().c_str());
	}

    try {
	    m_pDefaultMQPullConsumerImpl->getPullConsumer()->start();
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

void DefaultMQPullConsumer::shutdown()
{
    try {
	    m_pDefaultMQPullConsumerImpl->getPullConsumer()->shutdown();
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}
//MQConsumer end

//MQPullConsumer
void DefaultMQPullConsumer::registerMessageQueueListener(const std::string& topic, MessageQueueListener* pListener)
{
	m_registerTopics.insert(topic);

	if (pListener) 
	{
		m_pMessageQueueListener = pListener;
	}
}

PullResult* DefaultMQPullConsumer::pull(MessageQueue& mq, const std::string& subExpression, long long offset,int maxNums)
{
    ons::MessageQueueONS mqONS(mq.getTopic(),mq.getBrokerName(),mq.getQueueId());

    try {
	    ons::PullResultONS retONS = m_pDefaultMQPullConsumerImpl->getPullConsumer()->pull(
            mqONS, subExpression, offset, maxNums);
		//mjx test modify
        return m_pDefaultMQPullConsumerImpl->convertPullResult(retONS,mq.getQueueId());
		
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

void DefaultMQPullConsumer::pull(MessageQueue& mq, const std::string& subExpression, long long offset, int maxNums,PullCallback* pPullCallback)
{
    THROW_MQEXCEPTION(MQClientException, "pull(MessageQueue&,string&,llong,int,PullCallback*) is unavailable",-1);
}

PullResult* DefaultMQPullConsumer::pullBlockIfNotFound(MessageQueue& mq, const std::string& subExpression, long long offset,int maxNums)
{
    THROW_MQEXCEPTION(MQClientException, "pullBlockIfNotFound(MessageQueue&,string&,llong,int) is unavailable",-1);
}

void DefaultMQPullConsumer::pullBlockIfNotFound(MessageQueue& mq,
	const std::string& subExpression,
	long long offset,
	int maxNums,
	PullCallback* pPullCallback)
{
    THROW_MQEXCEPTION(MQClientException, "pullBlockIfNotFound(MessageQueue&,string&,llong,int,PullCallback*) is unavailable",-1);
}

void DefaultMQPullConsumer::updateConsumeOffset(MessageQueue& mq, long long offset)
{
    ons::MessageQueueONS mqONS(mq.getTopic(),mq.getBrokerName(),mq.getQueueId());
    
    try {
	    return m_pDefaultMQPullConsumerImpl->getPullConsumer()->updateConsumeOffset(mqONS, offset);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

long long DefaultMQPullConsumer::fetchConsumeOffset(MessageQueue& mq, bool fromStore)
{
    ons::MessageQueueONS mqONS(mq.getTopic(),mq.getBrokerName(),mq.getQueueId());
    
    try {
	    return m_pDefaultMQPullConsumerImpl->getPullConsumer()->fetchConsumeOffset(mqONS, fromStore);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

void DefaultMQPullConsumer::persistConsumeOffset(MessageQueue& mq)
{
    ons::MessageQueueONS mqONS(mq.getTopic(),mq.getBrokerName(),mq.getQueueId());
    
    try {
	    return m_pDefaultMQPullConsumerImpl->getPullConsumer()->persistConsumerOffset4PullConsumer(mqONS);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

std::set<MessageQueue> DefaultMQPullConsumer::fetchMessageQueuesInBalance(const std::string& topic)
{
    THROW_MQEXCEPTION(MQClientException, "fetchMessageQueuesInBalance(string&) is unavailable",-1);
}
//MQPullConsumer end

OffsetStore* DefaultMQPullConsumer::getOffsetStore()
{
	return m_pOffsetStore;
}

void DefaultMQPullConsumer::setOffsetStore(OffsetStore* offsetStore)
{
	m_pOffsetStore = offsetStore;
}

DefaultMQPullConsumerImpl* DefaultMQPullConsumer::getDefaultMQPullConsumerImpl()
{
	return m_pDefaultMQPullConsumerImpl;
}

void DefaultMQPullConsumer::setTcpTimeoutMilliseconds(int milliseconds)
{
    THROW_MQEXCEPTION(MQClientException, "setTcpTimeoutMilliseconds(int) is unavailable",-1);
}

int DefaultMQPullConsumer::getTcpTimeoutMilliseconds()
{
    THROW_MQEXCEPTION(MQClientException, "getTcpTimeoutMilliseconds() is unavailable",-1);
    return -1;
}

