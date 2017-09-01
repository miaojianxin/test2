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

#include "DefaultMQPushConsumer.h"
#include <stdio.h>
#include <list>
#include <string>

#include "QueryResult.h"
#include "MessageQueue.h"
#include "MessageExt.h"
#include "ClientConfig.h"
#include "DefaultMQPushConsumerImpl.h"
#include "MixAll.h"
#include "MQClientException.h"


DefaultMQPushConsumer::DefaultMQPushConsumer()
{
	m_consumerGroup = MixAll::DEFAULT_CONSUMER_GROUP;
	m_messageModel = CLUSTERING;
	m_consumeFromWhere = CONSUME_FROM_LAST_OFFSET;
    m_pAllocateMessageQueueStrategy = NULL;
	m_pMessageListener = NULL;
	m_consumeThreadMin = 10;
	m_consumeThreadMax = 20;
	m_consumeConcurrentlyMaxSpan = 2000;
	m_pullThresholdForQueue = 1000;
	m_pullInterval = 0;
	m_consumeMessageBatchMaxSize = 1;
	m_pullBatchSize = 32;
    m_pOffsetStore = NULL;

    ons::ONSFactoryProperty factoryInfo;
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ConsumerId, m_consumerGroup.c_str());
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::PublishTopics, m_createTopicKey);
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::MessageModel, ons::ONSFactoryProperty::CLUSTERING);
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::AccessKey, "xxxxxxxxx");
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::SecretKey, "xxxxxxxxxxxxxxxxxxx" );
    char szValue[32];
    sprintf(szValue,"%d",m_consumeThreadMax);
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ConsumeThreadNums, szValue);

	m_pDefaultMQPushConsumerImpl = new DefaultMQPushConsumerImpl();
    m_pDefaultMQPushConsumerImpl->setProperty(factoryInfo);

	/* modify by liang.haibo 2016-10-09 
     * Consumer第一次启动时，如果回溯消费，默认回溯到哪个时间点，数据格式如下，时间精度秒：<br>
     * 20131223171201<br>
     * 表示2013年12月23日17点12分01秒<br>
     * 默认回溯到相对启动时间的半小时前
	*/
	//m_consumeTimestamp = std::string(CDate::FormatGMTimeToString( CDate::GetSysDateTime() -  30 * 60, false));
}

DefaultMQPushConsumer::DefaultMQPushConsumer(const std::string& consumerGroup)
{
	m_consumerGroup = consumerGroup;
	m_messageModel = CLUSTERING;
	m_consumeFromWhere = CONSUME_FROM_LAST_OFFSET;
    /* modified by yu.guangjie at 2015-08-13, reason: */
	m_pAllocateMessageQueueStrategy  = NULL;
	m_pMessageListener = NULL;
	m_consumeThreadMin = 10;
	m_consumeThreadMax = 20;
	m_consumeConcurrentlyMaxSpan = 2000;
	m_pullThresholdForQueue = 1000;
	m_pullInterval = 0;
	m_consumeMessageBatchMaxSize = 1;
	m_pullBatchSize = 32;
    m_pOffsetStore = NULL;

    ons::ONSFactoryProperty factoryInfo;
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ConsumerId, m_consumerGroup.c_str());
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::PublishTopics, m_createTopicKey);
    
    factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::MessageModel, ons::ONSFactoryProperty::CLUSTERING);


	//factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::AccessKey, "xxxxxxxxx");
    //factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::SecretKey, "xxxxxxxxxxxxxxxxxxx" );
    
    char szValue[32];
    sprintf(szValue,"%d",m_consumeThreadMax);
	
   factoryInfo.setFactoryProperty(ons::ONSFactoryProperty::ConsumeThreadNums, szValue);

	m_pDefaultMQPushConsumerImpl = new DefaultMQPushConsumerImpl();
    m_pDefaultMQPushConsumerImpl->setProperty(factoryInfo);

	/* modify by liang.haibo 2016-10-09 
     * Consumer第一次启动时，如果回溯消费，默认回溯到哪个时间点，数据格式如下，时间精度秒：<br>
     * 20131223171201<br>
     * 表示2013年12月23日17点12分01秒<br>
     * 默认回溯到相对启动时间的半小时前
	*/
	//m_consumeTimestamp = std::string(CDate::FormatGMTimeToString( CDate::GetSysDateTime() -  30 * 60, false));
}

