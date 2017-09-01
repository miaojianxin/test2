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

#if!defined __DEFAULTMQPRODUCERIMPL_H__
#define __DEFAULTMQPRODUCERIMPL_H__

#include "Message.h"
#include "include/Message.h"
#include "include/ONSFactory.h"
#include "include/ONSClientException.h"
#include <iostream>

/**
* 生产者默认实现
*
*/
class DefaultMQProducerImpl
{
public:
    DefaultMQProducerImpl();
    virtual ~DefaultMQProducerImpl();

    void convertMessage(Message &srcMsg, ons::Message &dstMsg);

    void fetchSubscribeMessageQueues(
        const std::string& topic, std::vector<ons::MessageQueueONS>& mqs);

    void setProperty(ons::ONSFactoryProperty& property);
    
    ons::ONSFactoryProperty* getProperty();

    void setProducer(ons::Producer* pProducer);

    ons::Producer* getProducer();

private:
    ons::Producer* m_pProducer;
    ons::ONSFactoryProperty m_factoryInfo;
};

#endif
