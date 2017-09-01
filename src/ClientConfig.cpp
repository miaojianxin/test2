/**
 * Copyright (C) 2010-2013 kangliqiang, kangliq@163.com
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

#include <stdlib.h>
#include <sstream>

#include "SocketUtil.h"
#include "ClientConfig.h"
#include "UtilAll.h"
#include "MixAll.h"

ClientConfig::ClientConfig()
{
	char* addr = getenv(MixAll::NAMESRV_ADDR_ENV.c_str());
	if (addr)
	{
		m_namesrvAddr = addr;
	}
	else
	{
		m_namesrvAddr = "";
	}
	
	m_clientIP = getLocalAddress();
	m_instanceName = "DEFAULT";
	m_clientCallbackExecutorThreads = UtilAll::availableProcessors();
	m_pollNameServerInteval = 1000 * 30;
	m_heartbeatBrokerInterval = 1000 * 30;
	m_persistConsumerOffsetInterval = 1000 * 5;

	//mjx namesrv,onsaddr modify
	m_onsAddr  = "";
}

ClientConfig::~ClientConfig()
{
}

std::string ClientConfig::buildMQClientId()
{
	return m_clientIP+"@"+m_instanceName;
}

void ClientConfig::resetClientConfig(const ClientConfig& cc)
{
	m_namesrvAddr = cc.m_namesrvAddr;
	
	//mjx namesrv,onsaddr modify
	m_onsAddr = cc.m_onsAddr;
	
	m_clientIP = cc.m_clientIP;
	m_instanceName = cc.m_instanceName;
	m_clientCallbackExecutorThreads = cc.m_clientCallbackExecutorThreads;
	m_pollNameServerInteval = cc.m_pollNameServerInteval;
	m_heartbeatBrokerInterval = cc.m_heartbeatBrokerInterval;
	m_persistConsumerOffsetInterval = cc.m_persistConsumerOffsetInterval;
}

ClientConfig ClientConfig::cloneClientConfig()
{
	return *this;
}

std::string ClientConfig::getNamesrvAddr()
{
	return m_namesrvAddr;
}

void ClientConfig::setNamesrvAddr(const std::string& namesrvAddr)
{
	m_namesrvAddr = namesrvAddr;
}

//mjx namesrv,onsaddr modify

std::string ClientConfig::getNSAddr()
{
	return m_onsAddr;
}

void ClientConfig::setNSAddr(const std::string& onsAddr)
{
	m_onsAddr = onsAddr;
}


std::string ClientConfig::getClientIP()
{
	return m_clientIP;
}

void ClientConfig::setClientIP(const std::string& clientIP)
{
	m_clientIP = clientIP;
}

std::string ClientConfig::getInstanceName()
{
	return m_instanceName;
}

void ClientConfig::setInstanceName(const std::string& instanceName)
{
	m_instanceName = instanceName;
}

int ClientConfig::getClientCallbackExecutorThreads()
{
	return m_clientCallbackExecutorThreads;
}

void ClientConfig::setClientCallbackExecutorThreads(int clientCallbackExecutorThreads)
{
	m_clientCallbackExecutorThreads = clientCallbackExecutorThreads;
}

int ClientConfig::getPollNameServerInteval()
{
	return m_pollNameServerInteval;
}

void ClientConfig::setPollNameServerInteval(int pollNameServerInteval)
{
	m_pollNameServerInteval = pollNameServerInteval;
}

int ClientConfig::getHeartbeatBrokerInterval()
{
	return m_heartbeatBrokerInterval;
}

void ClientConfig::setHeartbeatBrokerInterval(int heartbeatBrokerInterval)
{
	m_heartbeatBrokerInterval = heartbeatBrokerInterval;
}

int ClientConfig:: getPersistConsumerOffsetInterval()
{
	return m_persistConsumerOffsetInterval;
}

void ClientConfig::setPersistConsumerOffsetInterval(int persistConsumerOffsetInterval)
{
	m_persistConsumerOffsetInterval = persistConsumerOffsetInterval;
}

int ClientConfig::getLogLevel()
{
    return UtilAll::GetLogLevel();
}

void ClientConfig::setLogLevel(int iLevel)
{
    UtilAll::SetLogLevel(iLevel);
}

void ClientConfig::changeInstanceNameToPID()
{
	if (m_instanceName == "DEFAULT") 
	{
		m_instanceName = UtilAll::getPidStr();
	}
}


//mjx test modify
//跟封装的阿里mq保持一致,在zmq里面无意义
std::string ClientConfig::getAccessKey()
{
    return m_accessKey;
}

void ClientConfig::setAccessKey(const std::string& accessKey)
{
    m_accessKey = accessKey;
}

std::string ClientConfig::getSecretKey()
{
    return m_secretKey;
}

void ClientConfig::setSecretKey(const std::string& secretKey)
{
    m_secretKey = secretKey;
}

