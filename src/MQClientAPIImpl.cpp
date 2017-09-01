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

#include "MQClientAPIImpl.h"

#include <assert.h>

#include "TcpRemotingClient.h"
#include "MQProtos.h"
#include "UtilAll.h"
#include "PullResultExt.h"
#include "ConsumerInvokeCallback.h"
#include "NamesrvUtil.h"
#include "VirtualEnvUtil.h"
#include "ClientRemotingProcessor.h"
#include "CommandCustomHeader.h"
#include "TopicList.h"
#include "ProducerInvokeCallback.h"
#include "MQClientException.h"
#include "SocketUtil.h"
#include "GetConsumerListByGroupResponseBody.h"

MQClientAPIImpl::MQClientAPIImpl( const RemoteClientConfig& remoteClientConfig,
	ClientRemotingProcessor* pClientRemotingProcessor)
	:m_pClientRemotingProcessor(pClientRemotingProcessor)
{
	m_pRemotingClient = new TcpRemotingClient(remoteClientConfig);

	/**
	* ע��ͻ���֧�ֵ�RPC CODE
	*/
	m_pRemotingClient->registerProcessor(CHECK_TRANSACTION_STATE_VALUE, m_pClientRemotingProcessor);
	m_pRemotingClient->registerProcessor(NOTIFY_CONSUMER_IDS_CHANGED_VALUE, m_pClientRemotingProcessor);
}

MQClientAPIImpl::~MQClientAPIImpl()
{
    if(m_pRemotingClient != NULL)
    {
        delete m_pRemotingClient;
        m_pRemotingClient = NULL;
    }
}

std::string MQClientAPIImpl::getProjectGroupPrefix()
{
	return m_projectGroupPrefix;
}

std::list<std::string> MQClientAPIImpl::getNameServerAddressList()
{
	return m_pRemotingClient->getNameServerAddressList();
}

TcpRemotingClient* MQClientAPIImpl::getRemotingClient()
{
	return m_pRemotingClient;
}

std::string MQClientAPIImpl::fetchNameServerAddr()
{
	try
	{
		std::string addrs = m_topAddressing.fetchNSAddr();
		if (!addrs.empty())
		{
			if (addrs!=m_nameSrvAddr)
			{
				updateNameServerAddressList(addrs);
				m_nameSrvAddr = addrs;
				return m_nameSrvAddr;
			}
		}
	}
	catch (...)
	{

	}

	return m_nameSrvAddr;
}

void MQClientAPIImpl::updateNameServerAddressList( const std::string& addrs)
{
	m_nameSrvAddr = addrs;
	std::list<std::string> lst;
	std::vector<std::string> av;
	int size = UtilAll::Split(av,addrs,";");

	for (int i=0; i<size; i++)
	{
		lst.push_back(av[i]);
		m_pRemotingClient->updateNameServerAddressList(lst);
	}
}

void MQClientAPIImpl::start()
{
	// Զ��ͨ�� Client ����
	m_pRemotingClient->start();

	// ��ȡ�������л�����ص�project group
	try
	{
		std::string localAddress = getLocalAddress();
		m_projectGroupPrefix = getProjectGroupByIp(localAddress, 3000);
	}
	catch (std::exception e)
	{
	}
}

void MQClientAPIImpl::shutdown()
{
	m_pRemotingClient->shutdown();
}

void MQClientAPIImpl::createSubscriptionGroup(const std::string& addr,
											  SubscriptionGroupConfig config,
											  int timeoutMillis)
{	
	/* modified by liang.haibo at 2016-10-11, reason: add createSubscriptionGroup*/
	// ����������л�����ص�projectGroupPrefix
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		config.groupName = VirtualEnvUtil::buildWithProjectGroup(config.groupName,m_projectGroupPrefix);
	}

	SubscriptionGroupConfigHeader * requestHeader = new SubscriptionGroupConfigHeader();
	requestHeader->groupName = config.groupName;
	requestHeader->consumeEnable = config.consumeEnable;
	requestHeader->consumeFromMinEnable = config.consumeFromMinEnable;
	requestHeader->consumeBroadcastEnable = config.consumeBroadcastEnable;
	requestHeader->retryQueueNums = config.retryQueueNums;
	requestHeader->retryMaxTimes = config.retryMaxTimes;
	requestHeader->brokerId = config.brokerId;
	requestHeader->whichBrokerWhenConsumeSlowly = config.whichBrokerWhenConsumeSlowly;
	
	RemotingCommand* request =
		RemotingCommand::createRequestCommand(UPDATE_AND_CREATE_SUBSCRIPTIONGROUP_VALUE, requestHeader);
	request->Encode();
	
	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
	delete request;
	if (response)
	{
	    int code = response->getCode();
		switch (code)
		{
			case SUCCESS_VALUE:
			{
                delete response;
				return;
			}
			default:
				break;
		}
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException, remark,code);
	}

}


void MQClientAPIImpl::createTopic(const std::string& addr,
								  const std::string& defaultTopic,
								  TopicConfig topicConfig,
								  int timeoutMillis)
{
	// ����������л�����ص�projectGroupPrefix
	std::string topicWithProjectGroup = topicConfig.getTopicName();
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		topicWithProjectGroup =
			VirtualEnvUtil::buildWithProjectGroup(topicConfig.getTopicName(), m_projectGroupPrefix);
	}

	CreateTopicRequestHeader* requestHeader = new CreateTopicRequestHeader();
	requestHeader->topic = (topicWithProjectGroup);
	requestHeader->defaultTopic=(defaultTopic);
	requestHeader->readQueueNums=(topicConfig.getReadQueueNums());
	requestHeader->writeQueueNums=(topicConfig.getWriteQueueNums());
	requestHeader->perm=(topicConfig.getPerm());
	requestHeader->topicFilterType=(topicConfig.getTopicFilterType());

	RemotingCommand* request =
		RemotingCommand::createRequestCommand(UPDATE_AND_CREATE_TOPIC_VALUE,requestHeader);
	request->Encode();

	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
	delete request;
	if (response)
	{
	    /* modified by yu.guangjie at 2015-08-14, reason: */
	    int code = response->getCode();
		switch (code)
		{
		case SUCCESS_VALUE:
			{
                delete response;
				return;
			}
		default:
			break;
		}
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException, remark,code);
	}

	THROW_MQEXCEPTION(MQClientException, "createTopic failed",-1);
}