DefaultMQPushConsumer::~DefaultMQPushConsumer()
{
    if(m_pDefaultMQPushConsumerImpl != NULL)
    {
        delete m_pDefaultMQPushConsumerImpl;
        m_pDefaultMQPushConsumerImpl = NULL;
    }
}

//MQAdmin
void DefaultMQPushConsumer::createTopic(const std::string& key, const std::string& newTopic, int queueNum)
{
    THROW_MQEXCEPTION(MQClientException, "createTopic(string&, string&, int) is unavailable",-1);
}

long long DefaultMQPushConsumer::searchOffset(const MessageQueue& mq, long long timestamp)
{
	THROW_MQEXCEPTION(MQClientException, "searchOffset(MessageQueue&, llong) is unavailable",-1);
}

long long DefaultMQPushConsumer::maxOffset(const MessageQueue& mq)
{
    THROW_MQEXCEPTION(MQClientException, "maxOffset(MessageQueue&) is unavailable",-1);
}

long long DefaultMQPushConsumer::minOffset(const MessageQueue& mq)
{
    THROW_MQEXCEPTION(MQClientException, "minOffset(MessageQueue&) is unavailable",-1);
}

long long DefaultMQPushConsumer::earliestMsgStoreTime(const MessageQueue& mq)
{
    THROW_MQEXCEPTION(MQClientException, "earliestMsgStoreTime(MessageQueue&) is unavailable",-1);
}

MessageExt* DefaultMQPushConsumer::viewMessage(const std::string& msgId)
{
	THROW_MQEXCEPTION(MQClientException, "viewMessage(string&) is unavailable",-1);
}

QueryResult DefaultMQPushConsumer::queryMessage(const std::string& topic,
		const std::string&  key,
		int maxNum,
		long long begin,
		long long end)
{
	THROW_MQEXCEPTION(MQClientException, "queryMessage(string&,string&,int,llong,llong) is unavailable",-1);
}
// MQadmin end

AllocateMessageQueueStrategy* DefaultMQPushConsumer::getAllocateMessageQueueStrategy()
{
	return m_pAllocateMessageQueueStrategy;
}

void DefaultMQPushConsumer::setAllocateMessageQueueStrategy(AllocateMessageQueueStrategy* pAllocateMessageQueueStrategy)
{
	m_pAllocateMessageQueueStrategy = pAllocateMessageQueueStrategy;
}

int DefaultMQPushConsumer::getConsumeConcurrentlyMaxSpan()
{
	return m_consumeConcurrentlyMaxSpan;
}

void DefaultMQPushConsumer::setConsumeConcurrentlyMaxSpan(int consumeConcurrentlyMaxSpan)
{
	m_consumeConcurrentlyMaxSpan = consumeConcurrentlyMaxSpan;
}

ConsumeFromWhere DefaultMQPushConsumer::getConsumeFromWhere()
{
	return m_consumeFromWhere;
}

void DefaultMQPushConsumer::setConsumeFromWhere(ConsumeFromWhere consumeFromWhere)
{
	m_consumeFromWhere = consumeFromWhere;
}

int DefaultMQPushConsumer::getConsumeMessageBatchMaxSize()
{
	return m_consumeMessageBatchMaxSize;
}

void DefaultMQPushConsumer::setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize)
{
	m_consumeMessageBatchMaxSize = consumeMessageBatchMaxSize;
}

std::string DefaultMQPushConsumer::getConsumerGroup()
{
	return m_consumerGroup;
}

void DefaultMQPushConsumer::setConsumerGroup(const std::string& consumerGroup)
{
	m_consumerGroup = consumerGroup;
    m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::ConsumerId, m_consumerGroup.c_str());
}

/** modify by liang.haibo 2016-10-09
** consume timestamp getter and setter 
**/
std::string DefaultMQPushConsumer::getConsumeTimestamp() {
	return m_consumeTimestamp;
}

