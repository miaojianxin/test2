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

#include "TcpRemotingClient.h"
#include "TcpTransport.h"
#include "ThreadPool.h"
#include "ScopedLock.h"
#include "KPRUtil.h"
#include "ResponseFuture.h"
#include "SocketUtil.h"
#include "TcpRequestProcessor.h"
#include "MQProtos.h"
#include "UtilAll.h"
#include <stdio.h>
#include <string.h>

#define PKT_DUMP(_buf, _len, _prefix) \
    if(MQLOG_DEBUG >= G_MQLOGLEVEL) \
    { \
        pkt_dump((unsigned char *)_buf,_len, _prefix); \
    }

#define P(_c) ((_c >= ' ' && _c < 0x7f) ? _c : '.')
void pkt_dump(const unsigned char *buf, int len, const char *prefix)
{
	// ....: .. .. .. .. .. .. .. ..  .. .. .. .. .. .. .. ..  ________ ________
	int i;
	char t[128];
	char hex[] = "0123456789abcdef";
    std::string strDump = "[";

    strDump.append(prefix);
	sprintf(t, "]---------------------- %d bytes at %p\n", len, buf);
    strDump.append(t);
	if (len > 40960)
		len = 40960;
	for (i = 0; i < len; i++) {
		unsigned char c = buf[i];
		unsigned int o = i % 16;
		if (o == 0) {
			if (i > 0)
            {
                strDump.append(t);
                strDump.append("\n");
            }
			memset(t, ' ', 79);
			t[80] = '\0';
			t[0] = hex[(i>>12) & 0xf];
			t[1] = hex[(i>>8) & 0xf];
			t[2] = hex[(i>>4) & 0xf];
			t[3] = hex[(i>>0) & 0xf];
			t[4] = ':';
		}
		t[6 + 3*o + (o >> 3)] = hex[c >> 4];
		t[7 + 3*o + (o >> 3)] = hex[c & 0xf];
		t[56 + o + (o >> 3)] = P(c);
	}
    strDump.append(t);
    strDump.append("\n");
    
    UtilAll::mqLogRaw(MQLOG_DEBUG, strDump.c_str());
}


ProcessDataWork::ProcessDataWork(TcpRemotingClient* pClient,std::string* pData)
	:m_pClient(pClient),m_pData(pData)
{

}

ProcessDataWork::~ProcessDataWork()
{

}

void ProcessDataWork::Do()
{
	try
	{
		m_pClient->ProcessData(m_pData);
	}
	catch (...)
	{
	}
    /* modified by yu.guangjie at 2015-08-28, reason: delete this */
    delete this;
}

TcpRemotingClient::TcpRemotingClient(const RemoteClientConfig& config)
	:m_config(config),m_stop (false)
{
	m_pThreadPool = new kpr::ThreadPool(10,5,20);
	m_EventThread = new EventThread(*this);
	m_maxFd=0;
	FD_ZERO (&m_rset);
	SocketInit();
}

TcpRemotingClient::~TcpRemotingClient()
{
	SocketUninit();
    /* modified by yu.guangjie at 2015-08-28, reason: */
	std::map<std::string ,TcpTransport*>::iterator it = m_tcpTransportTable.begin();
	for (; it!=m_tcpTransportTable.end(); it++)
	{
		TcpTransport* tts = it->second;
		delete tts;
	}
    m_tcpTransportTable.clear();
}

void TcpRemotingClient::start()
{
	m_EventThread->Start();
}

void TcpRemotingClient::shutdown()
{
	m_stop=true;
	m_pThreadPool->Destroy();
	m_EventThread->Join();
}

void TcpRemotingClient::updateNameServerAddressList(const std::list<std::string>& addrs)
{
	m_namesrvAddrList = addrs;
}

std::list<std::string> TcpRemotingClient::getNameServerAddressList()
{
	return m_namesrvAddrList;
}

RemotingCommand* TcpRemotingClient::invokeSync(const std::string& addr,
		RemotingCommand& request,
		int timeoutMillis)
{
	TcpTransport* tts = GetAndCreateTransport(addr);
	if (tts != NULL && tts->IsConnected())
	{
		return invokeSyncImpl(tts, request, timeoutMillis);
	}
	else
	{
		MqLogWarn("Can't connect to server: %s!", addr.c_str());        
		return NULL;
	}
}

int TcpRemotingClient::invokeAsync(const std::string& addr,
								   RemotingCommand& request,
								   int timeoutMillis,
								   InvokeCallback* pInvokeCallback)
{
	TcpTransport* tts = GetAndCreateTransport(addr);
	if (tts != NULL && tts->IsConnected())
	{
		return invokeAsyncImpl(tts, request, timeoutMillis, pInvokeCallback);
	}
	else
	{
		MqLogWarn("Can't connect to server: %s!", addr.c_str());  
		return -1;
	}
}

