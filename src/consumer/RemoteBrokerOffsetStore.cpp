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

#include "RemoteBrokerOffsetStore.h"
#include "MQClientFactory.h"
#include "MQClientException.h"
#include "MQClientAPIImpl.h"
#include "CommandCustomHeader.h"
#include "UtilAll.h"

RemoteBrokerOffsetStore::RemoteBrokerOffsetStore(MQClientFactory* pMQClientFactory, const std::string& groupName) 
{
	m_pMQClientFactory = pMQClientFactory;
	m_groupName = groupName;
}

void RemoteBrokerOffsetStore::load()
{

}

/* modified by yu.guangjie at 2015-08-16, reason: add RemoteBrokerOffsetStore*/
void RemoteBrokerOffsetStore::updateOffset(MessageQueue& mq, long long offset, bool increaseOnly)
{
	std::map<MessageQueue, AtomicLong>::iterator it = m_offsetTable.find(mq);
	if (it == m_offsetTable.end())
    {
        m_offsetTable[mq] = offset;
        it = m_offsetTable.find(mq);
    }
    if (it != m_offsetTable.end()) 
    {
        if (increaseOnly) 
        {
            MixAll::compareAndIncreaseOnly(it->second, offset);
        }
        else 
        {
            it->second.Set(offset);
        }
    }
}

long long RemoteBrokerOffsetStore::readOffset(MessageQueue& mq, ReadOffsetType type)
{
    std::map<MessageQueue, AtomicLong>::iterator it;
    switch (type) 
    {
    case MEMORY_FIRST_THEN_STORE:
    case READ_FROM_MEMORY: 
        {
            it = m_offsetTable.find(mq);
            if (it != m_offsetTable.end()) 
            {
                return it->second.Get();
            }
            else if (READ_FROM_MEMORY == type) 
            {
                return -1;
            }
        }
    case READ_FROM_STORE: 
        {
            try 
            {
                long long offset = fetchConsumeOffsetFromBroker(mq);
                updateOffset(mq, offset, false);
                return offset;
            }
            catch (MQBrokerException& e) 
            {
                return -1;
            }
            catch (MQException& e) 
            {
                MqLogWarn("fetchConsumeOffsetFromBroker exception: %s", e.what());
                return -2;
            }
            break;
        }
    default:
        break;
    }

	return -1;
}

void RemoteBrokerOffsetStore::persistAll(std::set<MessageQueue>& mqs)
{

    if(mqs.empty())
    {
        return;
    }

    std::set<MessageQueue> unusedMQ;
    long times = ++m_storeTimesTotal;

    std::map<MessageQueue, AtomicLong>::iterator it = m_offsetTable.begin();
    for (; it != m_offsetTable.end(); it++) 
    {
        AtomicLong offset = it->second;
        if (mqs.find(it->first) != mqs.end()) 
        {
            try 
            {
                updateConsumeOffsetToBroker(it->first, offset.Get());
                // 每隔5分钟打印一次消费进度
                if ((times % 60) == 0) 
                {
                    MqLogNotice("Group: {%s}, ClientId: {%s}, updateConsumeOffsetToBroker: {%lld}",
                        m_groupName.c_str(), m_pMQClientFactory->getClientId().c_str(), offset.Get());
                }
            }
            catch (MQException& e) 
            {
                MqLogWarn("updateConsumeOffsetToBroker exception: %s", e.what());
            }
        }
        // 本地多余的队列，需要删除掉
        else 
        {
            unusedMQ.insert(it->first);
        }
    }

    if (!unusedMQ.empty()) 
    {
        std::set<MessageQueue>::iterator itset = unusedMQ.begin();
        for (; itset != unusedMQ.end(); itset++) 
        {
            m_offsetTable.erase(*itset);
        }
    }

}

void RemoteBrokerOffsetStore::persist(MessageQueue& mq)
{
    std::map<MessageQueue, AtomicLong>::iterator it = m_offsetTable.find(mq);
    if (it != m_offsetTable.end()) 
    {
        try 
        {
            updateConsumeOffsetToBroker(mq, it->second.Get());
            MqLogDebug("updateConsumeOffsetToBroker: {%lld}", it->second.Get());
        }
        catch (MQException& e) 
        {
            MqLogWarn("updateConsumeOffsetToBroker exception: %s", e.what());
        }
    }
}

