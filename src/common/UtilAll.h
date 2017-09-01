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
#if!defined __UTILALL_H__
#define __UTILALL_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <string>
#include <vector>

#include "zlib.h"

typedef enum
{
	MQLOG_DEBUG = 0,
	MQLOG_VERBOSE,
	MQLOG_NOTICE,
	MQLOG_WARNING,
	MQLOG_ERROR
} MQLogLevel;

//mdy by lin.qiongshan, 2016年8月30日11:38:02, G_MQLOGLEVEL 不应该定义在头文件中
//	这回导致所有引用 UtilAll.h 的 cpp 文件定义了一个 static 的变量 G_MQLOGLEVEL，每个 cpp 文件中 G_MQLOGLEVEL 变量都不相同，导致其它地方调用 UtilAll::setLogLevel 或 ClientConfig::setLogLevel 不是对整体生效
//  新增 UtilAll.cpp ，定义实现，全局变量等（常量仍可放在头文件中）
//static int G_MQLOGLEVEL = getenv("MQLOGLEVEL")?atoi(getenv("MQLOGLEVEL")) : MQLOG_NOTICE;
extern int G_MQLOGLEVEL;
    
const std::string WHITESPACE=" \t\r\n";
const int CHUNK = 8192;

/**
* 各种方法大杂烩
*
*/
class UtilAll
{
public:
	static int Split(std::vector<std::string>& out, const std::string& in, const std::string& delimiter);
	
	static int Split(std::vector<std::string>& out, const std::string& in, const char delimiter);

	static std::string Trim(const std::string& str);

	static bool isBlank(const std::string& str);

	static int availableProcessors()
	{
		return 4;
	}

	static int hashCode(const char* pData, int len)
	{
	    int h = 0;

        for (int i = 0; i < len; i++) {
            h = 31*h + pData[i];
        }
		return h;
	}

	static bool compress(const char* pIn, int inLen, unsigned char** pOut, int* pOutLen, int level);

	static bool decompress(const char* pIn, int inLen, unsigned char** pOut, int* pOutLen);

	static unsigned long long hexstr2ull(const char* str)
	{
		char* end;
#ifdef WIN32
		return _strtoui64(str,&end,16);
#else
		return strtoull(str,&end,16);
#endif
	}

#ifdef WIN32
#define LOCALTIME_R(ti,tm)  localtime_s(tm,ti)
#else
#define LOCALTIME_R(ti,tm)  localtime_r(ti,tm)
#endif
    /* modified by yu.guangjie at 2015-08-17, reason: add begin*/

	static void mqLogRaw(int level, const char *msg);

    //去掉全路径的文件名的路径信息
	static const char* GetFileName(const char* sFullFileName);

	static void SetLogLevel(int iLevel);

    static int GetLogLevel()
    {
        return G_MQLOGLEVEL;
    }

	//获取当前进程pid的字符串表述。目前只支持 Linux，Windows 平台将返回抛出异常
	static std::string getPidStr();
public:

};

#ifdef WIN32
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

//lin.qiongshan, 2016年8月18日14:04:47, 原先的版本中，文件名和行号信息只在日志级别大于等于 MQLOG_WARNING 是才显式
//	为了便于查看日志信息，此处先改为所有日志级别都输出文件名和行号
#define ADD_MQLOG(__LogLvl, ...)  \
    do  \
    {   \
        if (__LogLvl >= G_MQLOGLEVEL)   \
        {   \
        	char szTmp[1024] = {0}; \
        	int iPos = 0; \
        	if(__LogLvl >= MQLOG_DEBUG) \
        	    iPos = sprintf(szTmp,"<%s:%d> ",UtilAll::GetFileName(__FILE__),__LINE__);\
		    SNPRINTF(szTmp+iPos, 1024-iPos, __VA_ARGS__);  \
			szTmp[sizeof(szTmp)-1]=0; \
            UtilAll::mqLogRaw(__LogLvl, szTmp);   \
        }   \
    }   \
    while(0)

#define MqLogError(...)     ADD_MQLOG(MQLOG_ERROR, __VA_ARGS__)
#define MqLogWarn(...)      ADD_MQLOG(MQLOG_WARNING, __VA_ARGS__)
#define MqLogNotice(...)    ADD_MQLOG(MQLOG_NOTICE, __VA_ARGS__)
#define MqLogVerb(...)      ADD_MQLOG(MQLOG_VERBOSE, __VA_ARGS__)
#define MqLogDebug(...)     ADD_MQLOG(MQLOG_DEBUG, __VA_ARGS__)


#endif
