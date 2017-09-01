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
* 消费进度存储到Consumer本地
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
	//lin.qiongshan, 2016年9月1日09:51:18，获取 offset 存放文件目录的环境变量名
	static const std::string OFFSET_STORE_DIR_ENV;

	//lin.qiongshan, 2016年9月1日09:51:18，查找并获取存放 offset 文件的路径
	//	1.先从环境变量获取：环境变量名通过常亮 OFFSET_STORE_DIR_ENV 定义
	//	2.如果环境变量获取失败，则取默认值 $(HOME)/.rocketmq_offsets
	//  3.如果都获取失败，返回当前目录 “.”
	std::string findOffsetStoreDir();
	//lin.qiongshan, 2016年9月1日09:51:18，查找并获取 offset 文件的相对路径：相对于 offsetStoreDir
	//	相对路径是：ClientId/groupName/offsets.json
	std::string findOffsetFileRelPath();
	//lin.qiongshan, 2016-9-19, 从本地文件读取 offset 数据（保存到 m_serializeWrapper）
	bool readOffsetFromLocal();

	MQClientFactory* m_pMQClientFactory;
	std::string m_groupName;
	std::string m_storePath;// 本地Offset存储路径（完整绝对路径）
	std::string m_offsetStoreDir;	///< lin.qiongshan, 2016年9月1日09:36:19, 保存 offset 文件的路径
	std::map<MessageQueue, AtomicLong> m_offsetTable;

	//m_offsetTable 操作互斥锁
	kpr::Mutex m_offsetTableLock;

	//lin.qs, 2016-9-19, offset 数据序列化和反序列化的操作类
	OffsetSerializeWrapper m_serializeWrapper;
};

#endif
