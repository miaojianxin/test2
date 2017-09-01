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

#include "AtomicValue.h"

long long str2ll(const char *str);
unsigned long long GetCurrentTimeMillis();

bool g_bResultSuccess = true;

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
            if(consumeTimes < 10)
            {
                printf("msgId=%s,key=%s,messageQueue=[topic=%s,broker=%s,queue=%d],"
                    "queueOffset=%lld,commitLogOffset=%lld\n", 
                    me->getMsgId().c_str(),me->getProperty(Message::PROPERTY_KEYS).c_str(),
                    context.messageQueue.getTopic().c_str(), 
                    context.messageQueue.getBrokerName().c_str(),
                    context.messageQueue.getQueueId(),
                    me->getQueueOffset(), me->getCommitLogOffset());
            }            
            ++consumeTimes;
		}
        if(g_bResultSuccess)
        {
            return CONSUME_SUCCESS;
        }

		return RECONSUME_LATER;
	}

	AtomicLong consumeTimes;
};


void Usage(const char* program)
{
	printf("Usage:%s [-s ip:port] [-g group] [-c topic] [-t tag]"
        "[-r result] \n",program);
    printf("\t -s nameserver addr, [default: getenv(\"NAMESRV_ADDR\")]\n");
    printf("\t -g producer group name, [default: Default_GroupTest]\n");
    printf("\t -c topic name, [default: Default_TopicTest]\n");
    printf("\t -t tag name, [default: *]\n");
    printf("\t -r is result success?, [default: true]\n");
}


int main(int argc, char* argv[])
{
    Usage(argv[0]);

    // init vars
    const char *szSrvAddr = getenv("NAMESRV_ADDR");
    const char *szTopic = "Default_TopicTest";
    const char *szGroup = "Default_GroupTest";
    const char *szTag = "*";    
	unsigned long count = 1;
    int interval = 1;
    
	for (int i=1; i< argc; i++)
	{
		if (strcmp(argv[i],"-s")==0)
		{
			if (i+1 < argc)
			{
			    szSrvAddr = argv[i+1];
				i++;
			}
			else
			{
				return 0;
			}
		}
        else if (strcmp(argv[i],"-c")==0)
		{
			if (i+1 < argc)
			{
				szTopic = argv[i+1];
				i++;
			}
			else
			{
				return -1;
			}
		}
        else if (strcmp(argv[i],"-g")==0)
		{
			if (i+1 < argc)
			{
				szGroup = argv[i+1];
				i++;
			}
			else
			{
				return -1;
			}
		}
        else if (strcmp(argv[i],"-t")==0)
		{
			if (i+1 < argc)
			{
				szTag = argv[i+1];
				i++;
			}
			else
			{
				return -1;
			}
		}
        else if (strcmp(argv[i],"-r")==0)
		{
			if (i+1 < argc)
			{
			    if(strcasecmp("true", argv[i+1]) != 0)
                {
                    g_bResultSuccess = false;
                }
				
				i++;
			}
			else
			{
				return -1;
			}
		}
		else
		{
		    printf("ERROR: invalid parameter[%s]!!!\n", argv[i]);
			return -1;
		}
	}

    if(NULL == szSrvAddr)
    {
        printf("ERROR: nameserver address is needed!!!\n");
        return -1;
    }

	DefaultMQPushConsumer consumer(szGroup);
	consumer.setNamesrvAddr(szSrvAddr);

    pid_t pid = getpid();
    char szPid[20];
    sprintf(szPid, "PID[%lu]", pid);
    consumer.setInstanceName(szPid);
    consumer.setConsumeMessageBatchMaxSize(10);

	consumer.subscribe(szTopic, szTag);

    MsgListener *listener = NULL;
    listener = new MsgListener();

	consumer.registerMessageListener(listener);

	consumer.start();

    long long nowtime = GetCurrentTimeMillis();
    long long lasttime = nowtime;

    long long curcount = 0;
    long long lastcount = 0;
    while(true)
    {
        sleep(1);

        curcount = listener->consumeTimes.Get();
        nowtime = GetCurrentTimeMillis();
        
        //printf("Consume TPS: %lld\n", (curcount-lastcount)*1000/(nowtime-lasttime));
        printf("Consume TPS: %lld\n", (curcount-lastcount));
        lasttime = nowtime;
        lastcount = curcount; 
    }

    //consumer.shutdown();
    printf("Exit!\n");
	
	delete listener;

	return 0;
}
