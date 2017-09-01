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
#include <set>
#include <map>

#include "MQConsumerInner.h"
#include "MessageExt.h"
#include "QueryResult.h"
#include "ServiceState.h"
#include "PullCallback.h"
#include "PullRequest.h"
#include "PullResult.h"
#include "ConsumeMessageHook.h"
#include "TimerThread.h"

class DefaultMQPushConsumer;
class ConsumeMessageHook;

class OffsetStore;
class RebalanceImpl;
class ConsumerStatManager;
class ConsumeMessageService;
class MessageListener;
class PullRequest;
class MQClientFactory;
class PullAPIWrapper;
class PullMessageService;

/**
* Push��ʽ��Consumerʵ��
*
*/
class DefaultMQPushConsumerImpl : public  MQConsumerInner
{
public:
	DefaultMQPushConsumerImpl(DefaultMQPushConsumer* pDefaultMQPushConsumer);
    virtual ~DefaultMQPushConsumerImpl();
	
	void start();
	void suspend();
	void resume();
	void shutdown();
	bool isPause();
	void setPause(bool pause);

	bool hasHook();
	void registerHook(ConsumeMessageHook* pHook);
	void executeHookBefore(ConsumeMessageContext& context);
	void executeHookAfter(ConsumeMessageContext& context);

	void createTopic(const std::string& key, const std::string& newTopic, int queueNum);
	std::set<MessageQueue>* fetchSubscribeMessageQueues(const std::string& topic);

	long long earliestMsgStoreTime(const MessageQueue& mq);
	long long maxOffset(const MessageQueue& mq);
	long long minOffset(const MessageQueue& mq);
	OffsetStore* getOffsetStore() ;
	void setOffsetStore(OffsetStore* pOffsetStore);

	void setTcpTimeoutMilliseconds(int milliseconds);
	int getTcpTimeoutMilliseconds();

	//MQConsumerInner
	std::string groupName() ;
	MessageModel messageModel() ;
	ConsumeType consumeType();
	ConsumeFromWhere consumeFromWhere();
	std::set<SubscriptionData> subscriptions();
	void doRebalance() ;
	void persistConsumerOffset() ;
	void updateTopicSubscribeInfo(const std::string& topic, const std::set<MessageQueue>& info);
	//std::map<std::string, SubscriptionData>& getSubscriptionInner() ;
	bool isSubscribeTopicNeedUpdate(const std::string& topic);
	//����ָ�룬��Ҫ��ҵ����������ָ��
	MessageExt* viewMessage(const std::string& msgId);
	QueryResult queryMessage(const std::string& topic,
							 const std::string&  key,
							 int maxNum,
							 long long begin,
							 long long end);

	void registerMessageListener(MessageListener* pMessageListener);
	long long searchOffset(const MessageQueue& mq, long long timestamp);
	void sendMessageBack(MessageExt& msg, int delayLevel);

	void subscribe(const std::string& topic, const std::string& subExpression);
	void unsubscribe(const std::string& topic);

	void updateConsumeOffset(MessageQueue& mq, long long offset);
	void updateCorePoolSize(int corePoolSize);
	bool isConsumeOrderly();
	void setConsumeOrderly(bool consumeOrderly);
	
	RebalanceImpl* getRebalanceImpl() ;
    PullAPIWrapper* getPullAPIWrapper();
	MessageListener* getMessageListenerInner();
	DefaultMQPushConsumer* getDefaultMQPushConsumer() ;
	ConsumerStatManager* getConsumerStatManager();
    ConsumeMessageService* getConsumeMessageService()
    {
        return m_pConsumeMessageService;
    }

    /**
	* ����ִ�����PullRequest
	*/
	void executePullRequestImmediately(PullRequest& pullRequest);

	/**
	* �Ժ���ִ�����PullRequest
	*/
	void executePullRequestLater(PullRequest& pullRequest, long timeDelay);
    /**
	* ͨ��Tag����ʱ�������offset��׼ȷ���������Ҫ����
	*/
	void correctTagsOffset(PullRequest& pullRequest) ;

    void executeTaskLater(PullRequest& pullRequest, long timeDelay);
    
private:	

	void pullMessage(PullRequest* pPullRequest);    

	void makeSureStateOK();	
	void checkConfig();
	void copySubscription() ;
	void updateTopicSubscribeInfoWhenSubscriptionChanged();	

private:
	static long long s_PullTimeDelayMillsWhenException;// ����Ϣ�쳣ʱ���ӳ�һ��ʱ������
	static long long s_PullTimeDelayMillsWhenFlowControl;
	static long long s_PullTimeDelayMillsWhenSuspend;

	static long long s_BrokerSuspendMaxTimeMillis;// ����ѯģʽ��Consumer������Broker�����ʱ��
	static long long s_ConsumerTimeoutMillisWhenSuspend;// ����ѯģʽ��Consumer��ʱʱ�䣨����Ҫ����brokerSuspendMaxTimeMillis��
	
	//add by lin.qiongshan��2016-9-2��TCP ������ʱʱ�����û�
	int m_tcpTimeoutMilliSeconds;

	long long flowControlTimes1;
	long long flowControlTimes2;
	ServiceState m_serviceState;
	volatile bool m_pause;// �Ƿ���ͣ������Ϣ suspend/resume
	bool m_consumeOrderly;// �Ƿ�˳��������Ϣ
	DefaultMQPushConsumer* m_pDefaultMQPushConsumer;
	MQClientFactory* m_pMQClientFactory;
	PullAPIWrapper* m_pPullAPIWrapper;
	MessageListener* m_pMessageListenerInner;// ������Ϣ������
	OffsetStore* m_pOffsetStore;// ���ѽ��ȴ洢
	RebalanceImpl* m_pRebalanceImpl;// Rebalanceʵ��
	ConsumerStatManager* m_pConsumerStatManager;
	ConsumeMessageService* m_pConsumeMessageService;// ������Ϣ����

	std::list<ConsumeMessageHook*> m_hookList;//����ÿ����Ϣ��ص�

    kpr::TimerThread_var m_pullLaterService;  

	friend class PullMessageService;
	friend class RebalancePushImpl;
    friend class DefaultMQPushConsumerImplCallback;
};


class ExecutePullTaskLater : public kpr::TimerHandler
{
public:
	ExecutePullTaskLater(PullRequest* pPullRequest,
		DefaultMQPushConsumerImpl* pDefaultMQPushConsumerImpl);

    void OnTimeOut(unsigned int timerID);
private:
	DefaultMQPushConsumerImpl* m_pDefaultMQPushConsumerImpl;
    PullRequest* m_pPullRequest;

};

class DefaultMQPushConsumerImplCallback : public PullCallback
{
public:
	DefaultMQPushConsumerImplCallback(SubscriptionData& subscriptionData,
		DefaultMQPushConsumerImpl* pDefaultMQPushConsumerImpl,
		PullRequest* pPullRequest, long long beginTimestamp);

	void onSuccess(PullResult& pullResult);
	void onException(MQException& e);


private:
	SubscriptionData m_subscriptionData;
	DefaultMQPushConsumerImpl* m_pDefaultMQPushConsumerImpl;
    PullRequest* m_pPullRequest;
    long long m_beginTimestamp;
};


#endif