void DefaultMQPushConsumer::setConsumeTimestamp(const std::string& consumeTimestamp) {
	m_consumeTimestamp = consumeTimestamp;
}
/* modify end liang.haibo 2016-10-09*/

int DefaultMQPushConsumer::getConsumeThreadMax()
{
	return m_consumeThreadMax;
}

void DefaultMQPushConsumer::setConsumeThreadMax(int consumeThreadMax)
{
	m_consumeThreadMax = consumeThreadMax;
    char szValue[32];
    sprintf(szValue,"%d",m_consumeThreadMax);
    m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(
        ons::ONSFactoryProperty::ConsumeThreadNums, szValue);
}

int DefaultMQPushConsumer::getConsumeThreadMin()
{
	return m_consumeThreadMin;
}

void DefaultMQPushConsumer::setConsumeThreadMin(int consumeThreadMin)
{
	m_consumeThreadMin = consumeThreadMin;
}

DefaultMQPushConsumerImpl* DefaultMQPushConsumer::getDefaultMQPushConsumerImpl()
{
	return m_pDefaultMQPushConsumerImpl;
}

void DefaultMQPushConsumer::setTcpTimeoutMilliseconds(int milliseconds)
{
    THROW_MQEXCEPTION(MQClientException, "setTcpTimeoutMilliseconds(int) is unavailable",-1);
}

int DefaultMQPushConsumer::getTcpTimeoutMilliseconds()
{
	THROW_MQEXCEPTION(MQClientException, "getTcpTimeoutMilliseconds() is unavailable",-1);
}

MessageListener* DefaultMQPushConsumer::getMessageListener()
{
	return m_pMessageListener;
}

void DefaultMQPushConsumer::setMessageListener(MessageListener* pMessageListener)
{
	m_pMessageListener = pMessageListener;
}

MessageModel DefaultMQPushConsumer::getMessageModel()
{
	return m_messageModel;
}

void DefaultMQPushConsumer::setMessageModel(MessageModel messageModel)
{
	m_messageModel = messageModel;
    if (m_messageModel == BROADCASTING)
    {
        m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(
            ons::ONSFactoryProperty::MessageModel, ons::ONSFactoryProperty::BROADCASTING);
    }
    else if (m_messageModel == CLUSTERING)
    {
        m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(
            ons::ONSFactoryProperty::MessageModel, ons::ONSFactoryProperty::CLUSTERING);
    }
}

int DefaultMQPushConsumer::getPullBatchSize()
{
	return m_pullBatchSize;
}

void DefaultMQPushConsumer::setPullBatchSize(int pullBatchSize)
{
	m_pullBatchSize = pullBatchSize;
}

long DefaultMQPushConsumer::getPullInterval()
{
	return m_pullInterval;
}

void DefaultMQPushConsumer::setPullInterval(long pullInterval)
{
	m_pullInterval = pullInterval;
}

int DefaultMQPushConsumer::getPullThresholdForQueue()
{
	return m_pullThresholdForQueue;
}

void DefaultMQPushConsumer::setPullThresholdForQueue(int pullThresholdForQueue)
{
	m_pullThresholdForQueue = pullThresholdForQueue;
}

std::map<std::string, std::string>& DefaultMQPushConsumer::getSubscription()
{
	return m_subscription;
}

void DefaultMQPushConsumer::setSubscription(const std::map<std::string, std::string>& subscription)
{
	m_subscription = subscription;
}

//MQConsumer
void DefaultMQPushConsumer::sendMessageBack(MessageExt& msg, int delayLevel)
{
    THROW_MQEXCEPTION(MQClientException, "sendMessageBack(MessageExt&,int) is unavailable",-1);
}

std::set<MessageQueue>* DefaultMQPushConsumer::fetchSubscribeMessageQueues(const std::string& topic)
{
    THROW_MQEXCEPTION(MQClientException, "fetchSubscribeMessageQueues(string&) is unavailable",-1);
}


