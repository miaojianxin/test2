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

#include "CommandCustomHeader.h"
#include <sstream>
#include <cstdlib>

#include "RemotingCommand.h"
#include "MQProtos.h"
#include "KPRUtil.h"
#include "json/json.h"
#include "UtilAll.h"

CommandCustomHeader* CommandCustomHeader::Decode(int code,char* pData,int len,bool isResponseType)
{
	if (isResponseType)
	{
		switch(code)
		{
		case SEND_MESSAGE_VALUE:
			return SendMessageResponseHeader::Decode(pData,len);
			break;
		case PULL_MESSAGE_VALUE:
			return PullMessageResponseHeader::Decode(pData,len);
			break;
		case GET_MAX_OFFSET_VALUE:
		case GET_MIN_OFFSET_VALUE:
        case QUERY_CONSUMER_OFFSET_VALUE:
			return QueryConsumerOffsetResponseHeader::Decode(pData,len);
			break;
        case GET_CONSUMER_LIST_BY_GROUP_VALUE:
            return GetConsumerListByGroupResponseHeader::Decode(pData,len);
            break;
		//lin.qiongshan, 2016年8月18日10:07:46, 一些请求的响应没有自定义字段，统一使用 ResponseHaveNoCustomHeader 进行处理
		case HEART_BEAT_VALUE:
		case UNREGISTER_CLIENT_VALUE:
			return ResponseHaveNoCustomHeader::Decode(code, pData, len);
			break;
			
		case SEARCH_OFFSET_BY_TIMESTAMP_VALUE:
			return SearchOffsetResponseHeader::Decode(pData,len);
			break;
			
		case GET_EARLIEST_MSG_STORETIME_VALUE:
			return GetEarliestMsgStoretimeResponseHeader::Decode(pData, len); 
			break;
		default:
            MqLogWarn("UNKOWN response code: %d", code);
			break;
		}
	}
    else
    {
        switch(code)
        {
        case NOTIFY_CONSUMER_IDS_CHANGED_VALUE:
            return NotifyConsumerIdsChangedRequestHeader::Decode(pData,len);
            break;
        default:
            MqLogWarn("UNKOWN request code: %d", code);
            break;
        }
    }

	return NULL;
}

//
//GetRouteInfoRequestHeader
//
GetRouteInfoRequestHeader::GetRouteInfoRequestHeader()
{
}

GetRouteInfoRequestHeader::~GetRouteInfoRequestHeader()
{

}

void GetRouteInfoRequestHeader::Encode(std::string& outData)
{
	std::stringstream ss;
	ss<<"{"<<"\"topic\":"<<"\""<<topic<<"\"}";

	outData = ss.str();
}

//
//SubscriptionGroupConfigHeader 
//
SubscriptionGroupConfigHeader::SubscriptionGroupConfigHeader()
{
}

SubscriptionGroupConfigHeader::~SubscriptionGroupConfigHeader()
{
}

void SubscriptionGroupConfigHeader::Encode(std::string& outData)
{
	std::stringstream ss;

	ss<<"{"<<"\"groupName\":"<<"\""<<groupName<<"\","
	  <<"\"consumeEnable\":"<<"\""<<consumeEnable<<"\","
	  <<"\"consumeFromMinEnable\":"<<consumeFromMinEnable<<","
	  <<"\"consumeBroadcastEnable\":"<<consumeBroadcastEnable<<","
	  <<"\"retryQueueNums\":"<<retryQueueNums<<","
	  <<"\"retryMaxTimes\":"<<retryMaxTimes<<","
	  <<"\"brokerId\":"<<brokerId<<","
	  <<"\"whichBrokerWhenConsumeSlowly\":"<<"\""<<whichBrokerWhenConsumeSlowly<<"\""
	  <<"}";
	
	outData = ss.str();
}

//
//CreateTopicRequestHeader
//
CreateTopicRequestHeader::CreateTopicRequestHeader()
{
}

