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
#include "SendResult.h"

SendResult::SendResult()
	:m_sendStatus(SEND_OK)
{
	//mjx test modify
	//alimq返回值只有msgid有意义，其他选项无意义
	//reset
	m_queueOffset = -1;
	m_messageQueue.setTopic("");
	m_messageQueue.setBrokerName("");
	m_messageQueue.setQueueId(-1);
	m_msgId = "";
	
}
	
	


SendResult::SendResult(const SendStatus& sendStatus,
	const std::string&  msgId,
	MessageQueue& messageQueue,
	long long queueOffset,
	std::string&  projectGroupPrefix)
	:m_sendStatus(sendStatus),
	m_msgId(msgId),
	m_messageQueue(messageQueue),
	m_queueOffset(queueOffset)
{

	// 清除虚拟运行环境相关的projectGroupPrefix
	//if (!UtilAll::isBlank(projectGroupPrefix))
	//{
	//	m_messageQueue.setTopic(VirtualEnvUtil::clearProjectGroup(m_messageQueue.getTopic(),
	//		projectGroupPrefix));
	//}
}

const std::string&  SendResult::getMsgId()
{
	return m_msgId;
}

//mjx test modify
void SendResult::setMsgId(const std::string&  msgId)
{
	m_msgId = msgId;
}

SendStatus SendResult::getSendStatus()
{
	return m_sendStatus;
}

void SendResult::setSendStatus(const SendStatus& sendStatus)
{
	m_sendStatus = sendStatus;
}

MessageQueue& SendResult::getMessageQueue()
{
	return m_messageQueue;
}

void SendResult::setMessageQueue(MessageQueue& messageQueue)
{
	m_messageQueue = messageQueue;
}

long long SendResult::getQueueOffset()
{
	return m_queueOffset;
}

//mjx test modify
void SendResult::setQueueOffset(long long queueOffset)
{
	m_queueOffset = queueOffset;
}




