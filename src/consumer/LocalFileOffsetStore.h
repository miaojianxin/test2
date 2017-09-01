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

#if!defined __LOCALFILEOFFSETSTORE_H__
#define __LOCALFILEOFFSETSTORE_H__

#include "OffsetStore.h"
#include <map>
#include <string>
#include <set>
#include "OffsetSerializeWrapper.h"
#include "Mutex.h"

class MQClientFactory;
class MessageQueue;

/**
* ���ѽ��ȴ洢��Consumer����
* 
*/
class LocalFileOffsetStore : public OffsetStore
{
public:
	LocalFileOffsetStore(MQClientFactory* pMQClientFactory, const std::string& groupName);

	void load();
	void updateOffset(MessageQueue& mq, long long offset, bool increaseOnly);
	long long readOffset(MessageQueue& mq, ReadOffsetType type);
	void persistAll(std::set<MessageQueue>& mqs);
	void persist(MessageQueue& mq);
	void removeOffset(MessageQueue& mq);
	long long ReadOffsetByGroup(const MessageQueue& mq,std::string strGroupName);
private:
	//lin.qiongshan, 2016��9��1��09:51:18����ȡ offset ����ļ�Ŀ¼�Ļ���������
	static const std::string OFFSET_STORE_DIR_ENV;

	//lin.qiongshan, 2016��9��1��09:51:18�����Ҳ���ȡ��� offset �ļ���·��
	//	1.�ȴӻ���������ȡ������������ͨ������ OFFSET_STORE_DIR_ENV ����
	//	2.�������������ȡʧ�ܣ���ȡĬ��ֵ $(HOME)/.rocketmq_offsets
	//  3.�������ȡʧ�ܣ����ص�ǰĿ¼ ��.��
	std::string findOffsetStoreDir();
	//lin.qiongshan, 2016��9��1��09:51:18�����Ҳ���ȡ offset �ļ������·��������� offsetStoreDir
	//	���·���ǣ�ClientId/groupName/offsets.json
	std::string findOffsetFileRelPath();
	//lin.qiongshan, 2016-9-19, �ӱ����ļ���ȡ offset ���ݣ����浽 m_serializeWrapper��
	bool readOffsetFromLocal();

	MQClientFactory* m_pMQClientFactory;
	std::string m_groupName;
	std::string m_storePath;// ����Offset�洢·������������·����
	std::string m_offsetStoreDir;	///< lin.qiongshan, 2016��9��1��09:36:19, ���� offset �ļ���·��
	std::map<MessageQueue, AtomicLong> m_offsetTable;

	//m_offsetTable ����������
	kpr::Mutex m_offsetTableLock;

	//lin.qs, 2016-9-19, offset �������л��ͷ����л��Ĳ�����
	OffsetSerializeWrapper m_serializeWrapper;
};

#endif