SendResult MQClientAPIImpl::sendMessage(const std::string& addr,
										const std::string& brokerName,
										Message& msg,
										SendMessageRequestHeader* pRequestHeader,
										int timeoutMillis,
										CommunicationMode communicationMode,
										SendCallback* pSendCallback)
{
	// ����������л�����ص�projectGroupPrefix
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		msg.setTopic(VirtualEnvUtil::buildWithProjectGroup(msg.getTopic(), m_projectGroupPrefix));
		pRequestHeader->producerGroup=(VirtualEnvUtil::buildWithProjectGroup(pRequestHeader->producerGroup,
			m_projectGroupPrefix));
		pRequestHeader->topic=(VirtualEnvUtil::buildWithProjectGroup(pRequestHeader->topic,
			m_projectGroupPrefix));
	}

	RemotingCommand* request =
		RemotingCommand::createRequestCommand(SEND_MESSAGE_VALUE, pRequestHeader);
	request->SetBody((char*)msg.getBody(),msg.getBodyLen(),false);
	request->Encode();
	SendResult result;

	switch (communicationMode)
	{
	case ONEWAY:
		m_pRemotingClient->invokeOneway(addr, *request, timeoutMillis);
        delete request;
		return result;
	case ASYNC:
		sendMessageAsync(addr, brokerName, msg, timeoutMillis, *request, pSendCallback);
        delete request;
		return result;
	case SYNC:
		{
			SendResult* r = sendMessageSync(addr, brokerName, msg, timeoutMillis, *request);
            delete request;
			if (r)
			{
				result =*r;
    			/* modified by yu.guangjie at 2016-04-19, reason: delete r */
				delete r;
			}

			return result;
		}
	default:
		break;
	}

	return result;
}

PullResult* MQClientAPIImpl::pullMessage(const std::string& addr,
										PullMessageRequestHeader* pRequestHeader,
										int timeoutMillis,
										CommunicationMode communicationMode,
										PullCallback* pPullCallback)
{
	// ����������л�����ص�projectGroupPrefix
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		pRequestHeader->consumerGroup=(VirtualEnvUtil::buildWithProjectGroup(
			pRequestHeader->consumerGroup, m_projectGroupPrefix));
		pRequestHeader->topic=(VirtualEnvUtil::buildWithProjectGroup(pRequestHeader->topic,
			m_projectGroupPrefix));
	}

	RemotingCommand* request = RemotingCommand::createRequestCommand(PULL_MESSAGE_VALUE, pRequestHeader);
	request->Encode();

	switch (communicationMode)
	{
	case ONEWAY:
		return NULL;
	case ASYNC:
		pullMessageAsync(addr, request, timeoutMillis, pPullCallback);
        delete request;
		return NULL;
	case SYNC:
        {
    		PullResult* result = pullMessageSync(addr, request, timeoutMillis);
            delete request;
            return result;
        }
	default:
		break;
	}

	return NULL;
}

MessageExt* MQClientAPIImpl::viewMessage( const std::string& addr,  long long phyoffset,  int timeoutMillis)
{
	/* modified by liang.haibo at 2016-10-11, reason: add viewMessage*/
	ViewMessageRequestHeader* requestHeader = new ViewMessageRequestHeader();
    requestHeader->setOffset(phyoffset);
    RemotingCommand* request =
		RemotingCommand::createRequestCommand(VIEW_MESSAGE_BY_ID_VALUE, requestHeader);
    request->Encode();
    RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
    if(response != NULL)
    {
        int code = response->getCode();
        switch (code) 
        {
			case SUCCESS_VALUE: 
			{	
				int tmp = 0;
				MessageExt* messageExt = MessageDecoder::decode( response->GetBody(), response->GetBodyLen(), tmp);
				
				// ����������л�����ص�projectGroupPrefix
				if (!UtilAll::isBlank(m_projectGroupPrefix)) 
				{
					messageExt->setTopic(VirtualEnvUtil::clearProjectGroup(messageExt->getTopic(),m_projectGroupPrefix));
				}
				delete response;
				return messageExt;
			}
			default:
				break;
        }
        std::string remark = response->getRemark();        
        delete response;

        THROW_MQEXCEPTION(MQClientException, remark,code);
    }
    
	return NULL;
	
}

long long MQClientAPIImpl::searchOffset( const std::string& addr,
										const std::string& topic,
										int queueId,
										long long timestamp,
										int timeoutMillis)
{
	/* modified by liang.haibo at 2016-10-08, reason: support message restore by timestamp*/
	MqLogVerb("searchOffset[%lld] on broker: address(%s), topic(%s), queueid(%d)", 
			  timestamp, addr.c_str(), topic.c_str(), queueId);
	
	// ����������л�����ص�projectGroupPrefix
    std::string topicWithProjectGroup = topic;
    if (!UtilAll::isBlank(m_projectGroupPrefix)) 
    {
        topicWithProjectGroup = VirtualEnvUtil::buildWithProjectGroup(topic, m_projectGroupPrefix);
    }
	
	SearchOffsetRequestHeader* requestHeader = new SearchOffsetRequestHeader();
	requestHeader->setTopic(topicWithProjectGroup);
	requestHeader->setQueueId(queueId);
	requestHeader->setTimestamp(timestamp);
	
	MqLogDebug("SearchOffsetRequest topic(%s) QueueID(%d) timestamp(%lld)", topicWithProjectGroup.c_str(), queueId, timestamp);
	RemotingCommand* request =
		RemotingCommand::createRequestCommand(SEARCH_OFFSET_BY_TIMESTAMP_VALUE, requestHeader);
    request->Encode();
    RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
    if(response != NULL)
    {
        int code = response->getCode();
        switch (code) 
        {
			case SUCCESS_VALUE: 
			{
				SearchOffsetResponseHeader* responseHeader = (SearchOffsetResponseHeader*)response->getCommandCustomHeader();
				long long offset = responseHeader->getOffset();
				delete response;
				MqLogDebug("SearchOffset Response Offset [%lld]", offset);
				return offset;
			}
			default:
				break;
        }
        std::string remark = response->getRemark();        
        delete response;

        THROW_MQEXCEPTION(MQClientException, remark,code);
    }
	
	return 0;
}