int TcpRemotingClient::invokeOneway(const std::string& addr,
									RemotingCommand& request,
									int timeoutMillis)
{
	TcpTransport* tts = GetAndCreateTransport(addr);
	if (tts != NULL && tts->IsConnected())
	{
		return invokeOnewayImpl(tts, request, timeoutMillis);
	}
	else
	{
		MqLogWarn("Can't connect to server: %s!", addr.c_str());  
		return -1;
	}
}

void TcpRemotingClient::HandleSocketEvent(fd_set rset)
{
	bool bUpdate = false;
	std::list<std::string*> data;
	{
		kpr::ScopedLock<kpr::Mutex> lock(m_mutex);
		std::map<std::string ,TcpTransport*>::iterator it = m_tcpTransportTable.begin();

		for (; it!=m_tcpTransportTable.end(); it++)
		{
			TcpTransport* tts = it->second;
			if (FD_ISSET (tts->GetSocket(), &rset))
			{
				if (tts->RecvData(data) <= 0) bUpdate = true;
			}
		}
	}

	std::list<std::string*>::iterator it = data.begin();
	for (; it!=data.end(); it++)
	{
		ProcessDataWork* work = new ProcessDataWork(this,(*it));
		m_pThreadPool->AddWork(work);
	}
	if (bUpdate) UpdateEvent();
}

void TcpRemotingClient::UpdateEvent()
{
	kpr::ScopedLock<kpr::Mutex> lock(m_mutex);
	std::map<std::string ,TcpTransport*>::iterator it = m_tcpTransportTable.begin();
	m_maxFd=0;
	FD_ZERO (&m_rset);

	for (; it!=m_tcpTransportTable.end(); it++)
	{
		TcpTransport* tts = it->second;
		if (!tts->IsConnected()) continue;
		FD_SET (tts->GetSocket(), &m_rset);
		if (tts->GetSocket() > m_maxFd)
		{
			m_maxFd = tts->GetSocket();
		}
	}
}

void TcpRemotingClient::Run()
{
	fd_set rset, xset;
	unsigned long long beginTime = GetCurrentTimeMillis();

	do
	{
		try
		{
			FD_ZERO (&rset);
			FD_ZERO (&xset);
			{
				kpr::ScopedLock<kpr::Mutex> lock(m_mutex);

				rset = m_rset;
				xset = m_rset;
			}

			struct timeval tv = {1, 0};
			int r = select(m_maxFd+1, &rset, NULL, &xset, &tv);
			int err = NET_ERROR;

			if (r == -1 && err == WSAEBADF)
			{
				// worker thread already closed some fd
				// let's loop and build fd set again
				continue;
			}

			if (r > 0)
			{
				HandleSocketEvent (rset);
			}

			HandleTimerEvent(GetCurrentTimeMillis()-beginTime );
		}
		catch (...)
		{
			
		}
	}
	while (!m_stop);
}

TcpTransport* TcpRemotingClient::GetAndCreateTransport( const std::string& addr )
{
	TcpTransport* tts;

	{
		kpr::ScopedLock<kpr::Mutex> lock(m_mutex);
		std::map<std::string ,TcpTransport*>::iterator it = m_tcpTransportTable.find(addr);
		if (it!=m_tcpTransportTable.end())
		{
		    tts = it->second;
            if(tts->IsConnected())
            {
                return tts;
            }
            else
            {
                m_tcpTransportTable.erase(it);
                delete tts;
            }
		}

		std::map<std::string ,std::string> config;
		tts = new TcpTransport(config);
        int iRet = tts->Connect(addr);
		if (iRet != CLIENT_ERROR_SUCCESS)
		{
		    MqLogWarn("Connect to %s failed: retcode=%d", addr.c_str(), iRet);
            delete tts;
			return NULL;
		}

		m_tcpTransportTable[addr]=tts;
	}

	UpdateEvent();

	return tts;
}