CreateTopicRequestHeader::~CreateTopicRequestHeader()
{
}

void CreateTopicRequestHeader::Encode(std::string& outData)
{
	std::stringstream ss;

	ss<<"{"<<"\"topic\":"<<"\""<<topic<<"\","
	  <<"\"defaultTopic\":"<<"\""<<defaultTopic<<"\","
	  <<"\"readQueueNums\":"<<readQueueNums<<","
	  <<"\"writeQueueNums\":"<<writeQueueNums<<","
	  <<"\"perm\":"<<perm<<","
	  <<"\"topicFilterType\":"<<"\""<<topicFilterType<<"\""
	  <<"}";

	outData = ss.str();
}

//
//SendMessageRequestHeader
//
SendMessageRequestHeader::SendMessageRequestHeader()
{
}

SendMessageRequestHeader::~SendMessageRequestHeader()
{
}

void SendMessageRequestHeader::Encode(std::string& outData)
{
	std::stringstream ss;

	ss<<"{"<<"\"producerGroup\":"<<"\""<<producerGroup<<"\","
	  <<"\"topic\":"<<"\""<<topic<<"\","
	  <<"\"defaultTopic\":"<<"\""<<defaultTopic<<"\","
	  <<"\"defaultTopicQueueNums\":"<<defaultTopicQueueNums<<","
	  <<"\"queueId\":"<<queueId<<","
	  <<"\"sysFlag\":"<<sysFlag<<","
	  <<"\"bornTimestamp\":"<<bornTimestamp<<","
	  <<"\"flag\":"<<flag<<","
	  <<"\"properties\":"<<"\""<<properties<<"\","
	  <<"\"reconsumeTimes\":"<<reconsumeTimes
	  <<"}";

	outData = ss.str();
}
//
//SendMessageResponseHeader
//

SendMessageResponseHeader::SendMessageResponseHeader()
{
}

SendMessageResponseHeader::~SendMessageResponseHeader()
{
}

void SendMessageResponseHeader::Encode(std::string& outData)
{
	std::stringstream ss;

	ss<<"{"<<"\"msgId\":"<<"\""<<msgId<<"\","
	  <<"\"queueId\":"<<queueId<<","
	  <<"\"queueOffset\":"<<queueOffset
	  <<"}";

	outData = ss.str();
}

CommandCustomHeader* SendMessageResponseHeader::Decode(char* pData,int len)
{
	MQJson::Reader reader;
	MQJson::Value object;
	if (!reader.parse(pData+8, object))
	{
		return NULL;
	}

	MQJson::Value ext = object["extFields"];

	std::string msgId = ext["msgId"].asString();
	int queueId = atoi(ext["queueId"].asString().c_str());
	long long queueOffset = str2ll(ext["queueOffset"].asString().c_str());

	SendMessageResponseHeader* h = new SendMessageResponseHeader();

	h->msgId = msgId;
	h->queueId = queueId;
	h->queueOffset = queueOffset;

	return h;
}

//
//PullMessageRequestHeader
//
PullMessageRequestHeader::PullMessageRequestHeader()
{
}

PullMessageRequestHeader::~PullMessageRequestHeader()
{
}

void PullMessageRequestHeader::Encode(std::string& outData)
{
	std::stringstream ss;

	ss<<"{"<<"\"consumerGroup\":"<<"\""<<consumerGroup<<"\","
	  <<"\"topic\":"<<"\""<<topic<<"\","
	  <<"\"queueId\":"<<queueId<<","
	  <<"\"queueOffset\":"<<queueOffset<<","
	  <<"\"maxMsgNums\":"<<maxMsgNums<<","
	  <<"\"sysFlag\":"<<sysFlag<<","
	  <<"\"commitOffset\":"<<commitOffset<<","
	  <<"\"suspendTimeoutMillis\":"<<suspendTimeoutMillis<<","
	  <<"\"subscription\":"<<"\""<<subscription<<"\","
	  <<"\"subVersion\":"<<subVersion
	  <<"}";

	outData = ss.str();
}