void RemoteBrokerOffsetStore::updateConsumeOffsetToBroker(const MessageQueue& mq, long long offset)
{

    FindBrokerResult findBrokerResult = m_pMQClientFactory->findBrokerAddressInAdmin(mq.getBrokerName());
    if (findBrokerResult.brokerAddr.empty()) 
    {
        // 此处可能对Name Server压力过大，需要调优
        m_pMQClientFactory->updateTopicRouteInfoFromNameServer(mq.getTopic());
        findBrokerResult = m_pMQClientFactory->findBrokerAddressInAdmin(mq.getBrokerName());
    }

    if (!findBrokerResult.brokerAddr.empty()) 
    {
        UpdateConsumerOffsetRequestHeader* requestHeader = new UpdateConsumerOffsetRequestHeader();
        requestHeader->setTopic(mq.getTopic());
        requestHeader->setConsumerGroup(m_groupName);
        requestHeader->setQueueId(mq.getQueueId());
        requestHeader->setCommitOffset(offset);

        // 使用oneway形式，原因是服务器在删除文件时，这个调用可能会超时
        m_pMQClientFactory->getMQClientAPIImpl()->updateConsumerOffsetOneway(
            findBrokerResult.brokerAddr, requestHeader, 1000 * 5);
    }
    else 
    {
        THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.getBrokerName() + "] not exist", -1);
    }

}

long long RemoteBrokerOffsetStore::fetchConsumeOffsetFromBroker(const MessageQueue& mq)
{

    FindBrokerResult findBrokerResult = m_pMQClientFactory->findBrokerAddressInAdmin(mq.getBrokerName());
    if(findBrokerResult.brokerAddr.empty())
    {
        // 此处可能对Name Server压力过大，需要调优
        m_pMQClientFactory->updateTopicRouteInfoFromNameServer(mq.getTopic());
        findBrokerResult = m_pMQClientFactory->findBrokerAddressInAdmin(mq.getBrokerName());
    }

    if (!findBrokerResult.brokerAddr.empty()) 
    {
        QueryConsumerOffsetRequestHeader* requestHeader = new QueryConsumerOffsetRequestHeader();
        requestHeader->setTopic(mq.getTopic());
        requestHeader->setConsumerGroup(m_groupName);
        requestHeader->setQueueId(mq.getQueueId());

        return m_pMQClientFactory->getMQClientAPIImpl()->queryConsumerOffset(
            findBrokerResult.brokerAddr, requestHeader, 1000 * 5);
    }
    else 
    {
        THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.getBrokerName() + "] not exist", -1);
    }

	return 0;
}

void RemoteBrokerOffsetStore::removeOffset(MessageQueue& mq) 
{
	m_offsetTable.erase(mq);
}

long long RemoteBrokerOffsetStore::ReadOffsetByGroup(const MessageQueue& mq, std::string strGroupName)
{

    FindBrokerResult findBrokerResult = m_pMQClientFactory->findBrokerAddressInAdmin(mq.getBrokerName());
    if(findBrokerResult.brokerAddr.empty())
    {
        // 此处可能对Name Server压力过大，需要调优
        m_pMQClientFactory->updateTopicRouteInfoFromNameServer(mq.getTopic());
        findBrokerResult = m_pMQClientFactory->findBrokerAddressInAdmin(mq.getBrokerName());
    }

    if(!findBrokerResult.brokerAddr.empty())
    {
        QueryConsumerOffsetRequestHeader* requestHeader = new QueryConsumerOffsetRequestHeader();
        requestHeader->setTopic(mq.getTopic());
        requestHeader->setConsumerGroup(strGroupName);
        requestHeader->setQueueId(mq.getQueueId());
        try
        {
            return m_pMQClientFactory->getMQClientAPIImpl()->queryConsumerOffset(
                   findBrokerResult.brokerAddr, requestHeader, 1000 * 5);
        }
        catch(MQBrokerException& e)
        {
            return -1;
        }
    }
    else
    {
        THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.getBrokerName() + "] not exist", -1);
    }

    return 0;
}