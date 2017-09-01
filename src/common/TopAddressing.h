/**
* Copyright (C) 2013 kangliqiang, kangliq@163.com
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
#if!defined __TOPADDRESSING_H__
#define  __TOPADDRESSING_H__

#include <string>

/**
 * Ѱַ����
 *
 */
class TopAddressing
{
public:
	//��̬��������ַ��
	//	1����ʹ�� setNsAddr �������õĵ�ַ 
	//	2�����û�����þ�̬���������ַ������ DEFAULT_WS_ADDR ��һ���̶��������͵�ַ���˿ڹ̶� 8080����ͨ�� host ��������ʵ��ָ��� IP ��ַ
	//	3��������� DEFAULT_WS_ADDR ��ʧ�ܣ������������ DEFAULT_LOCAL_WS_ADDR ��һ�����ػػ���ַ���̶��˿� 8080
	static const std::string DEFAULT_WS_ADDR;
	static const std::string DEFAULT_LOCAL_WS_ADDR;

	TopAddressing();

	~TopAddressing();

	std::string fetchNSAddr();

	const std::string& getNsAddr()
	{
		return m_nsAddr;
	}


	void setNsAddr(const std::string& nsAddr)
	{
		m_nsAddr = nsAddr;
	}

	const std::string& getWsAddr()
	{
		return m_wsAddr;
	}

	void setWsAddr(const std::string& wsAddr)
	{
		m_wsAddr = wsAddr;
	}

private:
	//��ָ����ַ�������󣬲�������Ӧ��Ϣ
	int request(const std::string& url, std::string& response);

	std::string m_nsAddr;

	std::string m_wsAddr;	///< �������� nameserver ��ַ�ľ�̬��������ַ
};

#endif