//
//PullMessageResponseHeader
//
PullMessageResponseHeader::PullMessageResponseHeader()
{
}

PullMessageResponseHeader::~PullMessageResponseHeader()
{
}

void PullMessageResponseHeader::Encode(std::string& outData)
{
	std::stringstream ss;

	ss<<"{"<<"\"suggestWhichBrokerId\":"<<suggestWhichBrokerId<<","
	  <<"\"nextBeginOffset\":"<<nextBeginOffset<<","
	  <<"\"minOffset\":"<<minOffset<<","
	  <<"\"maxOffset\":"<<maxOffset
	  <<"}";

	outData = ss.str();
}

CommandCustomHeader* PullMessageResponseHeader::Decode(char* pData,int len)
{
	MQJson::Reader reader;
	MQJson::Value object;
	if (!reader.parse(pData+8, object))
	{
		return NULL;
	}

	MQJson::Value ext = object["extFields"];
    /* modified by yu.guangjie at 2015-11-04, reason: */
	long long suggestWhichBrokerId = str2ll(ext["suggestWhichBrokerId"].asString().c_str());
	long long nextBeginOffset = str2ll(ext["nextBeginOffset"].asString().c_str());
	long long minOffset = str2ll(ext["minOffset"].asString().c_str());
	long long maxOffset = str2ll(ext["maxOffset"].asString().c_str());
	
	PullMessageResponseHeader* h = new PullMessageResponseHeader();

	h->suggestWhichBrokerId = suggestWhichBrokerId;
	h->nextBeginOffset = nextBeginOffset;
	h->minOffset = minOffset;
	h->maxOffset = maxOffset;

	return h;
}

GetConsumerListByGroupRequestHeader::GetConsumerListByGroupRequestHeader()
{

}

GetConsumerListByGroupRequestHeader::~GetConsumerListByGroupRequestHeader()
{

}

void GetConsumerListByGroupRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"consumerGroup\":"<<"\""<<consumerGroup<<"\"}";

	outData = ss.str();
}

CommandCustomHeader* GetConsumerListByGroupRequestHeader::Decode( char* pData,int len )
{
	return NULL;
}


GetConsumerListByGroupResponseHeader::GetConsumerListByGroupResponseHeader()
{

}

GetConsumerListByGroupResponseHeader::~GetConsumerListByGroupResponseHeader()
{

}

void GetConsumerListByGroupResponseHeader::Encode( std::string& outData )
{
	outData="{}";
}

CommandCustomHeader* GetConsumerListByGroupResponseHeader::Decode( char* pData,int len )
{
	return new GetConsumerListByGroupResponseHeader();
}

ConsumerSendMsgBackRequestHeader::ConsumerSendMsgBackRequestHeader()
{

}

ConsumerSendMsgBackRequestHeader::~ConsumerSendMsgBackRequestHeader()
{

}

void ConsumerSendMsgBackRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"offset\":"<<"\""<<offset<<"\","
		<<"\"group\":"<<"\""<<group<<"\","
		<<"\"delayLevel\":"<<"\""<<delayLevel<<"\""
		<<"}";

	outData = ss.str();
}

CommandCustomHeader* ConsumerSendMsgBackRequestHeader::Decode( char* pData,int len )
{
	return new ConsumerSendMsgBackRequestHeader();
}

UpdateConsumerOffsetRequestHeader::UpdateConsumerOffsetRequestHeader()
{

}

UpdateConsumerOffsetRequestHeader::~UpdateConsumerOffsetRequestHeader()
{

}

void UpdateConsumerOffsetRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"consumerGroup\":"<<"\""<<consumerGroup<<"\","
		<<"\"topic\":"<<"\""<<topic<<"\","
		<<"\"queueId\":"<<"\""<<queueId<<"\","
		<<"\"commitOffset\":"<<"\""<<commitOffset<<"\""
		<<"}";

	outData = ss.str();
}

