#ifndef _OFFSET_SERIALIZE_WRAPPER_H_
#define _OFFSET_SERIALIZE_WRAPPER_H_

/** consume offset 数据序列化/反序列化操作类
	@date 2016-9-18
	@author lin.qs
*/
#include <string>
#include <map>

#include "MessageQueue.h"
#include "AtomicValue.h"
#include "json/json.h"

class OffsetSerializeWrapper
{
public:
	//编码（序列化）接口。根据内部保存的 offset 数据进行编码（AddOffset 用于添加 offset 数据）
	std::string Encode();

	//解码（反序列化）接口。无论解码是否成功，不保证内部保存的 offset 数据的完整性（可能被清空，可能不变，可能包含部分解码结果，可能包含全部解码结果）
	bool Decode(const std::string& data);

	//返回内部保存的 offset 表数据
	const std::map<MessageQueue, AtomicLong>& GetOffsetTable() const;
	 
	//获取内部保存的 offset 数据的拷贝
	//	offsetTab [out] ：保存内部 offset 数据的拷贝。如果传入时已有数据，会被全部清空
	void GetOffsetTableCopy(std::map<MessageQueue, AtomicLong>& offsetTable) const;

	//添加一个 offset 数据，如果有重复的 queue，则会覆盖旧的值
	int AddOffset(const MessageQueue& queue, const AtomicLong& offset);

	//保存 offset 数据。会覆盖 Wrapper 内部已有数据
	void SetOffsetTable(const std::map<MessageQueue, AtomicLong>& offsetTable);

	void Clear() { m_offsetTable.clear(); }
private:
	//保存要序列化 offset 数据或反序列化的结果
	std::map<MessageQueue, AtomicLong> m_offsetTable;

	//用于进行序列化操作的对象
	MQJson::FastWriter m_serializeWriter;

	//用于进行反序列化操作的对象
	MQJson::Reader m_serializeReader;
};

#endif