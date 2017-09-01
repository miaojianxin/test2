#include "OffsetSerializeWrapper.h"
#include "UtilAll.h"

std::string OffsetSerializeWrapper::Encode()
{
	MQJson::Value valueOffsets(MQJson::arrayValue);

	for (std::map<MessageQueue, AtomicLong>::const_iterator itor = m_offsetTable.begin();
		itor != m_offsetTable.end();
		++itor)
	{
		MQJson::Value valueMessageQueue(MQJson::objectValue);
		valueMessageQueue["broker"] = itor->first.getBrokerName();
		valueMessageQueue["topic"] = itor->first.getTopic();
		valueMessageQueue["queueId"] = itor->first.getQueueId();

		MQJson::Value valueOneOffsetPair(MQJson::objectValue);
		valueOneOffsetPair["messageQueue"] = valueMessageQueue;
		//NOTE!: jsoncpp 不支持 long int，这里可能出现精度错误
		valueOneOffsetPair["offset"] = (int)itor->second.Get();

		valueOffsets.append(valueOneOffsetPair);
	}
	
	return m_serializeWriter.write(valueOffsets);
}

bool OffsetSerializeWrapper::Decode(const std::string & data)
{
	MQJson::Value valueOffsets;
	MqLogDebug("Decode: data=%s", data.c_str());
	if (m_serializeReader.parse(data, valueOffsets))
	{
		m_offsetTable.clear();

		for (int index = 0; index < valueOffsets.size(); ++index)
		{
			MQJson::Value valueOffsetOnePair = valueOffsets[index];

			MQJson::Value valueMessageQueue = valueOffsetOnePair["messageQueue"];
			MessageQueue messageQueue;
			messageQueue.setBrokerName(valueMessageQueue["broker"].asString());
			messageQueue.setTopic(valueMessageQueue["topic"].asString());
			messageQueue.setQueueId(valueMessageQueue["queueId"].asInt());

			AtomicLong offset(valueOffsetOnePair["offset"].asInt());

			m_offsetTable.insert(std::make_pair(messageQueue, offset));
		}

		return true;
	}

	return false;
}

const std::map<MessageQueue, AtomicLong>& OffsetSerializeWrapper::GetOffsetTable() const
{
	return m_offsetTable;
}

void OffsetSerializeWrapper::GetOffsetTableCopy(std::map<MessageQueue, AtomicLong>& offsetTable) const
{
	offsetTable = m_offsetTable;
}

int OffsetSerializeWrapper::AddOffset(const MessageQueue & queue, const AtomicLong & offset)
{
	m_offsetTable[queue] = offset;
	return 0;
}

void OffsetSerializeWrapper::SetOffsetTable(const std::map<MessageQueue, AtomicLong>& offsetTable)
{
	m_offsetTable = offsetTable;
}
