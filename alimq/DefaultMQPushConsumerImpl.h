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

#if!defined __DEFAULTMQPUSHCONSUMERIMPL_H__
#define __DEFAULTMQPUSHCONSUMERIMPL_H__

#include <string>
#include <list>
#include <string.h>
#include "MessageListener.h"
#include "include/MessageListener.h"
#include "include/PullResultONS.h"
#include "include/ONSFactory.h"

class OnsOrderListener : public ons::MessageOrderListener
{
public:
	OnsOrderListener();

	virtual ~OnsOrderListener();

	OrderAction consume(ons::Message& message, ons::ConsumeOrderContext& context);

    void setZmqListener(MessageListenerOrderly* listener);

    MessageListenerOrderly* getZmqListener();

private:
    MessageListenerOrderly* m_pZmqListener;
};


class OnsMessageListener : public ons::MessageListener
{
public:
	OnsMessageListener();

	virtual ~OnsMessageListener();

	Action consume(ons::Message& message, ons::ConsumeContext& context);

    void setZmqListener(MessageListenerConcurrently* listener);

    MessageListenerConcurrently* getZmqListener();
    
private:
    MessageListenerConcurrently* m_pZmqListener;
};

/**
* Push方式的Consumer实现
*
*/
class DefaultMQPushConsumerImpl
{
public:
	DefaultMQPushConsumerImpl();
    virtual ~DefaultMQPushConsumerImpl();

    void setProperty(ons::ONSFactoryProperty& property);

    ons::ONSFactoryProperty* getProperty();

    bool isConsumeOrderly();
    
    ons::PushConsumer* getPushConsumer();

    ons::OrderConsumer* getOrderConsumer();

    void setListener(MessageListener* listener);

    ons::MessageListener* getListener();

    ons::MessageOrderListener* getOrderListener();

private:
    bool m_consumeOrderly;// 是否顺序消费消息
    
    OnsMessageListener* m_pOnsListener;
    OnsOrderListener* m_pOrderListener;
    ons::PushConsumer* m_pPushConsumer;
    ons::OrderConsumer* m_pOrderConsumer;
    ons::ONSFactoryProperty m_factoryInfo;
};



#endif