std::list<std::string> MQClientAPIImpl::getConsumerIdListByGroup(const std::string& addr,
																const std::string& consumerGroup,
																int timeoutMillis)
{
	// ����������л�����ص�projectGroupPrefix
	std::string consumerGroupWithProjectGroup = consumerGroup;
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		consumerGroupWithProjectGroup =
			VirtualEnvUtil::buildWithProjectGroup(consumerGroup, m_projectGroupPrefix);
	}

	GetConsumerListByGroupRequestHeader* requestHeader = new GetConsumerListByGroupRequestHeader();
	requestHeader->consumerGroup = consumerGroupWithProjectGroup;
	RemotingCommand* request =
		RemotingCommand::createRequestCommand(GET_CONSUMER_LIST_BY_GROUP_VALUE, requestHeader);

	request->Encode();

	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
	if (response)
	{
	    /* modified by yu.guangjie at 2015-08-14, reason: */
	    int code = response->getCode();
		switch (code)
		{
		case SUCCESS_VALUE:
			{
				if (response->GetBody() != NULL)
				{
					GetConsumerListByGroupResponseBody* body =
						GetConsumerListByGroupResponseBody::Decode((char*)response->GetBody(),response->GetBodyLen());
                    std::list<std::string> conList = body->getConsumerIdList();
                    delete response;
                    delete body;
                    return conList;
				}
			}
		default:
			break;
		}
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException, remark,code);
	}

	THROW_MQEXCEPTION(MQClientException, "getConsumerIdListByGroup failed",-1);
}


long long MQClientAPIImpl::getMaxOffset( const std::string& addr,
		const std::string& topic,
		int queueId,
		int timeoutMillis)
{
	/* modified by liang.haibo at 2016-10-10 , reason: add getMaxOffset*/
	MqLogVerb("getMaxOffset on broker: address(%s), topic(%s), queueid(%d)", 
			  addr.c_str(), topic.c_str(), queueId);
	
	// ����������л�����ص�projectGroupPrefix
    std::string topicWithProjectGroup = topic;
    if (!UtilAll::isBlank(m_projectGroupPrefix)) 
    {
        topicWithProjectGroup = VirtualEnvUtil::buildWithProjectGroup(topic, m_projectGroupPrefix);
    }
	
	GetMaxOffsetRequestHeader* requestHeader = new GetMaxOffsetRequestHeader();
	requestHeader->setTopic(topicWithProjectGroup);
	requestHeader->setQueueId(queueId);

	RemotingCommand* request =
		RemotingCommand::createRequestCommand(GET_MAX_OFFSET_VALUE, requestHeader);
    request->Encode();
    RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
    if(response != NULL)
    {
        int code = response->getCode();
        switch (code) 
        {
			case SUCCESS_VALUE: 
			{
				GetMaxOffsetResponseHeader* responseHeader = ( GetMaxOffsetResponseHeader*)response->getCommandCustomHeader();
				long long offset = responseHeader->getOffset();
				delete response;
				MqLogDebug("GetMaxOffset Response Offset [%lld]", offset);
				return offset;
			}
			default:
				break;
        }
        std::string remark = response->getRemark();        
        delete response;

        THROW_MQEXCEPTION(MQClientException, remark,code);
    }
	
	return 0;
}


long long MQClientAPIImpl::getMinOffset(const std::string& addr,
										const std::string& topic,
										int queueId,
										int timeoutMillis)
{
    /* modified by yu.guangjie at 2015-08-17, reason: add getMinOffset*/
    // ����������л�����ص�projectGroupPrefix
    std::string topicWithProjectGroup = topic;
    if (!UtilAll::isBlank(m_projectGroupPrefix)) 
    {
        topicWithProjectGroup = VirtualEnvUtil::buildWithProjectGroup(topic, m_projectGroupPrefix);
    }

    GetMinOffsetRequestHeader* requestHeader = new GetMinOffsetRequestHeader();
    requestHeader->setTopic(topicWithProjectGroup);
    requestHeader->setQueueId(queueId);
    RemotingCommand* request =
            RemotingCommand::createRequestCommand(GET_MIN_OFFSET_VALUE, requestHeader);
    request->Encode();
    RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
    if(response != NULL)
    {
        int code = response->getCode();
        switch (code) 
        {
        case SUCCESS_VALUE: 
        {
            GetMinOffsetResponseHeader* responseHeader =
                    (GetMinOffsetResponseHeader*)response->getCommandCustomHeader();
            long long offset = responseHeader->getOffset();
            delete response;
            
            return offset;
        }
        default:
            break;
        }
        std::string remark = response->getRemark();        
        delete response;

        THROW_MQEXCEPTION(MQClientException, remark,code);
    }
    
	return 0;
}

long long MQClientAPIImpl::getEarliestMsgStoretime(const std::string& addr,
													const std::string& topic,
													int queueId,
													int timeoutMillis)
{
	/* modified by liang.haibo at 2016-10-10 , reason: */
	MqLogVerb("getMaxOffset on broker: address(%s), topic(%s), queueid(%d)", 
			  addr.c_str(), topic.c_str(), queueId);
	
	// ����������л�����ص�projectGroupPrefix
    std::string topicWithProjectGroup = topic;
    if (!UtilAll::isBlank(m_projectGroupPrefix)) 
    {
        topicWithProjectGroup = VirtualEnvUtil::buildWithProjectGroup(topic, m_projectGroupPrefix);
    }
	
	GetEarliestMsgStoretimeRequestHeader* requestHeader = new GetEarliestMsgStoretimeRequestHeader();
	requestHeader->setTopic(topicWithProjectGroup);
	requestHeader->setQueueId(queueId);

	RemotingCommand* request =
		RemotingCommand::createRequestCommand(GET_EARLIEST_MSG_STORETIME_VALUE, requestHeader);
    request->Encode();
    RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
    if(response != NULL)
    {
        int code = response->getCode();
        switch (code) 
        {
			case SUCCESS_VALUE: 
			{
				GetEarliestMsgStoretimeResponseHeader* responseHeader = ( GetEarliestMsgStoretimeResponseHeader*)response->getCommandCustomHeader();
				long long timestamp = responseHeader->getTimestamp();
				delete response;
				MqLogDebug("getEarliestMsgStoretime timestamp [%lld]",  timestamp );
				return timestamp;
			}
			default:
				break;
        }
        std::string remark = response->getRemark();        
        delete response;

        THROW_MQEXCEPTION(MQClientException, remark,code);
    }
	
	return 0;
}

