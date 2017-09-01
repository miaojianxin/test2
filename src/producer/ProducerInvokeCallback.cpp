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

#include "ProducerInvokeCallback.h"
#include "ResponseFuture.h"
#include "RemotingCommand.h"
#include "SendCallback.h"
#include "MQClientAPIImpl.h"
#include "MQClientException.h"
#include "Message.h"

ProducerInvokeCallback::ProducerInvokeCallback(const std::string& brokerName,
		Message& msg, 
		SendCallback* pSendCallBack, 
		MQClientAPIImpl* pClientAPIImpl)
	:m_pSendCallBack(pSendCallBack),
	 m_pMQClientAPIImpl(pClientAPIImpl),
	 m_pMsg(&msg),
	 m_pBrokerName(brokerName)
{
}

ProducerInvokeCallback::~ProducerInvokeCallback()
{
    /* modified by liang.haibo at 2016-09-18, reason: delete callback*/
    if(m_pSendCallBack != NULL)
    {
        delete m_pSendCallBack;
        m_pSendCallBack = NULL;
    }
}

void ProducerInvokeCallback::operationComplete(ResponseFuture* pResponseFuture)
{
    /* modified by liang.haibo at 2016-09-18, reason: producer callback*/
    RemotingCommand *response = pResponseFuture->getResponseCommand();
	if ( NULL != response )
	{	
        try 
		{
            SendResult *sendResult = m_pMQClientAPIImpl->processSendResponse(m_pBrokerName, *m_pMsg, response);
            m_pSendCallBack->onSuccess(*sendResult);

            response->SetBody(NULL, 0, false);
            delete response;
            
            delete sendResult;
        }
        catch (MQException e) {
            m_pSendCallBack->onException(e);
        }
		
	}
	else 
	{
        if (!pResponseFuture->isSendRequestOK()) 
        {
            MQException e = MQEXCEPTION(MQException, "send request failed", -1);
            m_pSendCallBack->onException(e);
        }
        else if (pResponseFuture->isTimeout()) 
        {
            MQException e = MQEXCEPTION(MQException, "wait response timeout", -2);
            m_pSendCallBack->onException(e);
        }
        else 
        {
            MQException e = MQEXCEPTION(MQException, "unknow reseaon", -3);
            m_pSendCallBack->onException(e);
        }
	}
}