void TcpRemotingClient::HandleTimerEvent(unsigned long long tm)
{
    /* modified by yu.guangjie at 2015-11-04, reason: */
	static unsigned long long s_LastTimer = 0;

    if(tm < s_LastTimer + 10)
    {
        return;
    }
    s_LastTimer = tm;

    kpr::ScopedLock<kpr::Mutex> lock(m_mutexResp);

    std::map<int,ResponseFuture*>::iterator it = m_responseTable.begin();
	while(it != m_responseTable.end())
	{
        ResponseFuture* pRes = it->second;
        if(pRes->isTimeout())
        {
			MqLogWarn("request is timeout, delete it's ResponseFuture object. opaque=%d", pRes->getOpaque());

            m_responseTable.erase(it++);
            /* modified by yu.guangjie at 2017-03-14, reason: delete on async */
            pRes->putResponse(NULL);
            if(pRes->getInvokeCallback() != NULL)
            {
                pRes->executeInvokeCallback();
                delete pRes;
            }
        }
        else
        {
            it++;
        }
	}
}

void TcpRemotingClient::ProcessData( std::string* pData )
{
	const char* data = pData->data();
	int len = pData->size();

    PKT_DUMP(data,len, "Decode data");
 
	RemotingCommand* cmd = RemotingCommand::CreateRemotingCommand(data,len);

    /* modified by yu.guangjie at 2015-08-13, reason: */
    ResponseFuture* resp = NULL;
	int code = 0;
	if (cmd->isResponseType())
	{
        {   
    	    kpr::ScopedLock<kpr::Mutex> lock(m_mutexResp);
    		std::map<int,ResponseFuture*>::iterator it = m_responseTable.find(cmd->getOpaque());
    		if (it!=m_responseTable.end())
    		{
    		    resp = it->second;
    			code = resp->getRequestCode();
                m_responseTable.erase(it);
    		}
			else
			{
				//note by lin.qiongshan, 2016-11-21
				//	Ӧ���� sendOneWay ��ʽ���͵��������Ӧ��sendOneWay ����������� ResponseFuture ���������յ������Ӧ��ʱ����Ȼ�Ҳ���
				MqLogWarn("response has no related ResponseFuture object. opaque=%d", cmd->getOpaque());
			}
        }        
        MqLogVerb("Recv response[%s]: code=%d, Opaque=%d", 
            getMQRequestNameByCode(code), code, cmd->getOpaque());
	}
	else
	{
		code = cmd->getCode();
        MqLogVerb("Recv request[%s]: code=%d", getMQRequestNameByCode(code), code);
	}
    
    if(code > 0)
    {
        cmd->MakeCustomHeader(code,data,len);
    }
	processMessageReceived(cmd, resp);
    
	delete pData;
}