long long MQClientAPIImpl::queryConsumerOffset(const std::string& addr,
												QueryConsumerOffsetRequestHeader* pRequestHeader,
												int timeoutMillis)
{
    /* modified by yu.guangjie at 2015-08-16, reason: add queryConsumerOffset */
    
    // ����������л�����ص�projectGroupPrefix
    if (!UtilAll::isBlank(m_projectGroupPrefix)) {
        pRequestHeader->setConsumerGroup(VirtualEnvUtil::buildWithProjectGroup(
            pRequestHeader->getConsumerGroup(), m_projectGroupPrefix));
        pRequestHeader->setTopic(VirtualEnvUtil::buildWithProjectGroup(
            pRequestHeader->getTopic(), m_projectGroupPrefix));
    }

    RemotingCommand* request =
            RemotingCommand::createRequestCommand(QUERY_CONSUMER_OFFSET_VALUE, pRequestHeader);
    request->Encode();
    RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
    if(response != NULL)
    {
        int code = response->getCode();
        switch (code) 
        {
        case SUCCESS_VALUE: 
        {
            QueryConsumerOffsetResponseHeader* responseHeader = (QueryConsumerOffsetResponseHeader*)response->getCommandCustomHeader();
            long long offset = responseHeader->getOffset();
            delete response;
            
            return offset;
        }
        default:
            break;
        }
        std::string remark = response->getRemark();        
        delete response;

		/** Note by lin.qiongshan, 2016-11-10��Ϊʲô��ô��
			RebalancePushImpl::computePullFromWhere
				==> RemoteBrokerOffsetStore::readOffset
					==> RemoteBrokerOffsetStore::fetchConsumeOffsetFromBroker
						==> ������
			���������׳� MQBrokerException �쳣ʱ��RemoteBrokerOffsetStore::readOffset �᷵�� -1���� RebalancePushImpl::computePullFromWhere �����ʾ�״�ע�ᣬ�������ôӺδ����Ѷ�λ��ʼ������λ��
				�����ԭ���� MQClientException, RemoteBrokerOffsetStore::readOffset �᷵�� -2��RebalancePushImpl::computePullFromWhere �޷���ȷ����
			�����״�ע�᣺
				consumer ���� code(14) ���� offset ����ʱ����� consumer_group ���״�ע�ᣬ����˵���Ӧ�������� 22���������׳��쳣�����ⲿ�ͻ��˸��� CONSUMER_FROM_WHERE �����ü�����ʼ����λ��
		*/
		/* modified by yu.guangjie at 2016-11-10, reason: change MQClientException to MQBrokerException*/
		THROW_MQEXCEPTION(MQBrokerException, remark, code);
    }   

	return 0;
}

void MQClientAPIImpl::updateConsumerOffset(const std::string& addr,
											UpdateConsumerOffsetRequestHeader* pRequestHeader,
											int timeoutMillis)
{
	/* modified by liang.haibo at 2016-10-11, reason�� add updateConsumerOffset 
	 * updateConsumerOffset used only by admin tools
	 * broker use updateConsumerOffsetOneway
	 */
	// ����������л�����ص�projectGroupPrefix
    if (!UtilAll::isBlank(m_projectGroupPrefix)) 
    {
        pRequestHeader->setConsumerGroup(VirtualEnvUtil::buildWithProjectGroup(
											 pRequestHeader->getConsumerGroup(), m_projectGroupPrefix));
        pRequestHeader->setTopic(VirtualEnvUtil::buildWithProjectGroup(
									 pRequestHeader->getTopic(), m_projectGroupPrefix));
    }

    RemotingCommand* request =
		RemotingCommand::createRequestCommand(UPDATE_CONSUMER_OFFSET_VALUE, pRequestHeader);
    request->Encode();
	
    RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
    if(response != NULL)
    {
        int code = response->getCode();
        switch (code) 
        {
			case SUCCESS_VALUE: 
			{
				return;
			}
			default:
				break;
        }
        std::string remark = response->getRemark();        
        delete response;

        THROW_MQEXCEPTION(MQClientException, remark,code);
    }   
}

void MQClientAPIImpl::updateConsumerOffsetOneway(const std::string& addr,
												UpdateConsumerOffsetRequestHeader* pRequestHeader,
												int timeoutMillis)
{
    /* modified by yu.guangjie at 2015-08-16, reason: add updateConsumerOffsetOneway*/
	
    // ����������л�����ص�projectGroupPrefix
    if (!UtilAll::isBlank(m_projectGroupPrefix)) 
    {
        pRequestHeader->setConsumerGroup(VirtualEnvUtil::buildWithProjectGroup(
            pRequestHeader->getConsumerGroup(), m_projectGroupPrefix));
        pRequestHeader->setTopic(VirtualEnvUtil::buildWithProjectGroup(
            pRequestHeader->getTopic(), m_projectGroupPrefix));
    }

    RemotingCommand* request =
            RemotingCommand::createRequestCommand(UPDATE_CONSUMER_OFFSET_VALUE, pRequestHeader);
    request->Encode();
    m_pRemotingClient->invokeOneway(addr, *request, timeoutMillis);

    delete request;

}

