/**
 * Copyright (C) 2010-2013 kangliqiang <kangliq@163.com>
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
#if!defined __CLIENTCONFIG_H__
#define __CLIENTCONFIG_H__

#include <string>
#include "RocketMQClient.h"

/**
 * Producer与Consumer的公共配置
 *
 * @author kangliqiang <kangliq@163.com>
 * @since 2013-10-31
 */
class ROCKETMQCLIENT_API ClientConfig
{
public:
	ClientConfig();
	virtual ~ClientConfig();
	
	//clientid=ip@instanceName
	std::string buildMQClientId();

	void resetClientConfig(const ClientConfig& cc);
	ClientConfig cloneClientConfig();

	std::string getNamesrvAddr();
	void setNamesrvAddr(const std::string& namesrvAddr);

	//mjx namesrv,onsaddr modify
	std::string getNSAddr();
	void setNSAddr(const std::string& onsAddr);


	std::string getClientIP();
	void setClientIP(const std::string& clientIP);

	std::string getInstanceName();
	void setInstanceName(const std::string& instanceName);

	int getClientCallbackExecutorThreads();
	void setClientCallbackExecutorThreads(int clientCallbackExecutorThreads);

	int getPollNameServerInteval();
	void setPollNameServerInteval(int pollNameServerInteval);

	int getHeartbeatBrokerInterval();
	void setHeartbeatBrokerInterval(int heartbeatBrokerInterval);

	int getPersistConsumerOffsetInterval();
	void setPersistConsumerOffsetInterval(int persistConsumerOffsetInterval);

    int getLogLevel();
    void setLogLevel(int iLevel);
    
	//如果 InstanceName 是默认值（"DEFAULT"），则将 InstanceName 替换为 pid。这是为了在同主机多个进程启动客户端实例时，避免 clientId 重复
	void changeInstanceNameToPID();

    /* modified by yu.guangjie at 2017-06-23, reason: */
    std::string getAccessKey();
	void setAccessKey(const std::string& accessKey);
    std::string getSecretKey();
	void setSecretKey(const std::string& secretKey);
    
private:
	int m_clientCallbackExecutorThreads;
	int m_pollNameServerInteval;
	int m_heartbeatBrokerInterval;
	int m_persistConsumerOffsetInterval;
	std::string m_namesrvAddr;
	//mjx namesrv,onsaddr modify
	std::string m_onsAddr;
	
	std::string m_clientIP;
	std::string m_instanceName;
    std::string m_accessKey;
    std::string m_secretKey;
};

#endif