RemotingCommand* TcpRemotingClient::invokeSyncImpl( TcpTransport* pTts,
		RemotingCommand& request,
		int timeoutMillis )
{
	ResponseFuture* responseFuture = new ResponseFuture(request.getCode(),request.getOpaque(), timeoutMillis, NULL, true);
    {
        kpr::ScopedLock<kpr::Mutex> lock(m_mutexResp);
        m_responseTable.insert(std::pair<int,ResponseFuture*>(request.getOpaque(), responseFuture));

		//add by lin.qiongshan, 2016��8��30��15:22:36, �����־
		MqLogDebug("save request's ResponseFuture object in invokeSyncImpl. request code=%d, opaque=%d", request.getCode(), request.getOpaque());
    }    
	int ret = SendCmd(pTts,request,timeoutMillis);
	if (ret==0)
	{
		responseFuture->setSendRequestOK(true);
	}
	else
	{
		// close socket?
		responseFuture->setSendRequestOK(false);
        {
            kpr::ScopedLock<kpr::Mutex> lock(m_mutexResp);
            std::map<int,ResponseFuture*>::iterator it = m_responseTable.find(request.getOpaque());
    		if (it!=m_responseTable.end())
    		{
                m_responseTable.erase(it);
    		}
        }
		delete responseFuture;

        /* modified by yu.guangjie at 2015-11-04, reason: add log*/
        MqLogWarn("Can't send request[%s] to server: %s!", 
            getMQRequestNameByCode(request.getCode()), pTts->GetServerURL().c_str());
		return NULL;
	}



	RemotingCommand* responseCommand = responseFuture->waitResponse(timeoutMillis);
	/** Mdy by lin.qiongshan, 2016-11-21����ʱɾ�� responseFuture ����
		1. �ڱ��������� SendCmd ʱ����������ʧ�ܻ�ɾ�� ResponseFuture ���󣬲����غ����������ߵ���ע�͵�λ��
		2. �ڱ��������� SendCmd ʱ����������ɹ��󣬻���� ResponseFuture ����� waitResponse ����ȴ���Ӧ
			2.1. ��� waitResponse ���ز�Ϊ�գ��� ResponseFuture �����л�ȡ�䱣��� ResponseCommand ������Ϊ�������ؽ��
				���� TcpRemotingClient::ProcessData �����У���� TcpRemotingClient �յ���Ӧ������� m_responseTable �Ƴ�����Ӧ����Ӧ������� ResponseFuture ����
					�������ͬ�����󣬲���ɾ���� ResponseFuture ���󣬴�ʱ��Ҫ���ⲿ��Ҳ���Ǳ�ע�͵�λ�ã�ɾ���� ResponseFuture ����
			2.2. ��� waitResponse ����Ϊ�գ�˵����ʱ�����Ὣ ResponseFuture ����� m_responseTable map ���Ƴ���
				2.2.1. ��� responseFuture->isSendRequestOK Ϊ true��˵�� SendCmd ʱ�ɹ�����Ӧ��ʱ����Ӧ��ʱ�� responseFuture �ɵ������߳�ͨ�� TcpRemotingClient::HandleTimerEvent ���ж�ʱ�����˴���Ӧ��ɾ��
					��ԭ���Ĵ����ڴ˴��Ὣ ResponseFuture ����� m_responseTable ���Ƴ���Ȼ��ɾ���� ResponseFuture ���󣻶�����һ���������߳�ͨ������ socket �ϵ���Ϣ���ܣ���ÿ���յ����ݺ����� TcpRemotingClient::HandleTimerEvent ɾ����ʱ�� ResponseFuture ���󣨰����� m_responseTable ���Ƴ���Ӧ���ɾ���� ResponseFuture ����
						�п��ܳ��ֱ�����ʱ���� TcpRemotingClient::HandleTimerEvent �ȷ��ֲ�ɾ���� ResponseFuture ���󣬶��˴���ɾ���ö��󣬵��� coredump
						��ˣ���������£�Ӧֻ��һ������ɾ��������ͳһ���� TcpRemotingClient::HandleTimerEvent
				2.2.2. ��� responseFuture->isSendRequestOK Ϊ false����������²����ߵ������Ϊ������� SendCmd ʧ�ܣ�����ǰ����߼��� SendCmd ʧ�ܺ�����ֱ�ӷ���

		���Ͼ��ǣ����˴��������ж� isSendRequestOK���� SendCmd �Ľ����
		waitResponse ���طǿ�ʱ��ֱ��ɾ�� ResponseFuture �����ٴ�֮ǰ��TcpRemotingClient::ProcessData �ѽ��ö���� m_responseTable �Ƴ���
		waitResponse ��ʱ��˵����ʱ��ͳһ�� TcpRemotingClient::HandleTimerEvent �н��д����˴��������κζ�������¼��־����
	*/
#if 1

	if (NULL != responseCommand)
	{
		delete responseFuture;
	}
	else
	{
		MqLogWarn("Can't receive response[%s] from server: %s!",
			getMQRequestNameByCode(request.getCode()), pTts->GetServerURL().c_str());
	}

#else
	if (responseCommand ==NULL)
	{
		// ��������ɹ�����ȡӦ��ʱ
		if (responseFuture->isSendRequestOK())
		{
			
		}
		else// ��������ʧ��
		{
		    
		}
        /* modified by yu.guangjie at 2015-11-04, reason: */
        {
            kpr::ScopedLock<kpr::Mutex> lock(m_mutexResp);
            std::map<int,ResponseFuture*>::iterator it = m_responseTable.find(request.getOpaque());
    		if (it!=m_responseTable.end())
    		{
                m_responseTable.erase(it);
    		}
        }
        MqLogWarn("Can't receive response[%s] from server: %s!", 
            getMQRequestNameByCode(request.getCode()), pTts->GetServerURL().c_str());
	}
	
    delete responseFuture;
#endif

	return responseCommand;
}

int TcpRemotingClient::invokeAsyncImpl( TcpTransport* pTts,
										RemotingCommand& request,
										int timeoutMillis,
										InvokeCallback* pInvokeCallback )
{
    /* modified by yu.guangjie at 2015-08-13, reason: change block to false*/
	ResponseFuture* responseFuture = new ResponseFuture(request.getCode(),request.getOpaque(), timeoutMillis, pInvokeCallback, false);
	{
        kpr::ScopedLock<kpr::Mutex> lock(m_mutexResp);
        m_responseTable.insert(std::pair<int,ResponseFuture*>(request.getOpaque(), responseFuture));

		MqLogDebug("save request's ResponseFuture object in invokeAsyncImpl. request code=%d, opaque=%d", request.getCode(), request.getOpaque());
	}
    int ret = SendCmd(pTts,request,timeoutMillis);
	if (ret==0)
	{
		responseFuture->setSendRequestOK(true);
	}
	else
	{
		responseFuture->setSendRequestOK(false);
		{
            kpr::ScopedLock<kpr::Mutex> lock(m_mutexResp);
            m_responseTable.erase(m_responseTable.find(request.getOpaque()));
		}

		//lin.qiongshan, 2016��8��18��16:32:02
		MqLogWarn("send request failed, delete it's ResponseFuture object. ret=%d, opaque=%d", ret, request.getOpaque());

        /* modified by yu.guangjie at 2015-11-04, reason: */
        responseFuture->executeInvokeCallback();
        delete responseFuture;
	}

	return ret;
}