void MQClientAPIImpl::sendHearbeat(const std::string& addr, HeartbeatData* pHeartbeatData, int timeoutMillis)
{
	// ����������л�����ص�projectGroupPrefix
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		std::set<ConsumerData>& consumerDatas = pHeartbeatData->getConsumerDataSet();
		std::set<ConsumerData>::iterator it = consumerDatas.begin();

		for (; it!= consumerDatas.end();it++)
		{
			ConsumerData& consumerData = (ConsumerData&)(*it);

			consumerData.groupName = VirtualEnvUtil::buildWithProjectGroup(consumerData.groupName,
				m_projectGroupPrefix);
			std::set<SubscriptionData>& subscriptionDatas = consumerData.subscriptionDataSet;
			std::set<SubscriptionData>::iterator itsub=subscriptionDatas.begin();

			for (; itsub != subscriptionDatas.end();itsub++)
			{
				SubscriptionData& subscriptionData = (SubscriptionData&)(*itsub);
				subscriptionData.setTopic(VirtualEnvUtil::buildWithProjectGroup(
					subscriptionData.getTopic(), m_projectGroupPrefix));
			}
		}

		std::set<ProducerData>& producerDatas = pHeartbeatData->getProducerDataSet();
		std::set<ProducerData>::iterator itp = producerDatas.begin();
		for (;itp!=producerDatas.end();itp++)
		{
			ProducerData& producerData = (ProducerData&)(*itp);
			producerData.groupName = VirtualEnvUtil::buildWithProjectGroup(producerData.groupName,
				m_projectGroupPrefix);
		}
	}

	RemotingCommand* request = RemotingCommand::createRequestCommand(HEART_BEAT_VALUE, NULL);
	
	std::string body;
	pHeartbeatData->Encode(body);
	request->SetBody((char*)body.data(),body.length(),true);

	request->Encode();

	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
	if (response)
	{
	    /* modified by yu.guangjie at 2015-08-14, reason: */
	    int code = response->getCode();
		switch (code)
		{
		case SUCCESS_VALUE:
			{
                delete response;
				return;
			}
		default:
			break;
		}        
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException, remark,code);
	}

	THROW_MQEXCEPTION(MQClientException, "sendHearbeat failed",-1);
}

void MQClientAPIImpl::unregisterClient(const std::string& addr,
										const std::string& clientID,
										const std::string& producerGroup,
										const std::string& consumerGroup,
										int timeoutMillis)
{
    /* modified by yu.guangjie at 2015-08-17, reason: add unregisterClient */
    MqLogVerb("unregister[%s & %s] on broker: address(%s), clientid(%s)", 
        producerGroup.c_str(), consumerGroup.c_str(), addr.c_str(), clientID.c_str());
    
    // ����������л�����ص�projectGroupPrefix
    std::string producerGroupWithProjectGroup = producerGroup;
    std::string consumerGroupWithProjectGroup = consumerGroup;
    if (!UtilAll::isBlank(m_projectGroupPrefix)) 
    {
        producerGroupWithProjectGroup =
            VirtualEnvUtil::buildWithProjectGroup(producerGroup, m_projectGroupPrefix);
        consumerGroupWithProjectGroup =
            VirtualEnvUtil::buildWithProjectGroup(consumerGroup, m_projectGroupPrefix);
    }

    UnregisterClientRequestHeader* requestHeader = new UnregisterClientRequestHeader();
    requestHeader->setClientID(clientID);
    requestHeader->setProducerGroup(producerGroupWithProjectGroup);
    requestHeader->setConsumerGroup(consumerGroupWithProjectGroup);
    RemotingCommand* request =
            RemotingCommand::createRequestCommand(UNREGISTER_CLIENT_VALUE, requestHeader);
    request->Encode();
    RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
    if(response != NULL)
    {
        int code = response->getCode();
        switch (code) 
        {
        case SUCCESS_VALUE: 
            {
                delete response;
                return;
            }
        default:
            break;
        }
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException, remark,code);
    }
}

void MQClientAPIImpl::endTransactionOneway(const std::string& addr,
											EndTransactionRequestHeader* pRequestHeader,
											const std::string& remark,
											int timeoutMillis)
{
	//TODO
}

void MQClientAPIImpl::queryMessage(const std::string& addr,
									QueryMessageRequestHeader* pRequestHeader,
									int timeoutMillis,
									InvokeCallback* pInvokeCallback)
{
	//TODO
}

bool MQClientAPIImpl::registerClient( const std::string& addr, HeartbeatData* pHeartbeatData, int timeoutMillis)
{
	// ����������л�����ص�projectGroupPrefix
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		std::set<ConsumerData>& consumerDatas = pHeartbeatData->getConsumerDataSet();
		std::set<ConsumerData>::iterator it = consumerDatas.begin();

		for (; it!= consumerDatas.end();it++)
		{
			ConsumerData& consumerData = (ConsumerData&)(*it);

			consumerData.groupName = VirtualEnvUtil::buildWithProjectGroup(consumerData.groupName,
									 m_projectGroupPrefix);
			std::set<SubscriptionData>& subscriptionDatas = consumerData.subscriptionDataSet;
			std::set<SubscriptionData>::iterator itsub=subscriptionDatas.begin();

			for (; itsub != subscriptionDatas.end();itsub++)
			{
				SubscriptionData& subscriptionData = (SubscriptionData&)(*itsub);
				subscriptionData.setTopic(VirtualEnvUtil::buildWithProjectGroup(
											  subscriptionData.getTopic(), m_projectGroupPrefix));
			}
		}

		std::set<ProducerData>& producerDatas = pHeartbeatData->getProducerDataSet();
		std::set<ProducerData>::iterator itp = producerDatas.begin();
		for (;itp!=producerDatas.end();itp++)
		{
			ProducerData& producerData = (ProducerData&)(*itp);
			producerData.groupName = VirtualEnvUtil::buildWithProjectGroup(producerData.groupName,
									 m_projectGroupPrefix);
		}
	}

	RemotingCommand* request = RemotingCommand::createRequestCommand(HEART_BEAT_VALUE, NULL);
	
	std::string body;
	pHeartbeatData->Encode(body);
	request->SetBody((char*)body.data(),body.length(),true);
	request->Encode();

	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
	if (response != NULL)
	{
	    return response->getCode() == SUCCESS_VALUE;
	}

	THROW_MQEXCEPTION(MQClientException, "RegisterClient failed",-1);
	
}

