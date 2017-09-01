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
#include "MQClientException.h"

DefaultMQProducerImpl::DefaultMQProducerImpl()
{
    m_pProducer = NULL;
}

DefaultMQProducerImpl::~DefaultMQProducerImpl()
{
	//mjx test modify
    //if (m_pProducer != NULL)
    //{	
    	
       // delete m_pProducer;
    //}
}

void DefaultMQProducerImpl::convertMessage(Message &srcMsg, ons::Message &dstMsg)
{
    dstMsg.setBody(srcMsg.getBody(), srcMsg.getBodyLen());
    dstMsg.setKey(srcMsg.getKeys().c_str());
    dstMsg.setTopic(srcMsg.getTopic().c_str());
    dstMsg.setTag(srcMsg.getTags().c_str());
    dstMsg.setUserProperties(srcMsg.getProperties());
}

void DefaultMQProducerImpl::setProperty(ons::ONSFactoryProperty& property)
{
    m_factoryInfo.setFactoryProperties(property.getFactoryProperties());
}

ons::ONSFactoryProperty* DefaultMQProducerImpl::getProperty()
{
    return &m_factoryInfo;
}

void DefaultMQProducerImpl::setProducer(ons::Producer* pProducer)
{
    m_pProducer = pProducer;
}

ons::Producer* DefaultMQProducerImpl::getProducer()
{
    if (NULL == m_pProducer) 
    {
        m_pProducer = ons::ONSFactory::getInstance()->createProducer(m_factoryInfo);
    }
    return m_pProducer;
}

void DefaultMQProducerImpl::fetchSubscribeMessageQueues(
    const std::string& topic, std::vector<ons::MessageQueueONS>& mqs)
{     
    ons::PullConsumer* consumer = ons::ONSFactory::getInstance()->createPullConsumer(m_factoryInfo);

    try 
    {
        consumer->start();
        /* get all mq */        
        consumer->fetchSubscribeMessageQueues(topic, mqs);

        consumer->shutdown();
    } catch (ons::ONSClientException& e) {
        delete consumer;
        THROW_MQEXCEPTION(MQClientException, e.GetMsg(), e.GetError());
    }
	//mjx test modify
   // delete consumer;
}

