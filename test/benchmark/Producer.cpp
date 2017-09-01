// Producer.cpp : 定义控制台应用程序的入口点。
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sys/time.h>

#ifdef WIN32
#   include <sys/timeb.h>
#   include <process.h>
#else
#   include <unistd.h>
#   include <sys/types.h>
#   include <signal.h>
#endif

#include <DefaultMQProducer.h>
#include <Message.h>
#include <SendResult.h>
#include <MQClientException.h>
#include "Thread.h"

const char *szTopic = "Default_TopicTest";
const char *szGroup = "Default_GroupTest";
const char *szTag = "";    
unsigned long count = 1;
int size = 50;
int interval = 1;

void MySleep(long millis)
{

#ifdef WIN32
	::Sleep(millis);
#else
	struct timespec tv;
	tv.tv_sec = millis / 1000;
	tv.tv_nsec = (millis % 1000) * 1000000;
	 nanosleep(&tv, 0);
#endif
}

unsigned long long GetCurrentTimeMillis()
{
#ifdef WIN32
	timeb tb;
	ftime(&tb);
	return tb.time * 1000ULL + tb.millitm;
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1000ULL+tv.tv_usec/1000;
#endif
}

void Usage(const char* program)
{
	printf("Usage:%s [-s ip:port] [-g group] [-c topic] [-t tag]"
        "[-n count] [-v size] [-i interval] [-T threadnum]\n",program);
    printf("\t -s nameserver addr, [default: getenv(\"NAMESRV_ADDR\")]\n");
    printf("\t -g producer group name, [default: Default_GroupTest]\n");
    printf("\t -c topic name, [default: Default_TopicTest]\n");
    printf("\t -t tag name, [default: null]\n");
	printf("\t -n message count, [default: 1]\n");
	printf("\t -v message size, [default: 50(B)] \n");
    printf("\t -i send interval, [default: 1(ms)] \n");
	printf("\t -T producer thread number, [default: 1] \n");
}

class ThreadProducer : public kpr::Thread
{
public:
	ThreadProducer( DefaultMQProducer* pProducer, int iThreadNo)
		:m_pProducer(pProducer), m_iThreadNo(iThreadNo)
	{
		char strName[32];
		sprintf(strName, "Producer%d", iThreadNo);
		SetName(strName);
	}
	virtual void Run();
	
private:
		
	DefaultMQProducer *m_pProducer;
	int m_iThreadNo;
};

