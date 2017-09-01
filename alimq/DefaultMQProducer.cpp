/**
* Copyright (C) 2013 kangliqiang ,kangliq@163.com
*
* Licensed under the Apache License, Version 2.0 (the "License")
{
}
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

#include "DefaultMQProducer.h"
#include <stdio.h>
#include <assert.h>
#include <set>
#include "MessageExt.h"
#include "QueryResult.h"
#include "DefaultMQProducerImpl.h"
#include "MixAll.h"
#include "MQClientException.h"

DefaultMQProducer::DefaultMQProducer()
	:m_defaultTopicQueueNums(4),
	 m_sendMsgTimeout(3000),
	 m_compressMsgBodyOverHowmuch(1024 * 4),
	 m_maxMessageSize(1024 * 128),
	 m_producerGroup (MixAll::DEFAULT_PRODUCER_GROUP),
	 m_createTopicKey(MixAll::DEFAULT_TOPIC),
	 m_retryAnotherBrokerWhenNotStoreOK(false),
	 m_compressLevel(5)
{
    char szValue[32];
    ons::ONSFactoryProperty factoryInfo;
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ProducerId, m_producerGroup.c_str());
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::PublishTopics, m_createTopicKey);
    sprintf(szValue,"%d",m_sendMsgTimeout);
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::SendMsgTimeoutMillis, szValue);
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::AccessKey, "xxxxxxxxx");
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::SecretKey, "xxxxxxxxxxxxxxxxxxx" );
    //ons::OrderProducer *pProducer = ons::ONSFactory::getInstance()->createOrderProducer(factoryInfo);
    
    m_pDefaultMQProducerImpl = new DefaultMQProducerImpl();
    m_pDefaultMQProducerImpl->setProperty(factoryInfo);
}

DefaultMQProducer::DefaultMQProducer(const std::string& producerGroup)
	:m_defaultTopicQueueNums(4),
	 m_sendMsgTimeout(3000),
	 m_compressMsgBodyOverHowmuch(1024 * 4),
	 m_maxMessageSize(1024 * 128),
	 m_producerGroup (producerGroup),
	 m_createTopicKey(MixAll::DEFAULT_TOPIC),
	 m_retryAnotherBrokerWhenNotStoreOK(false),
	 m_compressLevel(5)
{
    char szValue[32];
    ons::ONSFactoryProperty factoryInfo;
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ProducerId, m_producerGroup.c_str());
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ConsumerId, m_producerGroup.c_str());
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::PublishTopics, m_createTopicKey);
    sprintf(szValue,"%d",m_sendMsgTimeout);
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::SendMsgTimeoutMillis, szValue);
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::AccessKey, "xxxxxxxxx");
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::SecretKey, "xxxxxxxxxxxxxxxxxxx" );
    //ons::OrderProducer *pProducer = ons::ONSFactory::getInstance()->createOrderProducer(factoryInfo);
    
    m_pDefaultMQProducerImpl = new DefaultMQProducerImpl();
    m_pDefaultMQProducerImpl->setProperty(factoryInfo);
}

DefaultMQProducer::~DefaultMQProducer()
{
    if (m_pDefaultMQProducerImpl != NULL)
    {
        delete m_pDefaultMQProducerImpl;
        m_pDefaultMQProducerImpl = NULL;
    }
}

void DefaultMQProducer::start()
{
    m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::AccessKey, getAccessKey().c_str());
    m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::SecretKey, getSecretKey().c_str());

	//mjx namesrv,onsaddr modify
	if(getNamesrvAddr() != "")
    {
    	m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::NAMESRV_ADDR, getNamesrvAddr().c_str());
	}
	else
	{
		m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::ONSAddr, getNSAddr().c_str());

	}


	
    try {
	    m_pDefaultMQProducerImpl->getProducer()->start();
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

void DefaultMQProducer::shutdown()
{
    try {
	    m_pDefaultMQProducerImpl->getProducer()->shutdown();
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

std::vector<MessageQueue>* DefaultMQProducer::fetchPublishMessageQueues(const std::string& topic)
{
    m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::AccessKey, getAccessKey().c_str());
    m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::SecretKey, getSecretKey().c_str());

	//mjx namesrv,onsaddr modify
	if(getNamesrvAddr() != "")
	{
		m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::NAMESRV_ADDR, getNamesrvAddr().c_str());
	}
	else
	{
		
		m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::ONSAddr, getNSAddr().c_str());

	}

	
    std::vector<ons::MessageQueueONS> mqs;
    m_pDefaultMQProducerImpl->fetchSubscribeMessageQueues(topic, mqs);

	//mjx test modify 内部new的返回给用户层
    std::vector<MessageQueue>* mqList = new std::vector<MessageQueue>();
    std::vector<ons::MessageQueueONS>::iterator iter = mqs.begin();
    for (; iter != mqs.end(); ++iter)
    {
        MessageQueue mq(iter->getTopic(), iter->getBrokerName(), iter->getQueueId());
        mqList->push_back(mq);
    }

    return mqList;
}

SendResult DefaultMQProducer::send(Message& msg)
{
    SendResult result;
    ons::Message onsMsg;
    m_pDefaultMQProducerImpl->convertMessage(msg, onsMsg);
    
	ons::SendResultONS onsRet;
    try {
	    onsRet = m_pDefaultMQProducerImpl->getProducer()->send(onsMsg);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
    result.setMsgId(onsRet.getMessageId());

    return result;
}

void DefaultMQProducer::send(Message& msg, SendCallback* pSendCallback)
{
	 THROW_MQEXCEPTION(MQClientException, "send(Message&, SendCallback*) is unavailable",-1);
}

void DefaultMQProducer::sendOneway(Message& msg)
{
    ons::Message onsMsg;
    m_pDefaultMQProducerImpl->convertMessage(msg, onsMsg);
    
    try {
	    m_pDefaultMQProducerImpl->getProducer()->sendOneway(onsMsg);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
}

SendResult DefaultMQProducer::send(Message& msg, MessageQueue& mq)
{
    SendResult result;
    ons::Message onsMsg;
    m_pDefaultMQProducerImpl->convertMessage(msg, onsMsg);

    ons::MessageQueueONS onsMq(mq.getTopic(), mq.getBrokerName(), mq.getQueueId());
    
	ons::SendResultONS onsRet;
    try {
	    onsRet = m_pDefaultMQProducerImpl->getProducer()->send(onsMsg,onsMq);
    } catch (ons::ONSClientException& e) {
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
    result.setMsgId(onsRet.getMessageId());

    return result;
}

void DefaultMQProducer::send(Message& msg, MessageQueue& mq, SendCallback* pSendCallback)
{
	THROW_MQEXCEPTION(MQClientException, "send(Message&, MessageQueue&, SendCallback*) is unavailable",-1);
}

void DefaultMQProducer::sendOneway(Message& msg, MessageQueue& mq)
{
    THROW_MQEXCEPTION(MQClientException, "sendOneway(Message&, MessageQueue&) is unavailable",-1);
}

SendResult DefaultMQProducer::send(Message& msg, MessageQueueSelector* pSelector, void* arg)
{
    THROW_MQEXCEPTION(MQClientException, "send(Message&, MessageQueueSelector*, void*) is unavailable",-1);
}

void DefaultMQProducer::send(Message& msg,
							 MessageQueueSelector* pSelector,
							 void* arg,
							 SendCallback* pSendCallback)
{    
    THROW_MQEXCEPTION(MQClientException, "send(Message&, MessageQueueSelector*, void*, SendCallback*) is unavailable",-1);
}

void DefaultMQProducer::sendOneway(Message& msg, MessageQueueSelector* pSelector, void* arg)
{
    THROW_MQEXCEPTION(MQClientException, "sendOneway(Message&, MessageQueueSelector*, void*) is unavailable",-1);
}

TransactionSendResult DefaultMQProducer::sendMessageInTransaction(Message& msg,
		LocalTransactionExecuter* tranExecuter, void* arg)
{
	THROW_MQEXCEPTION(MQClientException,
		"sendMessageInTransaction not implement, please use TransactionMQProducer class",-1);
	TransactionSendResult result;

	return result;
}

void DefaultMQProducer::createTopic(const std::string& key, const std::string& newTopic, int queueNum)
{
    THROW_MQEXCEPTION(MQClientException, "createTopic(string&, string&, int) is unavailable",-1);
}

long long DefaultMQProducer::searchOffset(const MessageQueue& mq, long long timestamp)
{
    THROW_MQEXCEPTION(MQClientException, "searchOffset(MessageQueue&, long long) is unavailable",-1);
	return -1;
}

long long DefaultMQProducer::maxOffset(const MessageQueue& mq)
{
    THROW_MQEXCEPTION(MQClientException, "maxOffset(MessageQueue&) is unavailable",-1);
	return -1;
}

long long DefaultMQProducer::minOffset(const MessageQueue& mq)
{
	THROW_MQEXCEPTION(MQClientException, "minOffset(MessageQueue&) is unavailable",-1);
	return -1;
}

long long DefaultMQProducer::earliestMsgStoreTime(const MessageQueue& mq)
{
    THROW_MQEXCEPTION(MQClientException, "earliestMsgStoreTime(MessageQueue&) is unavailable",-1);
	return -1;
}

MessageExt* DefaultMQProducer::viewMessage(const std::string& msgId)
{
    THROW_MQEXCEPTION(MQClientException, "viewMessage(string&) is unavailable",-1);
	return NULL;
}

QueryResult DefaultMQProducer::queryMessage(const std::string& topic,
											const std::string& key,
											int maxNum,
											long long begin,
											long long end)
{
    THROW_MQEXCEPTION(MQClientException, "queryMessage(string&,string&,int,llong,llong) is unavailable",-1);
}

std::string DefaultMQProducer::getProducerGroup()
{
	return m_producerGroup;
}

void DefaultMQProducer::setProducerGroup(const std::string& producerGroup)
{
	m_producerGroup = producerGroup;
    m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::ProducerId, m_producerGroup.c_str());
}

std::string DefaultMQProducer::getCreateTopicKey()
{
	return m_createTopicKey;
}

void DefaultMQProducer::setCreateTopicKey(const std::string& createTopicKey)
{
	m_createTopicKey = createTopicKey;
}

int DefaultMQProducer::getSendMsgTimeout()
{
	return m_sendMsgTimeout;
}

void DefaultMQProducer::setSendMsgTimeout(int sendMsgTimeout)
{
    char szValue[32];
	m_sendMsgTimeout = sendMsgTimeout;
    sprintf(szValue,"%d",m_sendMsgTimeout);
    m_pDefaultMQProducerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::SendMsgTimeoutMillis, szValue);
}

int DefaultMQProducer::getCompressMsgBodyOverHowmuch()
{
	return m_compressMsgBodyOverHowmuch;
}

void DefaultMQProducer::setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch)
{
	m_compressMsgBodyOverHowmuch = compressMsgBodyOverHowmuch;
}

DefaultMQProducerImpl* DefaultMQProducer::getDefaultMQProducerImpl()
{
	return m_pDefaultMQProducerImpl;
}

bool DefaultMQProducer::isRetryAnotherBrokerWhenNotStoreOK()
{
	return m_retryAnotherBrokerWhenNotStoreOK;
}

void DefaultMQProducer::setRetryAnotherBrokerWhenNotStoreOK(bool retryAnotherBrokerWhenNotStoreOK)
{
	m_retryAnotherBrokerWhenNotStoreOK = retryAnotherBrokerWhenNotStoreOK;
}

int DefaultMQProducer::getMaxMessageSize()
{
	return m_maxMessageSize;
}

void DefaultMQProducer::setMaxMessageSize(int maxMessageSize)
{
	m_maxMessageSize = maxMessageSize;
}

int DefaultMQProducer::getDefaultTopicQueueNums()
{
	return m_defaultTopicQueueNums;
}

void DefaultMQProducer::setDefaultTopicQueueNums(int defaultTopicQueueNums)
{
	m_defaultTopicQueueNums = defaultTopicQueueNums;
}

int DefaultMQProducer::getCompressLevel()
{
	return m_compressLevel;
}

void DefaultMQProducer::setCompressLevel( int compressLevel )
{
	assert(compressLevel >=0 && compressLevel <= 9 || compressLevel == -1);

	m_compressLevel = compressLevel;
}
