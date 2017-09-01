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
#include <cstdlib>
#include "LocalFileOffsetStore.h"

#include "MQClientFactory.h"
#include "MQFileUtil.h"
#include "ScopedLock.h"

const std::string LocalFileOffsetStore::OFFSET_STORE_DIR_ENV = "LOCALFILE_OFFSET_STORE_DIR";

LocalFileOffsetStore::LocalFileOffsetStore(MQClientFactory* pMQClientFactory, 
	const std::string& groupName) 
{
	m_pMQClientFactory = pMQClientFactory;
	m_groupName = groupName;
	m_storePath = findOffsetStoreDir() + MQFileUtil::PATH_SEPARATOR + findOffsetFileRelPath();
}

void  LocalFileOffsetStore::load() 
{
	readOffsetFromLocal();

	kpr::ScopedLock<kpr::Mutex> lock(m_offsetTableLock);
	m_serializeWrapper.GetOffsetTableCopy(m_offsetTable);

	MqLogVerb("ConsumerGroup[%s] is Loading LocalFileOffsetStore... (from %s)", m_groupName.c_str(), m_storePath.c_str());
	for (std::map<MessageQueue, AtomicLong>::const_iterator itorOffset = m_offsetTable.begin();
		itorOffset != m_offsetTable.end();
		++itorOffset)
	{
		MqLogVerb("Load Item: {broker:%s, topic:%s, queueId:%d} : {offset: %lld}",
			itorOffset->first.getBrokerName().c_str(),
			itorOffset->first.getTopic().c_str(),
			itorOffset->first.getQueueId(),
			itorOffset->second.Get()
			);
	}
}


void  LocalFileOffsetStore::updateOffset(MessageQueue& mq, long long offset, bool increaseOnly)
{
	MqLogNotice("LocalFileOffsetStore::updateOffset[mq:{broker:%s, topic:%s, queueId:%d}, offset:%lld] begin...",
			mq.getBrokerName().c_str(),
			mq.getTopic().c_str(),
			mq.getQueueId(),
			offset);

	kpr::ScopedLock<kpr::Mutex> lock(m_offsetTableLock);

	std::map<MessageQueue, AtomicLong>::const_iterator itorOffset = m_offsetTable.find(mq);
	if (m_offsetTable.end() == itorOffset)
	{
		m_offsetTable.insert(std::make_pair(mq, AtomicLong(offset)));
	}
	else
	{
		AtomicLong offsetOld = itorOffset->second;
		if (increaseOnly)
		{
			MixAll::compareAndIncreaseOnly(offsetOld, offset);
			m_offsetTable[mq] = offsetOld;
		}
		else
		{
			m_offsetTable[mq] = offset;
		}
	}
}

long long  LocalFileOffsetStore::readOffset(MessageQueue& mq, ReadOffsetType type)
{
	std::map<MessageQueue, AtomicLong>::const_iterator itorOffset = m_offsetTable.find(mq);

    switch(type)
    {
	case READ_FROM_MEMORY: 
	{
		if (m_offsetTable.end() == itorOffset)
		{
			return -1;
		}
		else
		{
			return itorOffset->second.Get();
		}
	}
	case MEMORY_FIRST_THEN_STORE:
	{
		if (m_offsetTable.end() != itorOffset)
		{
			return itorOffset->second.Get();
		}

		//����ڴ��в����ڣ������ߵ� case READ_FROM_STORE���ӱ��ش��̶�ȡ
	}
	case READ_FROM_STORE: 
	{
		//�ӱ��ض�ȡ offset ʱ��ͬʱ���ƶ� MessageQueue �� offset ���µ��ڴ棨this.m_offsetTable����
		if (!readOffsetFromLocal())
		{
			MqLogWarn("�ӱ��ض�ȡ offset ����ʧ��");
			return -1;
		}

		itorOffset = m_serializeWrapper.GetOffsetTable().find(mq);
		if (m_serializeWrapper.GetOffsetTable().end() == itorOffset)
		{
			return -1;
		}
		else
		{
			updateOffset(mq, itorOffset->second, false);
			return itorOffset->second.Get();
		}
	}
	default:
		break;
	}

	return -1;
}

void  LocalFileOffsetStore::persistAll(std::set<MessageQueue>& mqs)
{
	MqLogVerb("LocalFileOffsetStore::persistAll begin..., size=%lld", mqs.size());
	if (mqs.empty())
	{
		return;
	}

	m_serializeWrapper.Clear();
	for (std::set<MessageQueue>::const_iterator itor = mqs.begin();
		itor != mqs.end();
		++itor)
	{
		std::map<MessageQueue, AtomicLong>::const_iterator itorOffset = m_offsetTable.find(*itor);
		if (itorOffset != m_offsetTable.end())
		{
			m_serializeWrapper.AddOffset(itorOffset->first, itorOffset->second);
		}
		else
		{
			MqLogWarn("{broker:%s, topic:%s, queueId:%d} not in LocalFileOffsetStore",
				itor->getBrokerName().c_str(),
				itor->getTopic().c_str(),
				itor->getQueueId());
		}
	}

	std::string serializeContent = m_serializeWrapper.Encode();
	MQFileUtil::SafeWriteFile(serializeContent, m_storePath);
}

void  LocalFileOffsetStore::persist(MessageQueue& mq)
{
	//ʵ�ʲ�֧�ֵ������е� offset �־û�
}

void  LocalFileOffsetStore::removeOffset(MessageQueue& mq)
{
	// ���ѽ��ȴ洢��Consumer����ʱ�ݲ��� offset ����
}

std::string LocalFileOffsetStore::findOffsetStoreDir()
{
	std::string offsetStoreDir = ".";

	const char* pszEnvValue = NULL;

	pszEnvValue = getenv(OFFSET_STORE_DIR_ENV.c_str());
	if (NULL != pszEnvValue)
	{
		offsetStoreDir = pszEnvValue;
	}
	else
	{
		pszEnvValue = getenv("HOME");
		if (NULL != pszEnvValue)
		{
			offsetStoreDir = pszEnvValue;
			offsetStoreDir += MQFileUtil::PATH_SEPARATOR;
			offsetStoreDir += ".rocketmq_offsets";
		}
	}

	return offsetStoreDir;
}

std::string LocalFileOffsetStore::findOffsetFileRelPath()
{
	return std::string("") + m_pMQClientFactory->getClientId() + MQFileUtil::PATH_SEPARATOR + m_groupName + MQFileUtil::PATH_SEPARATOR + "offsets.json";
}

bool LocalFileOffsetStore::readOffsetFromLocal()
{
	std::string fileContent;
	if (!MQFileUtil::ReadWholeFile(fileContent, m_storePath))
	{
		//��ȡʧ�ܲ�����������Ϊ���ʼʱ���ļ��ǲ����ڵģ���ȡʧ��ʱ������
		MqLogVerb("��ȡ�����ļ�[%s]ʧ��", m_storePath.c_str());
		return true;
	}

	if (!m_serializeWrapper.Decode(fileContent))
	{
		MqLogWarn("�����л� offset ����ʧ��");
		return false;
	}

	return true;
}

long long LocalFileOffsetStore::ReadOffsetByGroup(const MessageQueue& mq, std::string strGroupName)
{
    return 0;
}