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
#if!defined __REBALANCEIMPL_H__
#define __REBALANCEIMPL_H__

#include <map>
#include <string>
#include <set>
#include <list>
#include <vector>
#include "ConsumeType.h"
#include "MessageQueue.h"
#include "ProcessQueue.h"
#include "PullRequest.h"
#include "SubscriptionData.h"

class AllocateMessageQueueStrategy;
class MQClientFactory;

/**
* Rebalance�ľ���ʵ��
*
*/
class RebalanceImpl
{
public:
	RebalanceImpl(const std::string& consumerGroup, 
		MessageModel messageModel,
		AllocateMessageQueueStrategy* pAllocateMessageQueueStrategy, 
		MQClientFactory* pMQClientFactory);
	virtual ~RebalanceImpl();

	virtual void messageQueueChanged(const std::string& topic, 
		std::set<MessageQueue>& mqAll, 
		std::set<MessageQueue>& mqDivided)=0;
	virtual void removeUnnecessaryMessageQueue(MessageQueue& mq, ProcessQueue& pq)=0;
	virtual void dispatchPullRequest(std::list<PullRequest*>& pullRequestList)=0;
	virtual long long computePullFromWhere(MessageQueue& mq)=0;

	bool lock(MessageQueue& mq);
	void lockAll();

	void unlock(MessageQueue& mq, bool oneway);
	void unlockAll(bool oneway);

	void doRebalance();

	std::map<std::string, SubscriptionData> getSubscriptionInner();
    void subscribe(std::string topic, SubscriptionData& subData);
    void unsubscribe(std::string topic);
    bool hasSubscribe(std::string topic, SubscriptionData *psubData=NULL);
    
	std::map<MessageQueue, ProcessQueue*>& getProcessQueueTable();
	std::map<std::string, std::set<MessageQueue> >& getTopicSubscribeInfoTable();

	std::string& getConsumerGroup();
	void setConsumerGroup(const std::string& consumerGroup);

	MessageModel getMessageModel();
	void setMessageModel(MessageModel messageModel);

	AllocateMessageQueueStrategy* getAllocateMessageQueueStrategy();
	void setAllocateMessageQueueStrategy(AllocateMessageQueueStrategy* pAllocateMessageQueueStrategy);
	
	MQClientFactory* getmQClientFactory();
	void setmQClientFactory(MQClientFactory* pMQClientFactory);
    void removeProcessQueue(MessageQueue& mq);

private:
	std::map<std::string, std::set<MessageQueue> > buildProcessQueueTableByBrokerName();
	void rebalanceByTopic(const std::string& topic);
	bool updateProcessQueueTableInRebalance(const std::string& topic, std::set<MessageQueue>& mqSet);
	void truncateMessageQueueNotMyTopic();

protected:
	// ����õĶ��У���Ϣ�洢Ҳ������
	std::map<MessageQueue, ProcessQueue*> m_processQueueTable;

	//mjx modify add
	//broker name+ master ip
	std::map<std::string, std::string> m_brokerMaster;
	//down broker name
	std::vector<std::string> m_downBrokerName;
	
	// ���Զ��ĵ����ж��У���ʱ��Name Server�������°汾��
	std::map<std::string, std::set<MessageQueue> > m_topicSubscribeInfoTable;
	// ���Ĺ�ϵ���û����õ�ԭʼ����
	std::map<std::string /* topic */, SubscriptionData> m_subscriptionInner;
    kpr::Mutex m_subsMutex; /* added by yu.guangjie at 2015-08-20 */
    
	std::string m_consumerGroup;
	MessageModel m_messageModel;
	AllocateMessageQueueStrategy* m_pAllocateMessageQueueStrategy;
	MQClientFactory* m_pMQClientFactory;

    
};

#endif