void ThreadProducer::Run()
{
	char key[50];
    char szTime[20];
	char* value = new char[size];
    value[size-1] = '\0';

    struct tm tmNow;
    time_t tNow = time(NULL);
    time_t tLast = tNow;
    localtime_r(&tNow, &tmNow);
    snprintf(szTime,sizeof(szTime),"%ld-%02d%02d%02d", 
        getpid(), tmNow.tm_hour,tmNow.tm_min,tmNow.tm_sec);
    
	sprintf(value,"MQ-CPP[%s]", szTime);
    size_t iLen = strlen(value);
	for (int i = iLen; i < size - 10; i++)
	{
        value[i] = 'a' + i%3;
	}
    value[size - 10] = '\0';
    iLen = size - 10;

    long long nowtime = GetCurrentTimeMillis();
    long long lasttime = nowtime;
	unsigned int success=0;
    unsigned int sendfail=0;
    unsigned int respfail=0;
    unsigned int seq = 1;
	for (unsigned long i = 0; i < count; i++)
	{
	    tNow = time(NULL);
        if(tNow >= tLast+1)
        {
            localtime_r(&tNow, &tmNow);
            snprintf(szTime,sizeof(szTime),"%ld-%02d%02d%02d", 
                getpid(), tmNow.tm_hour,tmNow.tm_min,tmNow.tm_sec);

            nowtime = GetCurrentTimeMillis();
            printf("[TID%d]Send TPS: %u  Average RT: %.3f Send Failed: %u Response Failed: %u\n",
				m_iThreadNo, seq-1, (nowtime - lasttime - seq*interval + interval)/(double)seq, 
                sendfail, respfail);
            tLast = tNow;
            lasttime = nowtime;
            seq = 1;
            success = 0;
            respfail = 0;
            sendfail = 0;
        }
		try
		{
		    sprintf(key,"KEY[%s]%u", szTime, seq);
			sprintf(value + iLen,"%09u",seq++);
            
			Message msg(szTopic,// topic
				szTag,// tag
				key,// key
				value,// body
				strlen(value)+1);
			SendResult sendResult = m_pProducer->send(msg);
			success++;
            if(count < 100)
            {
                const char *status = NULL;
                switch(sendResult.getSendStatus())
                {
                case SEND_OK:
                    status = "SEND_OK";
                    break;
                case FLUSH_DISK_TIMEOUT:
                    status = "FLUSH_DISK_TIMEOUT";
                    break;
                case FLUSH_SLAVE_TIMEOUT:
                    status = "FLUSH_SLAVE_TIMEOUT";
                    break;
                case SLAVE_NOT_AVAILABLE:
                    status = "SLAVE_NOT_AVAILABLE";
                    break;
                default:
                    status = "UNKOWN";
                    break;
                }
                printf("[TID%d]sendStatus=%s,msgId=%s,key=%s,"
                    "messageQueue=[topic=%s,broker=%s,queue=%d],queueOffset=%lld\n",
                    m_iThreadNo, status, sendResult.getMsgId().c_str(),key,
                    sendResult.getMessageQueue().getTopic().c_str(),
                    sendResult.getMessageQueue().getBrokerName().c_str(),
                    sendResult.getMessageQueue().getQueueId(),
                    sendResult.getQueueOffset());
            }
		}
        catch (MQBrokerException& e)
        {
            ++respfail;
            printf("[TID%d]MQBrokerException(): %s\n", m_iThreadNo, e.what());
        }
		catch (MQClientException& e)
        {
            ++respfail;
            printf("[TID%d]MQClientException(): %s\n", m_iThreadNo, e.what());
        }
		catch (...) 
        {
			++sendfail;
            printf("[TID%d]Exception()\n", m_iThreadNo);
		}
        if(interval > 0)
        {
            MySleep(interval);
        }        
	}

    if(seq > 1)
    {
        nowtime = GetCurrentTimeMillis();
        printf("[TID%d]Send TPS: %u  Average RT: %.3f Send Failed: %u Response Failed: %u\n",
            m_iThreadNo, seq-1, (nowtime - lasttime - seq*interval + interval)/(double)seq, 
            sendfail, respfail);
    }
}


int main(int argc, char* argv[])
{
    Usage(argv[0]);

    // init vars
    const char *szSrvAddr = getenv("NAMESRV_ADDR");
	int iThreadNum = 1;

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
        else if (strcmp(argv[i],"-n")==0)
		{
			if (i+1 < argc)
			{
				count = atol(argv[i+1]);
				i++;
			}
			else
			{
				return -1;
			}
		}
		else if (strcmp(argv[i],"-v")==0)
		{
			if (i+1 < argc)
			{
				size = atoi(argv[i+1]);
				i++;
                if(size < 32)
                {
                    size = 32;
                    printf("NOTICE: adjust size = %d .\n", size);
                }
                else if(size > 1024*1024)
                {
                    size = 1024*1024;
                    printf("NOTICE: adjust size = %d .\n", size);
                }
			}
			else
			{
				return -1;
			}
		}
        else if (strcmp(argv[i],"-i")==0)
		{
			if (i+1 < argc)
			{
				interval = atoi(argv[i+1]);
                if(interval < 0)
                {
                    interval = 0;
                    printf("NOTICE: adjust interval = %d .\n", interval);
                }
				i++;
			}
			else
			{
				return -1;
			}
		}
		else if (strcmp(argv[i],"-T")==0)
		{
			if (i+1 < argc)
			{
				iThreadNum = atoi(argv[i+1]);
                if(iThreadNum < 1)
                {
                    interval = 1;
                    printf("NOTICE: adjust threadnum = %d .\n", iThreadNum);
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
    

	DefaultMQProducer producer(szGroup);
	producer.setNamesrvAddr(szSrvAddr);
	producer.start();

	ThreadProducer ** ppThead = new ThreadProducer *[iThreadNum];
	int i = 0;
	for (i = 0; i < iThreadNum; i++)
	{
	    ppThead[i] = new ThreadProducer(&producer, i);
		ppThead[i]->Start();
	}
	
	for (i = 0; i < iThreadNum; i++)
	{
		ppThead[i]->Join();
		delete ppThead[i];
	}
	delete [] ppThead;
	

	producer.shutdown();

	return 0;
}

