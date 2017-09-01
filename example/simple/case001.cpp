// UTF-8

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <string>
#include <iostream>
#include <vector>

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
#include <DefaultMQPushConsumer.h>
#include <MessageListener.h>
#include <Message.h>
#include <MessageExt.h>
#include <MessageQueue.h>
#include <MQClientException.h>


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

long long str2ll(const char *str);

class MsgListener : public MessageListenerConcurrently
{
public:
    ConsumeConcurrentlyStatus consumeMessage(std::list<MessageExt*>& msgs,
                                            ConsumeConcurrentlyContext& context)
    {
        char szTime[32];
        struct tm tmNow;
        time_t tNow = time(NULL);
#ifdef WIN32
        localtime_s(&tmNow,&tNow);
#else
        localtime_r(&tNow, &tmNow);
#endif
		sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",
			tmNow.tm_year+1900, tmNow.tm_mon+1, tmNow.tm_mday,
			tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
        printf("[%s]: consume %lu messages\n", szTime, msgs.size());

        std::list<MessageExt*>::iterator it = msgs.begin();
        int i = 0;
        for (;it != msgs.end();it++)
        {
            MessageExt* me = *it;
            std::string str;
            str.assign(me->getBody(),me->getBodyLen());
            printf("Topic[%s],Msg[%s]: %s\n", 
                context.messageQueue.getTopic().c_str(), 
                me->getMsgId().c_str(), str.c_str());
        }

        return CONSUME_SUCCESS;
    }

};

bool g_bStop = false;

static void sig_handler(const int sig) {
#ifdef WIN32
    printf("Signal handled: %d.\n", sig);
#else
    printf("Signal handled: %s.\n", strsignal(sig));
#endif
    
    g_bStop = true;
}

int main(int argc, char* argv[])
{
#ifndef WIN32
    signal(SIGINT, sig_handler);
#endif
    int count = 10;
    const char *ZMQ_SERVER = "10.45.61.30:9876";
    const char *topic = "CASE001_TOPIC";
    const char *value = "您好！截至05月05日19时，您已欠费11.13元，信用度即将用户。【安徽移动手机营业厅】每月28日交话费9折（10万名额），名额满后9.8折，点 ah.10086.cn/dt/khd";

    char szTime[32];
    struct tm tmNow;    

    // producer
    DefaultMQProducer producer("PID_001");
    producer.setNamesrvAddr(ZMQ_SERVER);
    producer.start();

    time_t tNow = time(NULL);
#ifdef WIN32
    localtime_s(&tmNow,&tNow);
#else
    localtime_r(&tNow, &tmNow);
#endif
	sprintf(szTime,"%04d-%02d-%02d %02d:%02d:%02d",
		tmNow.tm_year+1900, tmNow.tm_mon+1, tmNow.tm_mday,
		tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
    
    for (int i = 0; i < count; i++) {
        try
        {
            Message msg(topic,// topic
                "",// tag
                "",// key
                value,// body
                strlen(value)+1);
            SendResult sendResult;
            sendResult = producer.send(msg);
            printf("[%s]%d: sendresult=%d,msgid=%s\n",szTime,i, 
                sendResult.getSendStatus(),sendResult.getMsgId().c_str());
        }
        catch (MQClientException& e) {
            std::cout<<e<<std::endl;
            MySleep(3000);
        }
        catch (MQBrokerException& e) {
            std::cout<<e<<std::endl;
            MySleep(3000);
        }
    }

    producer.shutdown();


    // consumer
    MySleep(20000);
    
    DefaultMQPushConsumer consumer("CID_001");
    consumer.setNamesrvAddr(ZMQ_SERVER);
#ifdef WIN32
    long pid = _getpid();
#else
    pid_t pid = getpid();
#endif
    char szPid[20];
    sprintf(szPid, "PID[%lu]", pid);
    consumer.setInstanceName(szPid);
    consumer.subscribe(topic, "*");

    MessageListener *listener = new MsgListener();
    consumer.registerMessageListener(listener);
    consumer.start();

    while(!g_bStop)
    {
        MySleep(1000);
    }
    consumer.shutdown();
    
    delete listener;

    return 0;
}

