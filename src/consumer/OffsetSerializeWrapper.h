#ifndef _OFFSET_SERIALIZE_WRAPPER_H_
#define _OFFSET_SERIALIZE_WRAPPER_H_

/** consume offset �������л�/�����л�������
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
	//���루���л����ӿڡ������ڲ������ offset ���ݽ��б��루AddOffset ������� offset ���ݣ�
	std::string Encode();

	//���루�����л����ӿڡ����۽����Ƿ�ɹ�������֤�ڲ������ offset ���ݵ������ԣ����ܱ���գ����ܲ��䣬���ܰ������ֽ����������ܰ���ȫ����������
	bool Decode(const std::string& data);

	//�����ڲ������ offset ������
	const std::map<MessageQueue, AtomicLong>& GetOffsetTable() const;
	 
	//��ȡ�ڲ������ offset ���ݵĿ���
	//	offsetTab [out] �������ڲ� offset ���ݵĿ������������ʱ�������ݣ��ᱻȫ�����
	void GetOffsetTableCopy(std::map<MessageQueue, AtomicLong>& offsetTable) const;

	//���һ�� offset ���ݣ�������ظ��� queue����Ḳ�Ǿɵ�ֵ
	int AddOffset(const MessageQueue& queue, const AtomicLong& offset);

	//���� offset ���ݡ��Ḳ�� Wrapper �ڲ���������
	void SetOffsetTable(const std::map<MessageQueue, AtomicLong>& offsetTable);

	void Clear() { m_offsetTable.clear(); }
private:
	//����Ҫ���л� offset ���ݻ����л��Ľ��
	std::map<MessageQueue, AtomicLong> m_offsetTable;

	//���ڽ������л������Ķ���
	MQJson::FastWriter m_serializeWriter;

	//���ڽ��з����л������Ķ���
	MQJson::Reader m_serializeReader;
};

#endif