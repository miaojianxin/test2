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

#include "ConsumerInvokeCallback.h"
#include "ResponseFuture.h"
#include "PullCallback.h"
#include "MQClientAPIImpl.h"
#include "MQClientException.h"
#include "RemotingCommand.h"

ConsumerInvokeCallback::ConsumerInvokeCallback(PullCallback* pPullCallback, 
    MQClientAPIImpl* pClientAPIImpl)
	:m_pPullCallback(pPullCallback),
	m_pClientAPIImpl(pClientAPIImpl)
{
}

ConsumerInvokeCallback::~ConsumerInvokeCallback()
{
    /* modified by yu.guangjie at 2015-08-16, reason: delete callback*/
    if(m_pPullCallback != NULL)
    {
        delete m_pPullCallback;
        m_pPullCallback = NULL;
    }
}

void ConsumerInvokeCallback::operationComplete(ResponseFuture* pResponseFuture)
{
    /* modified by yu.guangjie at 2015-08-16, reason: consumer callback*/
    RemotingCommand *response = pResponseFuture->getResponseCommand();
    if (response != NULL) 
    {
        try {
            PullResult *pullResult = m_pClientAPIImpl->processPullResponse(response);
            m_pPullCallback->onSuccess(*pullResult);

            /* modified by yu.guangjie at 2015-08-28, reason: delete response */
            response->SetBody(NULL, 0, false);
            delete response;
            
            delete pullResult;
        }
        catch (MQException e) {
            m_pPullCallback->onException(e);
        }
    }
    else 
    {
        if (!pResponseFuture->isSendRequestOK()) 
        {
            MQException e = MQEXCEPTION(MQException, "send request failed", -1);
            m_pPullCallback->onException(e);
        }
        else if (pResponseFuture->isTimeout()) 
        {
            MQException e = MQEXCEPTION(MQException, "wait response timeout", -2);
            m_pPullCallback->onException(e);
        }
        else 
        {
            MQException e = MQEXCEPTION(MQException, "unknow reseaon", -3);
            m_pPullCallback->onException(e);
        }
    }

}
