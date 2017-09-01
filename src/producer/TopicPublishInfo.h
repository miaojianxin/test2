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

#if!defined __TOPICPUBLISHINFO_H__
#define __TOPICPUBLISHINFO_H__

#include <list>
#include <vector>
#include <string>
#include <math.h>
#include <stdlib.h>
#include "MessageQueue.h"
#include "AtomicValue.h"

/**
* ����Topic�õ���·����Ϣ
*
*/
class TopicPublishInfo
{
public:
	//add by lin.qiongshan. ���Ӹ��ƹ��캯�������ظ�ֵ����������������������Ŀ�Ķ���Ϊ�����ƴ����Ա m_messageQueueList �ڱ���� MessageQueue ָ��
	TopicPublishInfo(const TopicPublishInfo& another) :
		m_orderTopic(another.m_orderTopic),
		m_sendWhichQueue(another.m_sendWhichQueue)
	{
		m_messageQueueList.clear();
		for (std::vector<MessageQueue*>::const_iterator itor = another.m_messageQueueList.begin();
			itor != another.m_messageQueueList.end();
			++itor)
		{
			MessageQueue* mq = new MessageQueue(**itor);
			m_messageQueueList.push_back(mq);
		}
	}

	TopicPublishInfo& operator=(const TopicPublishInfo& another)
	{
		m_orderTopic = another.m_orderTopic;
		m_sendWhichQueue = another.m_sendWhichQueue;
		m_messageQueueList.clear();
		for (std::vector<MessageQueue*>::const_iterator itor = another.m_messageQueueList.begin();
			itor != another.m_messageQueueList.end();
			++itor)
		{
			MessageQueue* mq = new MessageQueue(**itor);
			m_messageQueueList.push_back(mq);
		}
	}

	~TopicPublishInfo()
	{
		clearMessageQueue();
	}




	TopicPublishInfo()
	{
		m_orderTopic = false;
	}

	bool isOrderTopic()
	{
		return m_orderTopic;
	}

	bool ok()
	{
		return !m_messageQueueList.empty();
	}

	void setOrderTopic(bool orderTopic)
	{
		m_orderTopic = orderTopic;
	}

	std::vector<MessageQueue*>& getMessageQueueList()
	{
		return m_messageQueueList;
	}

	//�ú��������⣬����û�б����ã����޸�,ǳcopy����
	void setMessageQueueList(const std::vector<MessageQueue*>& messageQueueList)
	{
		m_messageQueueList = messageQueueList;
	}

	AtomicInteger& getSendWhichQueue()
	{
		return m_sendWhichQueue;
	}

	void setSendWhichQueue(AtomicInteger& sendWhichQueue)
	{
		m_sendWhichQueue = sendWhichQueue;
	}

	/**
	* ���lastBrokerName��Ϊnull����Ѱ�����䲻ͬ��MessageQueue
	*/
	MessageQueue* selectOneMessageQueue(const std::string lastBrokerName)
	{
		if (!lastBrokerName.empty())
		{
			int index = m_sendWhichQueue++;
			for (size_t i = 0; i < m_messageQueueList.size(); i++)
			{
				int pos = abs(index++) % m_messageQueueList.size();
				MessageQueue* mq = m_messageQueueList.at(pos);
				if (mq->getBrokerName()!=lastBrokerName)
				{
				    //MessageQueue *mq_new = new MessageQueue(mq->getTopic(),mq->getBrokerName(),mq->getQueueId());
					return mq;
				}
			}

			return NULL;
		}
		else
		{
		    if (m_messageQueueList.size() <= 0)
            {
                return NULL;
            }
			int index = m_sendWhichQueue++;
			int pos = abs(index) % m_messageQueueList.size();
			MessageQueue* mq = m_messageQueueList.at(pos);
            //MessageQueue *mq_new = new MessageQueue(mq->getTopic(),mq->getBrokerName(),mq->getQueueId());
		    return mq;
		}
	}
    

    void clearMessageQueue()
    {
        for (size_t i = 0; i < m_messageQueueList.size(); i++)
		{
			MessageQueue* mq = m_messageQueueList.at(i);
            /* modified by yu.guangjie at 2015-11-24, reason: */
			delete mq;
		}
        m_messageQueueList.clear();
    }
private:
	bool m_orderTopic;
	std::vector<MessageQueue*> m_messageQueueList;
	AtomicInteger m_sendWhichQueue;
};

#endif
