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

#if!defined __PRODUCERINVOKECALLBACK_H__
#define __PRODUCERINVOKECALLBACK_H__

#include <string>
#include "InvokeCallback.h"

class Message;
class SendCallback;
class MQClientAPIImpl;

/**
* �첽����Ӧ��ص��ӿ�
*
*/
class ProducerInvokeCallback : public InvokeCallback
{
public:
	ProducerInvokeCallback(const std::string& brokerName, 
						   Message& msg, 
						   SendCallback* pSendCallBack, 
						   MQClientAPIImpl* pClientAPIImpl);
	virtual ~ProducerInvokeCallback();
	virtual void operationComplete(ResponseFuture* pResponseFuture);

private:
	SendCallback* m_pSendCallBack;
	MQClientAPIImpl* m_pMQClientAPIImpl;
	Message* m_pMsg;
	std::string m_pBrokerName;
};

#endif
