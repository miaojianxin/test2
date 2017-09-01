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
#if!defined __DEFAULTMQPULLCONSUMERIMPL_H__
#define __DEFAULTMQPULLCONSUMERIMPL_H__

#include <string>
#include <set>
#include <map>
#include <vector>
#include "include/PullResultONS.h"
#include "include/ONSFactory.h"

class PullResult;

/**
* Pull方式的Consumer实现
*/
class DefaultMQPullConsumerImpl
{
public:
	DefaultMQPullConsumerImpl();
	virtual ~DefaultMQPullConsumerImpl();

	//mjx test modify
    PullResult* convertPullResult(ons::PullResultONS& retONS,int queueid);

    void setProperty(ons::ONSFactoryProperty& property);

    ons::ONSFactoryProperty* getProperty();

    void setPullConsumer(ons::PullConsumer* pPullConsumer);

    ons::PullConsumer* getPullConsumer();

private:
	ons::PullConsumer* m_pPullConsumer;
	ons::ONSFactoryProperty m_factoryInfo;
    
};


#endif