void MQClientAPIImpl::consumerSendMessageBack(MessageExt& msg,
											  const std::string& consumerGroup,
											  int delayLevel,
											  int timeoutMillis)
{
	// ����������л�����ص�projectGroupPrefix
	std::string consumerGroupWithProjectGroup = consumerGroup;
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		consumerGroupWithProjectGroup =
			VirtualEnvUtil::buildWithProjectGroup(consumerGroup, m_projectGroupPrefix);
		msg.setTopic(VirtualEnvUtil::buildWithProjectGroup(msg.getTopic(), m_projectGroupPrefix));
	}

	ConsumerSendMsgBackRequestHeader* requestHeader = new ConsumerSendMsgBackRequestHeader();
	RemotingCommand* request =
		RemotingCommand::createRequestCommand(CONSUMER_SEND_MSG_BACK_VALUE,
		requestHeader);
	requestHeader->group = consumerGroupWithProjectGroup;
	requestHeader->offset = msg.getCommitLogOffset();
	requestHeader->delayLevel = delayLevel;

	std::string addr = socketAddress2IPPort(msg.getStoreHost());

	request->Encode();

	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
	if (response)
	{
        /* modified by yu.guangjie at 2015-08-14, reason: */
	    int code = response->getCode();
		switch (code)
		{
		case SUCCESS_VALUE:
            delete response;
			return;
		default:
			break;
		}
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException, remark,code);
	}

	THROW_MQEXCEPTION(MQClientException, "consumerSendMessageBack failed",-1);
}

std::set<MessageQueue> MQClientAPIImpl::lockBatchMQ(const std::string& addr,
													LockBatchRequestBody* pRequestBody,
													int timeoutMillis)
{
	// ����������л�����ص�projectGroupPrefix
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		pRequestBody->setConsumerGroup((VirtualEnvUtil::buildWithProjectGroup(
			pRequestBody->getConsumerGroup(), m_projectGroupPrefix)));
		std::set<MessageQueue>& messageQueues = pRequestBody->getMqSet();
		std::set<MessageQueue>::iterator it = messageQueues.begin();

		for (; it != messageQueues.end();it++)
		{
			MessageQueue& messageQueue = (MessageQueue&)(*it);
			messageQueue.setTopic(VirtualEnvUtil::buildWithProjectGroup(messageQueue.getTopic(),
				m_projectGroupPrefix));
		}
	}

	RemotingCommand* request = RemotingCommand::createRequestCommand(LOCK_BATCH_MQ_VALUE, NULL);
	
	std::string body;
	pRequestBody->Encode(body);
	request->SetBody((char*)body.data(),body.length(),true);
	request->Encode();

	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
    delete request;
	if (response)
	{
	    /* modified by yu.guangjie at 2015-08-14, reason: */
	    int code = response->getCode();
		switch (code)
		{
		case SUCCESS_VALUE:
			{
				LockBatchResponseBody* responseBody =
					LockBatchResponseBody::Decode((char*)response->GetBody(),response->GetBodyLen());
				std::set<MessageQueue> messageQueues = responseBody->getLockOKMQSet();

				// ����������л�����ص�projectGroupPrefix
				if (!UtilAll::isBlank(m_projectGroupPrefix))
				{
					std::set<MessageQueue>::iterator it = messageQueues.begin();

					for (; it != messageQueues.end();it++)
					{
						MessageQueue& messageQueue = (MessageQueue&)(*it);
						messageQueue.setTopic(VirtualEnvUtil::clearProjectGroup(messageQueue.getTopic(),
							m_projectGroupPrefix));
					}
				}
				//add by lin.qs@2017-4-5
				delete responseBody;
				responseBody = NULL;

                delete response;
                
				return messageQueues;
			}
		default:
			break;
		}
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException, remark,code);
	}

	THROW_MQEXCEPTION(MQClientException, "lockBatchMQ failed",-1);
}

void MQClientAPIImpl::unlockBatchMQ(const std::string& addr,
	UnlockBatchRequestBody* pRequestBody,
	int timeoutMillis,
	bool oneway)
{
	// ����������л�����ص�projectGroupPrefix
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		pRequestBody->setConsumerGroup((VirtualEnvUtil::buildWithProjectGroup(
			pRequestBody->getConsumerGroup(), m_projectGroupPrefix)));
		std::set<MessageQueue>& messageQueues = pRequestBody->getMqSet();
		std::set<MessageQueue>::iterator it = messageQueues.begin();

		for (; it != messageQueues.end();it++)
		{
			MessageQueue& messageQueue = (MessageQueue&)(*it);
			messageQueue.setTopic(VirtualEnvUtil::buildWithProjectGroup(messageQueue.getTopic(),
				m_projectGroupPrefix));
		}
	}

	RemotingCommand* request = RemotingCommand::createRequestCommand(UNLOCK_BATCH_MQ_VALUE, NULL);

	std::string body;
	pRequestBody->Encode(body);
	request->SetBody((char*)body.data(),body.length(),true);
	request->Encode();

	if (oneway)
	{
		m_pRemotingClient->invokeOneway(addr, *request, timeoutMillis);
        delete request;
	}
	else
	{
		RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *request, timeoutMillis);
        delete request;
		if (response)
		{
            /* modified by yu.guangjie at 2015-08-14, reason: */
		    int code = response->getCode();
			switch (code)
			{
			case SUCCESS_VALUE:
                delete response;
				return;
			default:
				break;
			}
            std::string remark = response->getRemark();        
            delete response;

			THROW_MQEXCEPTION(MQClientException, remark,code);
		}

		THROW_MQEXCEPTION(MQClientException, "unlockBatchMQ failed",-1);
	}
}

TopicStatsTable MQClientAPIImpl::getTopicStatsInfo(const std::string& addr,
	const std::string& topic,
													int timeoutMillis)
{
	//TODO
	TopicStatsTable t;
	return t;
}

