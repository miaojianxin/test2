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

#if!defined __COMMANDCUSTOMHEADER_H__
#define __COMMANDCUSTOMHEADER_H__

#include <string>

/**
* RemotingCommand中自定义字段对象
*
*/
class CommandCustomHeader
{
public :
	virtual ~CommandCustomHeader() {}
	virtual void Encode(std::string& outData)=0;

	static CommandCustomHeader* Decode(int code,char* pData,int len,bool isResponseType);
};

/**
* 没有自定义字段
* @author lin.qiongshan
* @date 2016年8月18日09:57:52
* @desc 客户端在收到自己请求对应的响应后，都会调用 CommandCustomHeader::Decode 解析自定义字段（extFields）
*	但是很多类型的请求对应的响应消息中，没有自定义字段（extFields 字段值为空），如果这些类型的请求没有定义自己的 CommandCustomHeader，客户端会显示警告
*	因此，增加一个表示没有自定义字段的 CommandCustomHeader 类型，来处理所有没有自定义字段的请求的响应
*	
*	具体的，哪些类型的请求的响应没有自定义字段的，参看 CommandCustomHeader::Decode 方法的实现
*/
class ResponseHaveNoCustomHeader : public CommandCustomHeader
{
public:
	/** 不做任何处理 */
	virtual void Encode(std::string& outData);

	/** 
	* 以 NOTICE 级别显示 code ，并说明没有自定义字段
	* @return NULL ，固定返回 NULL
	*/
	static CommandCustomHeader* Decode(int code, char* pData, int len);
};
class GetRouteInfoRequestHeader : public CommandCustomHeader
{
public:
	GetRouteInfoRequestHeader();
	~GetRouteInfoRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);
public:
	std::string topic;
};



class SubscriptionGroupConfigHeader : public CommandCustomHeader
{
public:
	SubscriptionGroupConfigHeader();
	~SubscriptionGroupConfigHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

public:
	std::string groupName;// 订阅组名
	bool consumeEnable;// 消费功能是否开启
	bool consumeFromMinEnable;// 是否允许从队列最小位置开始消费，线上默认会设置为false
	bool consumeBroadcastEnable;// 是否允许广播方式消费
	int retryQueueNums;// 消费失败的消息放到一个重试队列，每个订阅组配置几个重试队列
	int retryMaxTimes; // 重试消费最大次数，超过则投递到死信队列，不再投递，并报警
	long brokerId;// 从哪个Broker开始消费
	long whichBrokerWhenConsumeSlowly;// 发现消息堆积后，将Consumer的消费请求重定向到另外一台Slave机器
};

class CreateTopicRequestHeader : public CommandCustomHeader
{
public:
	CreateTopicRequestHeader();
	~CreateTopicRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

public:
	std::string topic;
	std::string defaultTopic;
	int readQueueNums;
	int writeQueueNums;
	int perm;
	std::string topicFilterType;
};

class SendMessageRequestHeader: public CommandCustomHeader
{
public:
	SendMessageRequestHeader();
	~SendMessageRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

public:
	std::string producerGroup;
	std::string topic;
	std::string defaultTopic;
	int defaultTopicQueueNums;
	int queueId;
	int sysFlag;
	long long bornTimestamp;
	int flag;
	std::string properties;
	int reconsumeTimes;
};

class SendMessageResponseHeader: public CommandCustomHeader
{
public:
	SendMessageResponseHeader();
	~SendMessageResponseHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

public:
	std::string msgId;
	int queueId;
	long long queueOffset;
};

class PullMessageRequestHeader: public CommandCustomHeader
{
public:
	PullMessageRequestHeader();
	~PullMessageRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

public:
	std::string consumerGroup;
	std::string topic;
	int queueId;
	long long queueOffset;
	int maxMsgNums;
	int sysFlag;
	long  long commitOffset;
	long long suspendTimeoutMillis;
	std::string subscription;
	long long subVersion;
};

class PullMessageResponseHeader: public CommandCustomHeader
{
public:
	PullMessageResponseHeader();
	~PullMessageResponseHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

public:
	long long suggestWhichBrokerId;
	long long nextBeginOffset;
	long long minOffset;
	long long maxOffset;
};

class GetConsumerListByGroupRequestHeader : public CommandCustomHeader
{
public:
	GetConsumerListByGroupRequestHeader();
	~GetConsumerListByGroupRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

public:
	std::string consumerGroup;
};

class GetConsumerListByGroupResponseHeader : public CommandCustomHeader
{
public:
	GetConsumerListByGroupResponseHeader();
	~GetConsumerListByGroupResponseHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);
};

class ConsumerSendMsgBackRequestHeader : public CommandCustomHeader
{
public:
	ConsumerSendMsgBackRequestHeader();
	~ConsumerSendMsgBackRequestHeader();
	
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

public:
	long long offset;
	std::string group;
	int delayLevel;
};

/* modified by yu.guangjie at 2015-08-16, reason: add begin */

class UpdateConsumerOffsetRequestHeader : public CommandCustomHeader
{
public:
	UpdateConsumerOffsetRequestHeader();
	~UpdateConsumerOffsetRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    std::string getConsumerGroup(void) 
    {
        return consumerGroup;
    }  
    void setConsumerGroup(std::string Group) 
    {
        consumerGroup = Group;
    }  
    std::string getTopic(void) 
    {
        return topic;
    }  
    void setTopic(std::string tic) 
    {
        topic = tic;
    }
    void setQueueId(int qId) 
    {
        queueId = qId;
    }
    void setCommitOffset(long long cOffset) 
    {
        commitOffset = cOffset;
    }
    
public:
	std::string consumerGroup;
    std::string topic;
    int queueId;
	long  long commitOffset;
};

