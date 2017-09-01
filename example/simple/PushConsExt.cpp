// PullConsumer.cpp : 定义控制台应用程序的入口点。
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <map>
#include <set>

#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#endif

#include <DefaultMQPushConsumer.h>
#include <MessageListener.h>
#include <Message.h>
#include <MessageExt.h>
#include <MessageQueue.h>
#include <PullResult.h>
#include <MQClientException.h>

long long str2ll(const char *str);

class MsgListener : public MessageListenerConcurrently
{
public:
	MsgListener()
	{
		consumeTimes = 0;
	}

	~MsgListener()
	{

	}

	ConsumeConcurrentlyStatus consumeMessage(std::list<MessageExt*>& msgs,
											ConsumeConcurrentlyContext& context)
	{
	    printf("consumeMessage() start...\n");
		MessageExt* msg = msgs.front();
		long long offset = msg->getQueueOffset();
		std::string maxOffset = msg->getProperty(Message::PROPERTY_MAX_OFFSET);

		long long diff = str2ll(maxOffset.c_str()) - offset;

		if (diff > 100000)
		{
			// 消息堆积情况的特殊处理
			// return ConsumeConcurrentlyStatus.CONSUME_SUCCESS;
		}

		std::list<MessageExt*>::iterator it = msgs.begin();

        int i = 0;
		for (;it != msgs.end();it++)
		{
			MessageExt* me = *it;
			std::string str;
			str.assign(me->getBody(),me->getBodyLen());
            printf("Topic[%s],Msg[%d:%d]: %s\n", 
                context.messageQueue.getTopic().c_str(), 
                consumeTimes, ++i, str.c_str());
		}

		consumeTimes++;
#if 0
		if ((consumeTimes % 2) == 0)
		{
			return RECONSUME_LATER;
		}
		else if ((consumeTimes % 3) == 0)
		{
			context.delayLevelWhenNextConsume = 5;
			return RECONSUME_LATER;
		}
#endif

		return CONSUME_SUCCESS;
	}

	int consumeTimes;
};

/* modified by yu.guangjie at 2015-08-26, reason: */

class MsgOrderListener : public MessageListenerOrderly
{
public:
	MsgOrderListener()
	{
		consumeTimes = 0;
        m_CurrMsg = NULL;
        //初始化互斥量
        pthread_mutex_init(&m_Mutex, NULL);
        //初始化条件变量
        pthread_cond_init(&m_ProdCond, NULL);
        pthread_cond_init(&m_ConsCond, NULL);
	}

	~MsgOrderListener()
	{

	}

    ConsumeOrderlyStatus consumeMessage(std::list<MessageExt*>& msgs,
												ConsumeOrderlyContext& context)
	{
#ifdef WIN32
		DWORD threadId = ::GetCurrentThreadId();
#else
		pthread_t threadId = pthread_self();
#endif
	    printf("[TID:%ld]order consumeMessage() Topic[%s],Broker[%s--%d]\n", 
	        threadId, context.messageQueue.getTopic().c_str(), 
            context.messageQueue.getBrokerName().c_str(),
            context.messageQueue.getQueueId());

		std::list<MessageExt*>::iterator it = msgs.begin();

        int i = 0;
		for (;;it++)
		{
            //锁定mutex
            pthread_mutex_lock(&m_Mutex);
            while(m_CurrMsg != NULL)
            {
                pthread_cond_wait(&m_ProdCond, &m_Mutex);
            }
            if(it != msgs.end())
            {
                m_CurrMsg = *it;  // 生产
            }
            
            pthread_cond_signal(&m_ConsCond); //发送条件变量信号
            pthread_mutex_unlock(&m_Mutex);  //解锁mutex
            if(it == msgs.end())
            {
                break;
            }
		}

		consumeTimes++;


		return SUCCESS;
	}

    MessageExt* getMessage()
	{
	    MessageExt* pmsg = NULL;
	    
        pthread_mutex_lock(&m_Mutex);    //锁定mutex
        while (m_CurrMsg == NULL)
        {
            pthread_cond_wait(&m_ConsCond, &m_Mutex);    //等待条件变量
        }
        sleep(1);
        //消费 
        pmsg = new MessageExt(m_CurrMsg->getQueueId(),
            m_CurrMsg->getBornTimestamp(),
            m_CurrMsg->getBornHost(),
            m_CurrMsg->getStoreTimestamp(),
            m_CurrMsg->getStoreHost(),
            m_CurrMsg->getMsgId());
        pmsg->setTopic(m_CurrMsg->getTopic());
        pmsg->setTags(m_CurrMsg->getTags());
        pmsg->setKeys(m_CurrMsg->getKeys());
        pmsg->setDelayTimeLevel(m_CurrMsg->getDelayTimeLevel());
        pmsg->setWaitStoreMsgOK(m_CurrMsg->isWaitStoreMsgOK());
        pmsg->setFlag(m_CurrMsg->getFlag());
        pmsg->setBody(m_CurrMsg->getBody(), m_CurrMsg->getBodyLen());
        pmsg->setProperties(m_CurrMsg->getProperties());
        
        m_CurrMsg = NULL;  
              
        pthread_cond_signal(&m_ProdCond); //发送条件变量信号
        pthread_mutex_unlock(&m_Mutex);  //解锁mutex
        
        return pmsg;
	}


	int consumeTimes;
    MessageExt*  m_CurrMsg;
    pthread_mutex_t m_Mutex;
    pthread_cond_t  m_ProdCond;
    pthread_cond_t  m_ConsCond;
};

//消费者
void *consumerProc(void *args)
{
#ifdef WIN32
		DWORD threadId = ::GetCurrentThreadId();
#else
		pthread_t threadId = pthread_self();
#endif

    MsgOrderListener *pLis = (MsgOrderListener *)args;

    while (1)
    {
        MessageExt *pmsg = pLis->getMessage();
        if(pmsg != NULL)
        {
            std::string str;
            str.assign(pmsg->getBody(),pmsg->getBodyLen());
            printf("[TID:%ld]consumerProc(): %s\n", threadId, str.c_str());
        }        

        delete pmsg;
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	if (argc<2)
	{
		printf("Usage:%s ip:port [order]\n",argv[0]);
		return 0;
	}

    bool bOrderConsumer = false;
    if(argc > 2)
    {
        if(strcasecmp(argv[2], "order") == 0)
        {
            bOrderConsumer = true;
        }
    }

	DefaultMQPushConsumer consumer("MyConsumerGroup");
	consumer.setNamesrvAddr(argv[1]);

    pid_t pid = getpid();
    char szPid[20];
    sprintf(szPid, "PID[%lu]", pid);
    consumer.setInstanceName(szPid);
    consumer.setConsumeMessageBatchMaxSize(3);

	/**
	* 订阅指定topic下所有消息
	*/
	// consumer.subscribe("TopicTest", "*");

	/**
	* 订阅指定topic下tags分别等于TagA或TagC或TagD
	*/
	//consumer.subscribe("TopicTest", "TagA || TagC || TagD");
	consumer.subscribe("CCR", "TagA || TagC || TagD");


    MessageListener *listener = NULL;
    if(bOrderConsumer)
    {
        listener = new MsgOrderListener();
    }
    else
    {
        listener = new MsgListener();
    }

	consumer.registerMessageListener(listener);

	consumer.start();

    pthread_t thread[2];
    pthread_create(&thread[0], NULL, consumerProc, listener);
    pthread_create(&thread[1], NULL, consumerProc, listener);


    while(true)
    {
        sleep(1);
    }
	
	delete listener;

	return 0;
}
