// PullConsumer.cpp : 定义控制台应用程序的入口点。
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/signal.h>
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

		//封装alimq，push消费，该处没意义的，maxoffset
		std::string maxOffset = msg->getProperty(Message::PROPERTY_MAX_OFFSET);

		long long diff = atoi(maxOffset.c_str()) - offset;

		//if (diff > 100000)
		//{
			// 消息堆积情况的特殊处理
			// return ConsumeConcurrentlyStatus.CONSUME_SUCCESS;
		//}

		//封装alimq，push消费，context无意义
		//MessageExt里面除queueid外，其他都有
	
		std::list<MessageExt*>::iterator it = msgs.begin();

        int i = 0;
		for (;it != msgs.end();it++)
		{
			MessageExt* me = *it;
			std::string str;
			str.assign(me->getBody(),me->getBodyLen());
            printf("msgid[%s],queueid[%d],queueoffset[%d], topic[%s],tags[%s],keys[%s],Msg[%d:%d]: %s\n", me->getMsgId().c_str(), me->getQueueId(),me->getQueueOffset(),me->getTopic().c_str(),me->getTags().c_str(),me->getKeys().c_str(),consumeTimes, ++i,str.c_str());
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
		for (;it != msgs.end();it++)
		{
			MessageExt* me = *it;
			std::string str;
			str.assign(me->getBody(),me->getBodyLen());
            printf("[TID:%ld] Msg[%d:%d]: %s\n", 
                threadId, consumeTimes, ++i, str.c_str());
		}

		consumeTimes++;

#if 0
		if ((consumeTimes % 3) == 0)
		{
			return SUSPEND_CURRENT_QUEUE_A_MOMENT;
		}
#endif


		return SUCCESS;
	}

	int consumeTimes;
	
};


bool g_bStop = false;

static void sig_handler(const int sig) {
    printf("Signal handled: %s.\n", strsignal(sig));
    g_bStop = true;
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

    signal(SIGINT, sig_handler);

	DefaultMQPushConsumer consumer("CID_TEST1");
	//
	consumer.setNamesrvAddr(argv[1]);
    consumer.setAccessKey("539f7db388b74879863f9335bbe4e3f6");
    consumer.setSecretKey("wjDVEg6Qs2ySkm1g/AJPXz/RZtc=");

    pid_t pid = getpid();
    char szPid[20];
    sprintf(szPid, "PID[%lu]_A", pid);

	
    //consumer.setInstanceName(szPid);
    //consumer.setConsumeMessageBatchMaxSize(3);




	/**
	* 订阅指定topic下所有消息
	*/
	// consumer.subscribe("TopicTest", "*");

	/**
	* 订阅指定topic下tags分别等于TagA或TagC或TagD
	*/
	//consumer.subscribe("TopicTest", "TagA || TagC || TagD");
	//
	//consumer.subscribe("test1", "TagA || TagC || TagD || TagB || TagE");


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
        
        consumer.subscribe("test1", "TagA || TagC || TagD || TagB || TagE");
	
        consumer.start();


    while(!g_bStop)
    {
        sleep(1);
    }
	
    consumer.shutdown();

	sleep(1);
	
	delete listener;

	return 0;
}