ConsumeStats MQClientAPIImpl::getConsumeStats(const std::string& addr,
											  const std::string& consumerGroup,
											  int timeoutMillis)
{
	//TODO
	ConsumeStats cs;
	return cs;
}

ProducerConnection* MQClientAPIImpl::getProducerConnectionList(const std::string& addr,
																const std::string& producerGroup,
																int timeoutMillis)
{
	//TODO
	return NULL;
}

ConsumerConnection* MQClientAPIImpl::getConsumerConnectionList(const std::string& addr,
																const std::string& consumerGroup,
																int timeoutMillis)
{
	//TODO
	return NULL;
}

KVTable MQClientAPIImpl::getBrokerRuntimeInfo( const std::string& addr,  int timeoutMillis)
{
	//TODO
	KVTable kv;
	return kv;
}

void MQClientAPIImpl::updateBrokerConfig( const std::string& addr,
	const std::map<std::string,std::string>&  properties,
	int timeoutMillis)
{
	//TODO
}

ClusterInfo* MQClientAPIImpl::getBrokerClusterInfo( int timeoutMillis)
{
	//TODO
	return NULL;
}

TopicRouteData* MQClientAPIImpl::getDefaultTopicRouteInfoFromNameServer(const std::string& topic,
																		int timeoutMillis)
{
    MqLogVerb("Get topic[%s] routeinfo from NS: %s", topic.c_str(), m_nameSrvAddr.c_str());
	GetRouteInfoRequestHeader* requestHeader = new GetRouteInfoRequestHeader();
	requestHeader->topic=topic;

	RemotingCommand* request = RemotingCommand::createRequestCommand(GET_ROUTEINTO_BY_TOPIC_VALUE,requestHeader);
	request->Encode();

	RemotingCommand* response = m_pRemotingClient->invokeSync(m_nameSrvAddr, *request, timeoutMillis);
    delete request;
	if (response)
	{
	    int code = response->getCode();
		switch (code)
		{
		case TOPIC_NOT_EXIST_VALUE:
			{
				break;
			}
		case SUCCESS_VALUE:
			{
				int bodyLen = response->GetBodyLen();
				const char* body = response->GetBody();
				if (body)
				{
                    /* modified by yu.guangjie at 2015-08-14, reason: */
					TopicRouteData *route = TopicRouteData::Decode(body,bodyLen);
                    delete response;
                    return route;
				}
			}
		default:
			break;
		}
        /* modified by yu.guangjie at 2015-08-13, reason: */
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException,remark,code);
	}

	return NULL;
}

TopicRouteData* MQClientAPIImpl::getTopicRouteInfoFromNameServer(const std::string& topic, int timeoutMillis)
{
	// ����������л�����ص�projectGroupPrefix
	std::string topicWithProjectGroup = topic;
	if (!UtilAll::isBlank(m_projectGroupPrefix))
	{
		topicWithProjectGroup = VirtualEnvUtil::buildWithProjectGroup(topic, m_projectGroupPrefix);
	}
    
    /* modified by yu.guangjie at 2015-08-14, reason: use getDefaultTopicRouteInfoFromNameServer*/
    return getDefaultTopicRouteInfoFromNameServer(topicWithProjectGroup, timeoutMillis);
}

TopicList* MQClientAPIImpl::getTopicListFromNameServer( int timeoutMillis)
{
	RemotingCommand* request = RemotingCommand::createRequestCommand(GET_ALL_TOPIC_LIST_FROM_NAMESERVER_VALUE,NULL);
	request->Encode();
	RemotingCommand* response = m_pRemotingClient->invokeSync(m_nameSrvAddr, *request, timeoutMillis);
    delete request;
	if (response)
	{
	    int code = response->getCode();
		switch (code)
		{
		case SUCCESS_VALUE:
			{
				char* body = (char*)response->GetBody();
				if (body != NULL)
				{
					TopicList* topicList = TopicList::Decode(body, response->GetBodyLen());

					if (!UtilAll::isBlank(m_projectGroupPrefix))
					{
						std::set<std::string> newTopicSet;

						const std::set<std::string>& topics = topicList->getTopicList();
						std::set<std::string>::const_iterator it = topics.begin();
						for (;it!=topics.end();it++)
						{
							std::string topic = *it;
							newTopicSet.insert(VirtualEnvUtil::clearProjectGroup(topic, m_projectGroupPrefix));
						}

						topicList->setTopicList(newTopicSet);
					}

                    delete response;

					return topicList;
				}
			}
		default:
			break;
		}
        /* modified by yu.guangjie at 2015-08-13, reason: */
        std::string remark = response->getRemark();        
        delete response;

		THROW_MQEXCEPTION(MQClientException,remark,code);
	}

	return NULL;
}

int MQClientAPIImpl::wipeWritePermOfBroker(const std::string& namesrvAddr,
											const std::string& brokerName,
											int timeoutMillis)
{
	//TODO
	return 0;
}

void MQClientAPIImpl::deleteTopicInBroker(const std::string& addr,
										  const std::string& topic,
										  int timeoutMillis)
{
	//TODO
}

void MQClientAPIImpl::deleteTopicInNameServer(const std::string& addr,
											  const std::string& topic,
											  int timeoutMillis)
{
	//TODO
}

void MQClientAPIImpl::deleteSubscriptionGroup(const std::string& addr,
											  const std::string& groupName,
											  int timeoutMillis)
{
	//TODO
}

std::string MQClientAPIImpl::getKVConfigValue(const std::string& projectNamespace,
											  const std::string& key,
											  int timeoutMillis)
{
	//TODO
	return "";
}

void MQClientAPIImpl::putKVConfigValue(const std::string& projectNamespace,
										const std::string& key,
										const std::string& value,
										int timeoutMillis)
{
	//TODO
}

void MQClientAPIImpl::deleteKVConfigValue(const std::string& projectNamespace,
										  const std::string& key,
										  int timeoutMillis)
{
	//TODO
}

