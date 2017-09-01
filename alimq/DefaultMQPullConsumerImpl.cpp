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

#include "DefaultMQPullConsumerImpl.h"

#include <string>
#include <set>
#include <map>
#include <string.h>
#include "PullResult.h"


DefaultMQPullConsumerImpl::DefaultMQPullConsumerImpl()
{
	m_pPullConsumer = NULL;
}

DefaultMQPullConsumerImpl::~DefaultMQPullConsumerImpl()
{
	//mjx test modify
	/*
    if (m_pPullConsumer != NULL)
    {
        delete m_pPullConsumer;
        m_pPullConsumer = NULL;
    }
    */
}

//mjx test modify
PullResult* DefaultMQPullConsumerImpl::convertPullResult(ons::PullResultONS& retONS, int queueid)
{
    PullResult* result = NULL;
    /* covert pull status */
    PullStatus pullStatus;
    switch(retONS.pullStatus)
    {
    case ons::ONS_FOUND:
        pullStatus = FOUND;
        break;
    case ons::ONS_NO_NEW_MSG:
        pullStatus = NO_NEW_MSG;
        break;
    case ons::ONS_NO_MATCHED_MSG:
        pullStatus = NO_MATCHED_MSG;
        break;
    case ons::ONS_OFFSET_ILLEGAL:
        pullStatus = OFFSET_ILLEGAL;
        break;
    case ons::ONS_BROKER_TIMEOUT:
    default:
        pullStatus = BROKER_TIMEOUT;
        break;
    }
    /* convert pull msg list */
    std::list<MessageExt*> msgList;
    
    std::vector<ons::Message>::iterator iter = retONS.msgFoundList.begin();
    for (; iter != retONS.msgFoundList.end(); ++iter)
    {
        MessageExt* msg = new MessageExt();
        /* set properties first */
        std::map<std::string, std::string> tmpMap = iter->getUserProperties();
        msg->setProperties(tmpMap);
        
        msg->setMsgId(iter->getMsgID());
        msg->setTopic(iter->getTopic());
        msg->setTags(iter->getTag());
        msg->setKeys(iter->getKey());
		//mjx test modify
        //const char* body = iter->getBody();
		//对于二进制数据，有问题
        //msg->setBody(body, strlen(body));
        msg->setBody(iter->getMsgBody().c_str(), iter->getMsgBody().length());

		//mjx test modify
        msg->setQueueId(queueid);
		
        msg->setQueueOffset(iter->getQueueOffset());
        msg->setReconsumeTimes(iter->getReconsumeTimes());
        msg->setStoreTimestamp(iter->getStoreTimestamp());
        msg->setBornTimestamp(iter->getStartDeliverTime());
        
        msgList.push_back(msg);
    }
    /* create result */
    result = new PullResult(pullStatus, retONS.nextBeginOffset, 
        retONS.minOffset, retONS.maxOffset, msgList);
    return result;
}

void DefaultMQPullConsumerImpl::setProperty(ons::ONSFactoryProperty& property)
{
    m_factoryInfo.setFactoryProperties(property.getFactoryProperties());
}

ons::ONSFactoryProperty* DefaultMQPullConsumerImpl::getProperty()
{
    return &m_factoryInfo;
}

void DefaultMQPullConsumerImpl::setPullConsumer(ons::PullConsumer* pPullConsumer)
{
    m_pPullConsumer = pPullConsumer;
}

ons::PullConsumer* DefaultMQPullConsumerImpl::getPullConsumer()
{
    if (NULL == m_pPullConsumer) 
    {
        m_pPullConsumer = ons::ONSFactory::getInstance()->createPullConsumer(m_factoryInfo);
    }
    return m_pPullConsumer;
}

