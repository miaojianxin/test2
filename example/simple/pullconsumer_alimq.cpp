

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <map>
#include <set>

#include <DefaultMQPullConsumer.h>
#include <Message.h>
#include <MessageExt.h>
#include <MessageQueue.h>
#include <PullResult.h>
#include <MQClientException.h>

struct ACLASS
{
	int a;
	int b;
	int c;

};


using namespace std;

std::map<MessageQueue, long long> offseTable;

void putMessageQueueOffset(const MessageQueue& mq, long long offset)
{
	offseTable[mq]=offset;
}

long long getMessageQueueOffset(const MessageQueue& mq) 
{
	std::map<MessageQueue, long long>::iterator it = offseTable.find(mq);

	if (it!=offseTable.end())
	{
		return it->second;
	}

	return 0;
}

int g_cnt = 0;

void PrintResult(PullResult& result)
{
	std::cout<<"[pullStatus="<<result.pullStatus
		<<"][nextBeginOffset="<<result.nextBeginOffset
		<<"][minOffset="<<result.minOffset
		<<"][maxOffset="<<result.maxOffset
		<<"]"<<std::endl;

	std::list<MessageExt*>::iterator it = result.msgFoundList.begin();

	for (;it!=result.msgFoundList.end();it++)
	{
		MessageExt* me = *it;
		std::string str;
		str.assign(me->getBody(),me->getBodyLen());
		//to do test
		//测试消息对象的各属性
		std::cout<<"message queue id:"<<me->getQueueId()
		         <<",message msg id:"<<me->getMsgId()
		         <<",message queue offset:"<<me->getQueueOffset()
		         <<",message topic:"<<me->getTopic()
		         <<",message tags:"<<me->getTags()
		         <<",message keys:"<<me->getKeys()
		         <<",message properties:"<<me->getProperty("user_property")
		         <<std::endl;
		
		std::cout<<str<<std::endl;

		//测试二进制消息
		if(g_cnt %3 == 0)
		{
			ACLASS* pa;
			pa = (ACLASS*)(char*)(me->getBody());
			std::cout<<pa->a<<","<<pa->b<<","<<pa->c<<endl;

		}

		g_cnt++;
	}
}

int main(int argc, char* argv[])
{
	if (argc<2)
	{
		printf("Usage:%s webaddress\n",argv[0]);
		return 0;
	}

	DefaultMQPullConsumer consumer("CID_TEST1");
	
	consumer.setAccessKey("539f7db388b74879863f9335bbe4e3f6");
    consumer.setSecretKey("wjDVEg6Qs2ySkm1g/AJPXz/RZtc=");

	consumer.setMessageModel(CLUSTERING);
	
	consumer.setNamesrvAddr(argv[1]);
	consumer.start();

	std::set<MessageQueue>* mqs = consumer.fetchSubscribeMessageQueues("test1");

	std::set<MessageQueue>::iterator it = mqs->begin();

	for (;it!=mqs->end();it++)
	{
		MessageQueue mq = *it;
		
		bool noNewMsg = false;
		
		while (!noNewMsg)
		{
			try
			{
						

				// false 表示从内存中的offset table中获取offset        
				// true 表示从远端的服务器中获取最后消费的offset  
				
				long long offset = consumer.fetchConsumeOffset(mq, false);       
				if (offset == -1) offset = 0;

				long long offset_server = consumer.fetchConsumeOffset(mq, true);
				
				printf("----- the broker name :%s, queue id %d-----\n",mq.getBrokerName().c_str(),mq.getQueueId());
				
				printf("the queue %d: current offset from memory is %d, offset from server is%d, \n", mq.getQueueId(),offset,offset_server);

				printf("the queue %d :max offset is %d\n",mq.getQueueId(),consumer.maxOffset(mq));

				printf("the queue %d :min offset is %d\n",mq.getQueueId(),consumer.minOffset(mq));
				
				
				PullResult* pullResult = consumer.pull(mq, "*", offset, 10);

				

			   // if pull request timeout or received NULL response, pullStatus will be        
			   // setted to BROKER_TIMEOUT,        
			   // And nextBeginOffset/minOffset/MaxOffset will be setted to 0 

			   
			   if (pullResult->pullStatus != BROKER_TIMEOUT) 
			   {           
			   		// 更新内存中的offset table   
			   		printf("the queue %d :update offset is %d\n",mq.getQueueId(),pullResult->nextBeginOffset);

					consumer.updateConsumeOffset(mq, pullResult->nextBeginOffset);         

					// 更新服务器上的消费进度，这个需要定时做，要更新的mq必须是保存在内存中的offset table中
					consumer.persistConsumeOffset(mq);
					
			   } 

			   else
			  {          
					cout << "broker timeout occur" << endl;        
			  }
				
												
				switch (pullResult->pullStatus)
				{
				case FOUND:

					 printf(
                "find consumer msg, its topic is:%s, its broker name is:%s, "
                "its queueId is:%d\n",mq.getTopic().c_str(), mq.getBrokerName().c_str(),mq.getQueueId());

					 PrintResult(*pullResult);
					// 
					break;
				case NO_MATCHED_MSG:
					break;
				case NO_NEW_MSG:
					noNewMsg = true;
					break;
				case OFFSET_ILLEGAL:
					break;
				default:
					break;
				}

				delete pullResult;
			}
			catch (MQClientException& e)
			{
				std::cout<<e<<std::endl;
			}
		}
	}


	delete mqs;
	consumer.shutdown();

	return 0;
}