void DefaultMQPushConsumer::start()
{
    m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::AccessKey, getAccessKey().c_str());
    m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::SecretKey, getSecretKey().c_str());
	//mjx namesrv,onsaddr modify
	if(getNamesrvAddr() != "")
	{
		m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::NAMESRV_ADDR, getNamesrvAddr().c_str());
	}
	else
	{
		
		m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::ONSAddr, getNSAddr().c_str());
	}


	//mjx test modify
	if (m_pDefaultMQPushConsumerImpl->isConsumeOrderly())
	{
	   try 
		{
		    m_pDefaultMQPushConsumerImpl->getOrderConsumer()->start();
	    } 

		catch (ons::ONSClientException& e) {
	        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
	    }
	}
	else
	{
		 try 
		{
		    m_pDefaultMQPushConsumerImpl->getPushConsumer()->start();
	    } 

		catch (ons::ONSClientException& e) {
	        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
	    }

	}




	
}

void DefaultMQPushConsumer::shutdown()
{

	//mjx test modify
	if (m_pDefaultMQPushConsumerImpl->isConsumeOrderly())
    {
	    try {
		    m_pDefaultMQPushConsumerImpl->getOrderConsumer()->shutdown();
	    } catch (ons::ONSClientException& e) {
	        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
	    }
	}
	else
	{

		try {
		    m_pDefaultMQPushConsumerImpl->getPushConsumer()->shutdown();
	    } catch (ons::ONSClientException& e) {
	        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
	    }

	}
}

//MQConsumer end

//MQPushConsumer
void DefaultMQPushConsumer::registerMessageListener(MessageListener* pMessageListener)
{
	m_pMessageListener = pMessageListener;
	m_pDefaultMQPushConsumerImpl->setListener(pMessageListener);
}

void DefaultMQPushConsumer::subscribe(const std::string& topic, const std::string& subExpression)
{

	//mjx test modify
	//设置的factoy属性只有在阿里api createconsumer时候才能传递过去
    //一旦创建了，后期再怎么设置factory，设置的都无效了
    //在所有可能createconsumer之前，都要把factory属性全部设置到位
    m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::AccessKey, getAccessKey().c_str());
    m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::SecretKey, getSecretKey().c_str());

	//mjx namesrv,onsaddr modify
	if(getNamesrvAddr() != "")
	{
		m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::NAMESRV_ADDR, getNamesrvAddr().c_str());
	}
	else
	{
		
		m_pDefaultMQPushConsumerImpl->getProperty()->setFactoryProperty(ons::ONSFactoryProperty::ONSAddr, getNSAddr().c_str());
	}




    if (m_pDefaultMQPushConsumerImpl->isConsumeOrderly())
    {
        try {
    	    m_pDefaultMQPushConsumerImpl->getOrderConsumer()->subscribe(
                topic.c_str(), subExpression.c_str(),m_pDefaultMQPushConsumerImpl->getOrderListener());
        } catch (ons::ONSClientException& e) {
            THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
        }
    }
    else 
    {
        try {
    	    m_pDefaultMQPushConsumerImpl->getPushConsumer()->subscribe(
                topic.c_str(), subExpression.c_str(),m_pDefaultMQPushConsumerImpl->getListener());
        } catch (ons::ONSClientException& e) {
            THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
        }
    }
}

void DefaultMQPushConsumer::unsubscribe(const std::string& topic)
{
    THROW_MQEXCEPTION(MQClientException, "unsubscribe(string&) is unavailable",-1);
}

void DefaultMQPushConsumer::updateCorePoolSize(int corePoolSize)
{
    THROW_MQEXCEPTION(MQClientException, "updateCorePoolSize(int) is unavailable",-1);
}

void DefaultMQPushConsumer::suspend()
{
    THROW_MQEXCEPTION(MQClientException, "suspend() is unavailable",-1);
}

void DefaultMQPushConsumer::resume()
{
    THROW_MQEXCEPTION(MQClientException, "resume() is unavailable",-1);
}
//MQPushConsumer end

OffsetStore* DefaultMQPushConsumer::getOffsetStore()
{
	return m_pOffsetStore;
}

void DefaultMQPushConsumer::setOffsetStore(OffsetStore* pOffsetStore)
{
	m_pOffsetStore = pOffsetStore;
}