CommandCustomHeader* UpdateConsumerOffsetRequestHeader::Decode( char* pData,int len )
{
	return new UpdateConsumerOffsetRequestHeader();
}


QueryConsumerOffsetRequestHeader::QueryConsumerOffsetRequestHeader()
{

}

QueryConsumerOffsetRequestHeader::~QueryConsumerOffsetRequestHeader()
{

}

void QueryConsumerOffsetRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"consumerGroup\":"<<"\""<<consumerGroup<<"\","
		<<"\"topic\":"<<"\""<<topic<<"\","
		<<"\"queueId\":"<<"\""<<queueId<<"\""
		<<"}";

	outData = ss.str();
}

CommandCustomHeader* QueryConsumerOffsetRequestHeader::Decode( char* pData,int len )
{
	return new QueryConsumerOffsetRequestHeader();
}

QueryConsumerOffsetResponseHeader::QueryConsumerOffsetResponseHeader()
{

}

QueryConsumerOffsetResponseHeader::~QueryConsumerOffsetResponseHeader()
{

}

void QueryConsumerOffsetResponseHeader::Encode( std::string& outData )
{
}

CommandCustomHeader* QueryConsumerOffsetResponseHeader::Decode( char* pData,int len )
{
    MQJson::Reader reader;
	MQJson::Value object;
	if (!reader.parse(pData+8, object))
	{
		return NULL;
	}

	MQJson::Value ext = object["extFields"];
	long long queueOffset = str2ll(ext["offset"].asString().c_str());

	QueryConsumerOffsetResponseHeader* h = new QueryConsumerOffsetResponseHeader();

	h->offset = queueOffset;

	return h;
}


SearchOffsetRequestHeader::SearchOffsetRequestHeader()
{

}

SearchOffsetRequestHeader::~SearchOffsetRequestHeader()
{

}

void SearchOffsetRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"topic\":"<<"\""<<topic<<"\","
	  <<"\"queueId\":"<<"\""<<queueId<<"\","
	  <<"\"timestamp\":"<<"\""<<timestamp<<"\""
	  <<"}";

	outData = ss.str();
}

CommandCustomHeader* SearchOffsetRequestHeader::Decode( char* pData,int len )
{
	return new SearchOffsetRequestHeader();
}

SearchOffsetResponseHeader::SearchOffsetResponseHeader()
{

}

SearchOffsetResponseHeader::~SearchOffsetResponseHeader()
{

}

void SearchOffsetResponseHeader::Encode( std::string& outData )
{
}

CommandCustomHeader* SearchOffsetResponseHeader::Decode( char* pData,int len )
{
    MQJson::Reader reader;
	MQJson::Value object;
	if (!reader.parse(pData+8, object))
	{
		return NULL;
	}

	MQJson::Value ext = object["extFields"];
	long long queueOffset = str2ll(ext["offset"].asString().c_str());

	SearchOffsetResponseHeader* h = new SearchOffsetResponseHeader();

	h->offset = queueOffset;

	return h;
}


GetMinOffsetRequestHeader::GetMinOffsetRequestHeader()
{

}

GetMinOffsetRequestHeader::~GetMinOffsetRequestHeader()
{

}

void GetMinOffsetRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"topic\":"<<"\""<<topic<<"\","
		<<"\"queueId\":"<<"\""<<queueId<<"\""
		<<"}";

	outData = ss.str();
}


CommandCustomHeader* GetMinOffsetRequestHeader::Decode( char* pData,int len )
{
	return new GetMinOffsetRequestHeader();
}


GetMaxOffsetRequestHeader::GetMaxOffsetRequestHeader()
{

}

GetMaxOffsetRequestHeader::~GetMaxOffsetRequestHeader()
{

}

void GetMaxOffsetRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"topic\":"<<"\""<<topic<<"\","
	  <<"\"queueId\":"<<"\""<<queueId<<"\""
	  <<"}";

	outData = ss.str();
}


