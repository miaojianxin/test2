#include <curl/curl.h>
#include <stdexcept>

#include "TopAddressing.h"
#include "UtilAll.h"

const std::string TopAddressing::DEFAULT_WS_ADDR = "http://nameserver.ztesoft:8080/getNsAddr";
const std::string TopAddressing::DEFAULT_LOCAL_WS_ADDR = "http://127.0.0.1:8080/getNsAddr";

//note by lin.qiongshan@2017-6-16, 多线程下使用 curl 好像会有问题，因此使用 curl 寻址服务默认关闭，通过特定宏（_ENABLE_CURL_TOPADDRESSING）才可以开启

class CurlException : public std::runtime_error
{
public:
	CurlException(const char* msg):
		std::runtime_error(msg)
	{}
};

//libcurl 的全局初始化方法（curl_global_init）只能调用一次，通过该全局变量标记是否初始化过，避免重复初始化
static bool g_isCurlGlobalInited = false;

struct CurlWriteData
{
	std::string data;
};

size_t curlWriteFunction(void* buffer, size_t size, size_t nmemb, void *data)
{
	CurlWriteData* pData = (CurlWriteData*)data;
	pData->data.append((const char*)buffer, size*nmemb);
	return size*nmemb;
}

TopAddressing::TopAddressing()
{
#ifdef _ENABLE_CURL_TOPADDRESSING
	if (!g_isCurlGlobalInited)
	{
		CURLcode code = curl_global_init(CURL_GLOBAL_NOTHING);
		if (0 != code)
		{
			MqLogWarn("curl global init failed, ret=%d", code);
			throw CurlException("curl global init failed");
		}
		g_isCurlGlobalInited = true;
	}
#endif
}

TopAddressing::~TopAddressing()
{
#ifdef _ENABLE_CURL_TOPADDRESSING
	if (g_isCurlGlobalInited)
	{
		curl_global_cleanup();
	}
#endif
}

std::string TopAddressing::fetchNSAddr()
{
	int iRet = 0;
#ifdef _ENABLE_CURL_TOPADDRESSING
	do {
		MqLogVerb("try to fetch nameserver addr from web server[%s, %s, %s]",
			m_wsAddr.c_str(),
			DEFAULT_WS_ADDR.data(),
			DEFAULT_LOCAL_WS_ADDR.c_str());

		if (!m_wsAddr.empty())
		{
			iRet = request(m_wsAddr, m_nsAddr);
			if (0 == iRet)
			{
				break;
			}
		}

		iRet = request(DEFAULT_WS_ADDR, m_nsAddr);
		if (0 == iRet)
		{
			break;
		}

		iRet = request(DEFAULT_LOCAL_WS_ADDR, m_nsAddr);
		if (0 == iRet)
		{
			break;
		}
	} while (false);
	
	if (0 != iRet)
	{
		m_nsAddr.clear();
		MqLogWarn("get namesrv addr failed, code=%d", iRet);
	}
	else
	{
		MqLogVerb("fetch namesrv addr succeed, addr is %s", m_nsAddr.c_str());
	}
#endif
	return m_nsAddr;
}

int TopAddressing::request(const std::string & url, std::string & response)
{
	int iRet = 0;
#ifdef _ENABLE_CURL_TOPADDRESSING
	CURL* curl = curl_easy_init();
	if (NULL == curl)
	{
		MqLogWarn("curl_easy_init failed");
		return -1;
	}

	try
	{
		CurlWriteData curlData;	///< curl 执行结果数据

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlData);

		CURLcode code = curl_easy_perform(curl);
		if (0 == code)
		{
			//TODO: translate the http response to nameserver addr
			response = UtilAll::Trim(curlData.data);
		}
		else
		{
			iRet = code;
		}

	}
	catch (std::exception& e)
	{
		MqLogWarn("exception, msg=%s", e.what());
		iRet = -1;
	}
	catch (...)
	{
		MqLogWarn("unknown exception");
		iRet = -1;
	}

	if (NULL != curl)
	{
		curl_easy_cleanup(curl);
		curl = NULL;
	}
#endif
	return iRet;
}