std::string MQClientAPIImpl::getProjectGroupByIp(const std::string& ip,  int timeoutMillis)
{
	return getKVConfigValue(NamesrvUtil::NAMESPACE_PROJECT_CONFIG, ip, timeoutMillis);
}

std::string MQClientAPIImpl::getKVConfigByValue(const std::string& projectNamespace,
												const std::string& projectGroup,
												int timeoutMillis)
{
	//TODO
	return "";
}

KVTable MQClientAPIImpl::getKVListByNamespace( const std::string& projectNamespace,  int timeoutMillis)
{
	//TODO

	return KVTable();
}

void MQClientAPIImpl::deleteKVConfigByValue(const std::string& projectNamespace,
											const std::string& projectGroup,
											int timeoutMillis)
{
	//TODO
}

SendResult* MQClientAPIImpl::sendMessageSync(const std::string& addr,
											 const std::string& brokerName,
											 Message& msg,
											 int timeoutMillis,
											 RemotingCommand& request)
{
	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, request, timeoutMillis);
    /* modified by yu.guangjie at 2015-08-14, reason: */
	SendResult *result = processSendResponse(brokerName, msg, response);
    delete response;
    
    return result;
}

void MQClientAPIImpl::sendMessageAsync(const std::string& addr,
										const std::string& brokerName,
										Message& msg,
										int timeoutMillis,
										RemotingCommand& request,
										SendCallback* pSendCallback)
{
	/* modified by liang.haibo at 2016-09-18 , reason: procuder sender callback */
	ProducerInvokeCallback* callback = new ProducerInvokeCallback(brokerName, msg, pSendCallback, this);
	m_pRemotingClient->invokeAsync(addr, request, timeoutMillis,callback );
}

SendResult* MQClientAPIImpl::processSendResponse(const std::string& brokerName,
												 Message& msg,
												 RemotingCommand* pResponse)
{
    /* modified by yu.guangjie at 2015-08-31, reason: */
	if (pResponse==NULL)
	{
	    std::string strMsg = "Can't connect/send to broker message: " + brokerName;
	    THROW_MQEXCEPTION(MQBrokerException, strMsg.c_str(),-1);
		return NULL;
	}

	switch (pResponse->getCode())
	{
	case FLUSH_DISK_TIMEOUT_VALUE:
	case FLUSH_SLAVE_TIMEOUT_VALUE:
	case SLAVE_NOT_AVAILABLE_VALUE:
		{
			// TODO LOG
		}
	case SUCCESS_VALUE:
		{
			SendStatus sendStatus = SEND_OK;
			switch (pResponse->getCode())
			{
			case FLUSH_DISK_TIMEOUT_VALUE:
				sendStatus = FLUSH_DISK_TIMEOUT;
				break;
			case FLUSH_SLAVE_TIMEOUT_VALUE:
				sendStatus = FLUSH_SLAVE_TIMEOUT;
				break;
			case SLAVE_NOT_AVAILABLE_VALUE:
				sendStatus = SLAVE_NOT_AVAILABLE;
				break;
			case SUCCESS_VALUE:
				sendStatus = SEND_OK;
				break;
			default:
				//assert false;
				break;
			}

			SendMessageResponseHeader* responseHeader = (SendMessageResponseHeader*) pResponse->getCommandCustomHeader();

			MessageQueue messageQueue (msg.getTopic(), brokerName, responseHeader->queueId);

			return new SendResult(sendStatus, responseHeader->msgId, messageQueue,
				responseHeader->queueOffset, m_projectGroupPrefix);
		}
	default:
		break;
	}

	THROW_MQEXCEPTION(MQBrokerException,pResponse->getRemark(),pResponse->getCode());
	return NULL;
}

void MQClientAPIImpl::pullMessageAsync(const std::string& addr,
										RemotingCommand* pRequest,
										int timeoutMillis,
										PullCallback* pPullCallback)
{
	ConsumerInvokeCallback* callback = new ConsumerInvokeCallback(pPullCallback, this);
	m_pRemotingClient->invokeAsync(addr, *pRequest, timeoutMillis,callback);
}

PullResult* MQClientAPIImpl::processPullResponse( RemotingCommand* pResponse)
{
	PullStatus pullStatus = NO_NEW_MSG;
	switch (pResponse->getCode())
	{
	case SUCCESS_VALUE:
		pullStatus = FOUND;
		break;
	case PULL_NOT_FOUND_VALUE:
		pullStatus = NO_NEW_MSG;
		break;
	case PULL_RETRY_IMMEDIATELY_VALUE:
		pullStatus = NO_MATCHED_MSG;
		break;
	case PULL_OFFSET_MOVED_VALUE:
		pullStatus = OFFSET_ILLEGAL;
		break;

	default:
		THROW_MQEXCEPTION(MQBrokerException,pResponse->getRemark(),pResponse->getCode());
		break;
	}

	PullMessageResponseHeader* responseHeader =(PullMessageResponseHeader*) pResponse->getCommandCustomHeader();
	std::list<MessageExt*> msgFoundList;
	return new PullResultExt(pullStatus, responseHeader->nextBeginOffset,
		responseHeader->minOffset, responseHeader->maxOffset, msgFoundList,
		responseHeader->suggestWhichBrokerId, pResponse->GetBody(),pResponse->GetBodyLen());
}
 
PullResult* MQClientAPIImpl::pullMessageSync(const std::string& addr,
											 RemotingCommand* pRequest,
											 int timeoutMillis)
{
	RemotingCommand* response = m_pRemotingClient->invokeSync(addr, *pRequest, timeoutMillis);

	//mdy by lin.qs, @2017-5-9, ע�⣬���� invokeSync ������ response �п���Ϊ�գ�û�л�ȡ������˵���Ӧ��������ʧ�ܵ������
	if (NULL == response)
	{
		THROW_MQEXCEPTION(MQBrokerException, "ͬ����Ϣ��ȡʧ��, û���յ�����˵���Ӧ", 0);
	}

	PullResult* result = processPullResponse(response);
    /* SetBody(NULL,0,false): because result has the body */
	response->SetBody(NULL,0,false);
	delete response;

	return result;
}