int TcpRemotingClient::invokeOnewayImpl( TcpTransport* pTts,
		RemotingCommand& request,
		int timeoutMillis )
{
	request.markOnewayRPC();
	
	//add by lin.qiongshan, 2016��8��30��15:32:07, oneway ��Ϊ�����淢����Ϣ���յ���Ӧ����Ӧ���� m_responseTable �в����ж�Ӧ��������Ϣ
	//	Ϊ�˷���ȷ����Щ������ oneway �ģ�ͨ����־��ӡ
	MqLogDebug("do not need ResponseFuture object in invokeOnewayImpl. request code=%d, opaque=%d", request.getCode(), request.getOpaque());

	return SendCmd(pTts,request,timeoutMillis);
}

void TcpRemotingClient::processMessageReceived(RemotingCommand* pCmd, ResponseFuture* pResp)
{
	switch (pCmd->getType())
	{
	case REQUEST_COMMAND:
		processRequestCommand(pCmd);
        /* modified by yu.guangjie at 2015-08-28, reason: delete cmd */
        delete pCmd;
		break;
	case RESPONSE_COMMAND:
		processResponseCommand(pCmd, pResp);
		break;
	default:
        /* modified by yu.guangjie at 2015-08-28, reason: delete cmd */
        delete pCmd;
		break;
	}
}

void TcpRemotingClient::processRequestCommand(RemotingCommand* pCmd)
{
    /* modified by yu.guangjie at 2015-08-12, reason: add processRequestCommand*/
    std::map<int,TcpRequestProcessor*>::iterator it = m_processorTable.find(pCmd->getCode());
	if (it!=m_processorTable.end())
	{
		TcpRequestProcessor * trp = it->second;
		RemotingCommand *rc = trp->processRequest(pCmd);
		
	}
	else
	{
		// û�ҵ���������
	}
}

void TcpRemotingClient::processResponseCommand(RemotingCommand* pCmd, ResponseFuture* pResp)
{
    /* modified by yu.guangjie at 2015-08-13, reason: */
    if(NULL == pResp)
    {// û�ҵ�req��������
        /* modified by yu.guangjie at 2015-08-28, reason: delete cmd */
        delete pCmd;
        return;
    }

    /* modified by yu.guangjie at 2015-09-30, reason: */
    if(pResp->getInvokeCallback() != NULL)
    {//ASYNC
        pResp->putResponse(pCmd);
        pResp->executeInvokeCallback();
        delete pResp;
    }
    else
    {
        pResp->putResponse(pCmd);
    }
}

int TcpRemotingClient::SendCmd( TcpTransport* pTts,RemotingCommand& msg,int timeoutMillis )
{
 	/* modified by yu.guangjie at 2016-04-18, reason: */
	int code = msg.getCode();
    MqLogVerb("Send request[%s] to server[%s]: code=%d", 
        getMQRequestNameByCode(code), pTts->GetServerURL().c_str(), code);
    if(msg.GetHeadLen() <= 0)
    {
        MqLogWarn("Request is NULL, command code: %d", code);
		return -1;
    }
	char *pData = NULL;
	const char* pconstData = msg.GetHead();
	int iHeadLen = msg.GetHeadLen();
	int iBodyLen = msg.GetBodyLen();
	int iTotalLen = iHeadLen + iBodyLen;
	if (iTotalLen > iHeadLen && msg.GetBody())
	{
	    pData = new char[iTotalLen];
		memcpy(pData,msg.GetHead(),iHeadLen);
		memcpy(pData+iHeadLen, msg.GetBody() ,iBodyLen);
		pconstData = pData;
	}
	PKT_DUMP(pconstData, iTotalLen, "Send msg data"); 
	int ret = pTts->SendData(pconstData, iTotalLen, timeoutMillis);
	if (pData != NULL)
	{
	    delete pData;
		pData = NULL;
	}

	return ret;
}

void TcpRemotingClient::registerProcessor( int requestCode, TcpRequestProcessor* pProcessor )
{
	m_processorTable[requestCode]=pProcessor;
}
