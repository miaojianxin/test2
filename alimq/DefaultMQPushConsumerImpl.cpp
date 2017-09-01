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

OnsOrderListener::OnsOrderListener()
{
	m_pZmqListener = NULL;
}

OnsOrderListener::~OnsOrderListener()
{

}
    
OrderAction OnsOrderListener::consume(ons::Message& message, ons::ConsumeOrderContext& context)
{
    std::list<MessageExt *> msgList;
    MessageQueue mq;
    ConsumeOrderlyContext zmqContext(mq);

    MessageExt* msg = new MessageExt();
    /* set properties first */
    std::map<std::string, std::string> tmpMap = message.getUserProperties();
    msg->setProperties(tmpMap);  
	
    msg->setMsgId(message.getMsgID());
    msg->setTopic(message.getTopic());
    msg->setTags(message.getTag());
    msg->setKeys(message.getKey());

	//mjx test modify
	//此处需要修改 二进制数据，不对
   // const char* body = message.getBody();
   // msg->setBody(body, strlen(body));

    msg->setBody(message.getMsgBody().c_str(), message.getMsgBody().length());

	//阿里的message没有把queueid带过来
    //msg->setQueueId(iter->getQueueId);
	//mjx test modify
	msg->setQueueId(-1);
	
    msg->setQueueOffset(message.getQueueOffset());
    msg->setReconsumeTimes(message.getReconsumeTimes());
    msg->setStoreTimestamp(message.getStoreTimestamp());
    msg->setBornTimestamp(message.getStartDeliverTime());
    
    msgList.push_back(msg);
	
    /* consume message */
    ConsumeOrderlyStatus zmqStatus = m_pZmqListener->consumeMessage(msgList, zmqContext);

	
    /* destroy MessageExt */
    delete msg;

    if (zmqStatus == SUCCESS)
    {
        return Success;
    }
    else
    {
        return Suspend;
    }
}

void OnsOrderListener::setZmqListener(MessageListenerOrderly* listener)
{
    m_pZmqListener = listener;
}

MessageListenerOrderly* OnsOrderListener::getZmqListener()
{
    return m_pZmqListener;
}


OnsMessageListener::OnsMessageListener()
{
	m_pZmqListener = NULL;
}

OnsMessageListener::~OnsMessageListener()
{

}

Action OnsMessageListener::consume(ons::Message& message, ons::ConsumeContext& context)
{
    std::list<MessageExt *> msgList;
    MessageQueue mq;
    ConsumeConcurrentlyContext zmqContext(mq);

    MessageExt* msg = new MessageExt();
    /* set properties first */
    std::map<std::string, std::string> tmpMap = message.getUserProperties();
    msg->setProperties(tmpMap);        
    msg->setMsgId(message.getMsgID());
    msg->setTopic(message.getTopic());
    msg->setTags(message.getTag());
    msg->setKeys(message.getKey());

	//mjx test modify
	//此处需要修改,二进制数据不对
    //const char* body = message.getBody();
    //msg->setBody(body, strlen(body));
     msg->setBody(message.getMsgBody().c_str(), message.getMsgBody().length());

	//阿里的message没有把queueid带过来
    //msg->setQueueId(iter->getQueueId);
    //mjx test modify
    msg->setQueueId(-1);
	
    msg->setQueueOffset(message.getQueueOffset());
    msg->setReconsumeTimes(message.getReconsumeTimes());
    msg->setStoreTimestamp(message.getStoreTimestamp());
    msg->setBornTimestamp(message.getStartDeliverTime());
    
    msgList.push_back(msg);
    /* consume message */
    ConsumeConcurrentlyStatus zmqStatus = m_pZmqListener->consumeMessage(msgList, zmqContext);
    /* destroy MessageExt */
    delete msg;

    if (zmqStatus == CONSUME_SUCCESS)
    {
        return CommitMessage;
    }
    else
    {
        return ReconsumeLater;
    }
}

void OnsMessageListener::setZmqListener(MessageListenerConcurrently* listener)
{
    m_pZmqListener = listener;
}

MessageListenerConcurrently* OnsMessageListener::getZmqListener()
{
    return m_pZmqListener;
}

DefaultMQPushConsumerImpl::DefaultMQPushConsumerImpl()
{
    m_pPushConsumer = NULL;
    m_pOrderConsumer = NULL;
    m_pOnsListener = new OnsMessageListener();
    m_pOrderListener = new OnsOrderListener();
}

DefaultMQPushConsumerImpl::~DefaultMQPushConsumerImpl()
{
	//mjx test modify
	/*
    if (m_pPushConsumer != NULL)
    {
        delete m_pPushConsumer;
        m_pPushConsumer = NULL;
    }
    if (m_pOrderConsumer != NULL)
    {
        delete m_pOrderConsumer;
        m_pOrderConsumer = NULL;
    }
    */
    
    if (m_pOnsListener != NULL)
    {
        delete m_pOnsListener;
        m_pOnsListener = NULL;
    }
    if (m_pOrderListener != NULL)
    {
        delete m_pOrderListener;
        m_pOrderListener = NULL;
    }
}

void DefaultMQPushConsumerImpl::setProperty(ons::ONSFactoryProperty& property)
{
    m_factoryInfo.setFactoryProperties(property.getFactoryProperties());
}

ons::ONSFactoryProperty* DefaultMQPushConsumerImpl::getProperty()
{
    return &m_factoryInfo;
}

bool DefaultMQPushConsumerImpl::isConsumeOrderly()
{
    return m_consumeOrderly;
}

ons::PushConsumer* DefaultMQPushConsumerImpl::getPushConsumer()
{
    if (NULL == m_pPushConsumer) 
    {
        m_pPushConsumer = ons::ONSFactory::getInstance()->createPushConsumer(m_factoryInfo);
    }
    return m_pPushConsumer;
}

ons::OrderConsumer* DefaultMQPushConsumerImpl::getOrderConsumer()
{
    if (NULL == m_pOrderConsumer) 
    {
        m_pOrderConsumer = ons::ONSFactory::getInstance()->createOrderConsumer(m_factoryInfo);
    }
    return m_pOrderConsumer;
}

void DefaultMQPushConsumerImpl::setListener(MessageListener* listener)
{
    if (dynamic_cast<MessageListenerOrderly*>(listener) != NULL)
	{
		m_consumeOrderly = true;
        m_pOrderListener->setZmqListener((MessageListenerOrderly*)listener);
	}
	else if (dynamic_cast<MessageListenerConcurrently*>(listener) != NULL)
	{
		m_consumeOrderly = false;
        m_pOnsListener->setZmqListener((MessageListenerConcurrently*)listener);
	}
}

ons::MessageListener* DefaultMQPushConsumerImpl::getListener()
{
    return m_pOnsListener;
}

ons::MessageOrderListener* DefaultMQPushConsumerImpl::getOrderListener()
{
    return m_pOrderListener;
}

