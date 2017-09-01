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
#if!defined __DEFAULTMQPUSHCONSUMER_H__
#define __DEFAULTMQPUSHCONSUMER_H__

#include <list>
#include <string>

#include "MQPushConsumer.h"
#include "MessageExt.h"
#include "ClientConfig.h"
#include "RocketMQClient.h"

class AllocateMessageQueueStrategy;
class DefaultMQPushConsumerImpl;
class OffsetStore;

/**
* ������Broker Push��Ϣ��Consumer��ʽ����ʵ����Ȼ��Consumer�ڲ���̨��Broker Pull��Ϣ<br>
* ���ó���ѯ��ʽ����Ϣ��ʵʱ��ͬpush��ʽһ�£��Ҳ�����ν������Ϣ����Broker��Consumerѹ������
*
*/
class ROCKETMQCLIENT_API DefaultMQPushConsumer : public ClientConfig ,public MQPushConsumer
{
public:
	DefaultMQPushConsumer();
	DefaultMQPushConsumer(const std::string& consumerGroup);
	~DefaultMQPushConsumer();

	//MQAdmin
	void createTopic(const std::string& key, const std::string& newTopic, int queueNum);
	long long searchOffset(const MessageQueue& mq, long long timestamp);
	long long maxOffset(const MessageQueue& mq);
	long long minOffset(const MessageQueue& mq);
	long long earliestMsgStoreTime(const MessageQueue& mq);
	//����ָ�룬��Ҫ��ҵ����������ָ��
	MessageExt* viewMessage(const std::string& msgId);
	QueryResult queryMessage(const std::string& topic,
							 const std::string&  key,
							 int maxNum,
							 long long begin,
							 long long end);

	// MQadmin end

	AllocateMessageQueueStrategy* getAllocateMessageQueueStrategy();

	//Note by lin.qiongshan, Consumer ��������ʱ����ɾ���漰��ȥ�� AllocateMessageQueueStrategy ָ�룬����ⲿ��Ҫά���ö������������
	void setAllocateMessageQueueStrategy(AllocateMessageQueueStrategy* pAllocateMessageQueueStrategy);
	
	int getConsumeConcurrentlyMaxSpan();
	void setConsumeConcurrentlyMaxSpan(int consumeConcurrentlyMaxSpan);

	ConsumeFromWhere getConsumeFromWhere();
	void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere);

	int getConsumeMessageBatchMaxSize();
	void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize);

	std::string getConsumerGroup();
	void setConsumerGroup(const std::string& consumerGroup) ;

	int getConsumeThreadMax() ;
	void setConsumeThreadMax(int consumeThreadMax);

	int getConsumeThreadMin();
	void setConsumeThreadMin(int consumeThreadMin);

	MessageListener* getMessageListener();
	//Note by lin.qiongshan, Consumer ������������ɾ�����ý�ȥ�� MessageListener ָ�룬�����Ҫ�ⲿά����ָ��������������
	void setMessageListener(MessageListener* pMessageListener);

	MessageModel getMessageModel();
	void setMessageModel(MessageModel messageModel) ;

	int getPullBatchSize() ;
	void setPullBatchSize(int pullBatchSize);

	long getPullInterval();
	void setPullInterval(long pullInterval);

	int getPullThresholdForQueue();
	void setPullThresholdForQueue(int pullThresholdForQueue);

	std::map<std::string, std::string>& getSubscription();
	void setSubscription(const std::map<std::string, std::string>& subscription);
	
	/** modify by liang.haibo 2016-10-09 
	** consumeTimestamp getter and setter
	**/
    std::string getConsumeTimestamp();
	void setConsumeTimestamp(const std::string& consumeTimestamp);

	//MQConsumer
	void sendMessageBack(MessageExt& msg, int delayLevel);
	std::set<MessageQueue>* fetchSubscribeMessageQueues(const std::string& topic);

	void start();
	//Note by lin.qiongshan, shutdown �������� start��start ֻ���������״�ʵ��������á�
	//	Consumer �������������Զ� shutdown���� shutdown ����һЩ��Ҫ�����������ֹһ���ͻ���ʵ��ʱ��һ��Ҫ���� shutdown
	//	shutdown ��ͬʱ���Թر�������� MQClientFactory ʵ������ʵ������ clientId ��ͬ�Ŀͻ���ʵ��
	//	��Ϊ MQClientFactory �� shutdown ʱ��ֻ�������еĿͻ���ʵ�������˳����Ż������رգ�����ĳ���ͻ���ʵ���� shutdown������Ӱ������ʵ������ ConsumerGroup ��ͬ��clientId ��ͬ����һ�� client��
	void shutdown();
	//MQConsumer end
	
	//MQPushConsumer
	void registerMessageListener(MessageListener* pMessageListener);

	void subscribe(const std::string& topic, const std::string& subExpression);
	void unsubscribe(const std::string& topic);

	void updateCorePoolSize(int corePoolSize);

	void suspend() ;
	void resume();
	//MQPushConsumer end

	OffsetStore* getOffsetStore();
	void setOffsetStore(OffsetStore* offsetStore);

	DefaultMQPushConsumerImpl* getDefaultMQPushConsumerImpl();

	//add by lin.qiongshan, 2016-9-2��TCP ������ʱ���û�
	void setTcpTimeoutMilliseconds(int milliseconds);
	int getTcpTimeoutMilliseconds();