CommandCustomHeader* GetMaxOffsetRequestHeader::Decode( char* pData,int len )
{
	return new GetMaxOffsetRequestHeader();
}


GetEarliestMsgStoretimeRequestHeader::GetEarliestMsgStoretimeRequestHeader()
{

}

GetEarliestMsgStoretimeRequestHeader::~GetEarliestMsgStoretimeRequestHeader()
{

}

void GetEarliestMsgStoretimeRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"topic\":"<<"\""<<topic<<"\","
	  <<"\"queueId\":"<<"\""<<queueId<<"\""
	  <<"}";

	outData = ss.str();
}

CommandCustomHeader* GetEarliestMsgStoretimeRequestHeader::Decode( char* pData,int len )
{
	return new GetEarliestMsgStoretimeRequestHeader();
}

GetEarliestMsgStoretimeResponseHeader::GetEarliestMsgStoretimeResponseHeader()
{

}

GetEarliestMsgStoretimeResponseHeader::~GetEarliestMsgStoretimeResponseHeader()
{

}

void GetEarliestMsgStoretimeResponseHeader::Encode( std::string& outData )
{
}

CommandCustomHeader* GetEarliestMsgStoretimeResponseHeader::Decode( char* pData,int len )
{
    MQJson::Reader reader;
	MQJson::Value object;
	if (!reader.parse(pData+8, object))
	{
		return NULL;
	}

	MQJson::Value ext = object["extFields"];
	long long qTimestamp = str2ll(ext["timestamp"].asString().c_str());

	GetEarliestMsgStoretimeResponseHeader* h = new GetEarliestMsgStoretimeResponseHeader();

	h->timestamp = qTimestamp;

	return h;
}



NotifyConsumerIdsChangedRequestHeader::NotifyConsumerIdsChangedRequestHeader()
{

}

NotifyConsumerIdsChangedRequestHeader::~NotifyConsumerIdsChangedRequestHeader()
{

}

void NotifyConsumerIdsChangedRequestHeader::Encode( std::string& outData )
{
}

CommandCustomHeader* NotifyConsumerIdsChangedRequestHeader::Decode( char* pData,int len )
{
    MQJson::Reader reader;
	MQJson::Value object;
	if (!reader.parse(pData+8, object))
	{
		return NULL;
	}

	MQJson::Value ext = object["extFields"];

	NotifyConsumerIdsChangedRequestHeader* h = new NotifyConsumerIdsChangedRequestHeader();
	h->consumerGroup = ext["consumerGroup"].asString().c_str();

	return h;
}


UnregisterClientRequestHeader::UnregisterClientRequestHeader()
{

}

UnregisterClientRequestHeader::~UnregisterClientRequestHeader()
{

}

void UnregisterClientRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"clientID\":"<<"\""<<clientID<<"\","
		<<"\"producerGroup\":"<<"\""<<producerGroup<<"\","
		<<"\"consumerGroup\":"<<"\""<<consumerGroup<<"\""
		<<"}";

	outData = ss.str();
}

CommandCustomHeader* UnregisterClientRequestHeader::Decode( char* pData,int len )
{
	return new UnregisterClientRequestHeader();
}


ViewMessageRequestHeader::ViewMessageRequestHeader()
{

}

ViewMessageRequestHeader::~ViewMessageRequestHeader()
{

}

void ViewMessageRequestHeader::Encode( std::string& outData )
{
	std::stringstream ss;

	ss<<"{"<<"\"offset\":"<<"\""<<offset<<"\"}";

	outData = ss.str();
}

CommandCustomHeader* ViewMessageRequestHeader::Decode( char* pData,int len )
{
	return new ViewMessageRequestHeader();
}


void ResponseHaveNoCustomHeader::Encode(std::string & outData)
{
	return;
}

CommandCustomHeader * ResponseHaveNoCustomHeader::Decode(int code, char* pData, int len)
{
	MqLogNotice("code=%d has no extFields", code);
	return NULL;
}