class QueryConsumerOffsetRequestHeader : public CommandCustomHeader
{
public:
	QueryConsumerOffsetRequestHeader();
	~QueryConsumerOffsetRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    std::string getConsumerGroup(void) 
    {
        return consumerGroup;
    }  
    void setConsumerGroup(std::string Group) 
    {
        consumerGroup = Group;
    }    
    std::string getTopic(void) 
    {
        return topic;
    } 
    void setTopic(std::string tic) 
    {
        topic = tic;
    }
    void setQueueId(int qId) 
    {
        queueId = qId;
    }
    
public:
	std::string consumerGroup;
    std::string topic;
    int queueId;
};

class QueryConsumerOffsetResponseHeader : public CommandCustomHeader
{
public:
	QueryConsumerOffsetResponseHeader();
	~QueryConsumerOffsetResponseHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    long long getOffset(void) 
    {
        return offset;
    }  
    void setOffset(long long cOffset) 
    {
        offset = cOffset;
    }
    
public:
	long long offset;
};

class SearchOffsetRequestHeader : public CommandCustomHeader
{
public:
	SearchOffsetRequestHeader();
	~SearchOffsetRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    void setTopic(std::string tic) 
    {
        topic = tic;
    }
    void setQueueId(int qId) 
    {
        queueId = qId;
    }
	void setTimestamp(long long tsp)
	{
		timestamp = tsp;
	}
    
public:
    std::string topic;
    int queueId;
	long long timestamp;
};

class SearchOffsetResponseHeader : public CommandCustomHeader
{
public:
	SearchOffsetResponseHeader();
	~SearchOffsetResponseHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    long long getOffset(void) 
    {
        return offset;
    }  
    void setOffset(long long cOffset) 
    {
        offset = cOffset;
    }
    
public:
	long long offset;
};


class GetMinOffsetRequestHeader : public CommandCustomHeader
{
public:
	GetMinOffsetRequestHeader();
	~GetMinOffsetRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    std::string getTopic(void) 
    {
        return topic;
    } 
    void setTopic(std::string tic) 
    {
        topic = tic;
    }
    int getQueueId() 
    {
        return queueId;
    }
    void setQueueId(int qId) 
    {
        queueId = qId;
    }
    
public:
    std::string topic;
    int queueId;
};


typedef QueryConsumerOffsetResponseHeader GetMinOffsetResponseHeader;


class GetMaxOffsetRequestHeader : public CommandCustomHeader
{
public:
	GetMaxOffsetRequestHeader();
	~GetMaxOffsetRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    std::string getTopic(void) 
    {
        return topic;
    } 
    void setTopic(std::string tic) 
    {
        topic = tic;
    }
    int getQueueId() 
    {
        return queueId;
    }
    void setQueueId(int qId) 
    {
        queueId = qId;
    }
    
public:
    std::string topic;
    int queueId;
};

typedef QueryConsumerOffsetResponseHeader GetMaxOffsetResponseHeader;



class GetEarliestMsgStoretimeRequestHeader : public CommandCustomHeader
{
public:
	GetEarliestMsgStoretimeRequestHeader();
	~GetEarliestMsgStoretimeRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    void setTopic(std::string tic) 
    {
        topic = tic;
    }
    void setQueueId(int qId) 
    {
        queueId = qId;
    }
    
public:
    std::string topic;
    int queueId;
};

class GetEarliestMsgStoretimeResponseHeader : public CommandCustomHeader
{
public:
	GetEarliestMsgStoretimeResponseHeader();
	~GetEarliestMsgStoretimeResponseHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    long long getTimestamp(void) 
    {
        return timestamp;
    }  
    void setTimestamp(long long cTimestamp)
    {
        timestamp = cTimestamp;
    }
    
public:
	long long timestamp;
};


class NotifyConsumerIdsChangedRequestHeader: public CommandCustomHeader
{
public:
	NotifyConsumerIdsChangedRequestHeader();
	~NotifyConsumerIdsChangedRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    std::string getConsumerGroup(void) 
    {
        return consumerGroup;
    }  
    void setConsumerGroup(std::string Group) 
    {
        consumerGroup = Group;
    }    

    
public:
	std::string consumerGroup;
};

class UnregisterClientRequestHeader: public CommandCustomHeader
{
public:
	UnregisterClientRequestHeader();
	~UnregisterClientRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);

    std::string getClientID(void) 
    {
        return clientID;
    }


    void setClientID(std::string cliId) 
    {
        clientID = cliId;
    }

    std::string getProducerGroup(void) 
    {
        return producerGroup;
    }


    void setProducerGroup(std::string proGroup) 
    {
        producerGroup = proGroup;
    }
    
    std::string getConsumerGroup(void) 
    {
        return consumerGroup;
    }  
    void setConsumerGroup(std::string Group) 
    {
        consumerGroup = Group;
    }    

    
public:
    std::string clientID;
    std::string producerGroup;
	std::string consumerGroup;
};


class ViewMessageRequestHeader: public CommandCustomHeader
{
public:
	ViewMessageRequestHeader();
	~ViewMessageRequestHeader();
	virtual void Encode(std::string& outData);
	static CommandCustomHeader* Decode(char* pData,int len);
	
    long long getOffset(void) 
    {
        return offset;
    }  
    void setOffset(long long cOffset) 
    {
        offset = cOffset;
    }
    
public:
	long long offset;
};
#endif