protected:
	DefaultMQPushConsumerImpl* m_pDefaultMQPushConsumerImpl;

private:
	/**
	* ��ͬ�������Consumer��Ϊͬһ��Group��Ӧ�ñ������ã�����֤����Ψһ
	*/
	std::string m_consumerGroup;

	/**
	* ��Ⱥ����/�㲥����
	*/
	MessageModel m_messageModel;

	/**
	* Consumer����ʱ�������￪ʼ����
	*/
	ConsumeFromWhere m_consumeFromWhere;

	/**
	* ���з����㷨��Ӧ�ÿ���д
	*/
	AllocateMessageQueueStrategy* m_pAllocateMessageQueueStrategy ;

	/**
	* ���Ĺ�ϵ
	*/
	std::map<std::string /* topic */, std::string /* sub expression */> m_subscription ;

	/**
	* ��Ϣ������
	*/
	MessageListener* m_pMessageListener;

	/**
	* Offset�洢��ϵͳ����ݿͻ��������Զ�������Ӧ��ʵ�֣����Ӧ�������ˣ�����Ӧ�����õ�Ϊ��
	*/
	OffsetStore* m_pOffsetStore;

	/**
	* ������Ϣ�̣߳���С��Ŀ
	*/
	int m_consumeThreadMin;

	/**
	* ������Ϣ�̣߳������Ŀ
	*/
	int m_consumeThreadMax;

	/**
	* ͬһ���в������ѵ�����ȣ�˳�����ѷ�ʽ����£��˲�����Ч
	*/
	int m_consumeConcurrentlyMaxSpan;

	/**
	* ���ض�����Ϣ�������˷�ֵ����ʼ����
	*/
	int m_pullThresholdForQueue;

	/**
	* ����Ϣ��������Ϊ�˽�����ȡ�ٶȣ��������ô���0��ֵ
	*/
	long m_pullInterval;

	/**
	* ����һ����Ϣ�������
	*/
	int m_consumeMessageBatchMaxSize;

	/**
	* ����Ϣ��һ����������
	*/
	int m_pullBatchSize;
	
    /**
     * Consumer��һ������ʱ������������ѣ�Ĭ�ϻ��ݵ��ĸ�ʱ��㣬���ݸ�ʽ���£�ʱ�侫���룺<br>
     * 20131223171201<br>
     * ��ʾ2013��12��23��17��12��01��<br>
     * Ĭ�ϻ��ݵ��������ʱ��İ�Сʱǰ
     */
	std::string m_consumeTimestamp;
};

#endif
