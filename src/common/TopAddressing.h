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
 * 寻址服务
 *
 */
class TopAddressing
{
public:
	//静态服务器地址：
	//	1）先使用 setNsAddr 方法设置的地址 
	//	2）如果没有设置静态服务器其地址，则尝试 DEFAULT_WS_ADDR ：一个固定的域名和地址（端口固定 8080），通过 host 配置域名实际指向的 IP 地址
	//	3）如果请求 DEFAULT_WS_ADDR 还失败，则最后尝试请求 DEFAULT_LOCAL_WS_ADDR ：一个本地回环地址，固定端口 8080
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
	//向指定地址发起请求，并返回响应信息
	int request(const std::string& url, std::string& response);

	std::string m_nsAddr;

	std::string m_wsAddr;	///< 用于请求 nameserver 地址的静态服务器地址
};

#endif
