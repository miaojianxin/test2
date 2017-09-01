#include <errno.h>


#ifdef WIN32
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "MQFileUtil.h"
#include "UtilAll.h"


#ifdef WIN32
const std::string MQFileUtil::PATH_SEPARATOR = "\\";
#else
const std::string MQFileUtil::PATH_SEPARATOR = "/";
#endif

std::string MQFileUtil::GetDirPathSegment(const std::string & url)
{
#ifdef WIN32
#error 本接口目前只支持 LINUX 平台
#else
	std::string::size_type pos = url.rfind(PATH_SEPARATOR);
	if (std::string::npos == pos)
	{
		return url;
	}
	else
	{
		return url.substr(0, pos + 1);
	}
#endif
}

std::string MQFileUtil::GetFileNameSegment(const std::string & url)
{
#ifdef WIN32
#error 本接口目前只支持 LINUX 平台
#else
	std::string::size_type pos = url.rfind(PATH_SEPARATOR);
	if (std::string::npos == pos)
	{
		return url;
	}
	else
	{
		return url.substr(pos + 1);
	}
#endif
}

std::string MQFileUtil::CombinePath(const std::string & dirPath, const std::string & filename, const std::string & fileSuffix)
{
	std::string path = dirPath.substr(0, dirPath.rfind(PATH_SEPARATOR));
	path
		.append(PATH_SEPARATOR)
		.append(filename)
		.append(fileSuffix);
	return path;
}

bool MQFileUtil::ReadWholeFile(std::string& fileContent, const std::string & filepath, int readBufSize, size_t sizeLimit)
{
	static const size_t S_MAX_FILE_SIZE = 10*1024*1024;

	//计算实际大小限制
	size_t realSizeLimit = S_MAX_FILE_SIZE;
	if (sizeLimit <= 0)
	{
		//nothing to do
	}
	else
	{
		if (sizeLimit < realSizeLimit)
		{
			realSizeLimit = sizeLimit;
		}
	}

	FILE* fp = fopen(filepath.c_str(), "rb");
	if (NULL == fp)
	{
		MqLogWarn("打开文件[%s]失败, errno=%d, errmsg=%s", filepath.c_str(), errno, strerror(errno));
		return false;
	}


	fileContent.clear();
	char* readBuf = new char[readBufSize + 1];
	size_t readCount = 0;		//单次读入的数据个数
	size_t couldAddCount = 0;	//单次读入时，实际可保存到 fileContent 中的数据格式（要考虑 sizeLimit，readCount 和 S_MAX_FILE_SIZE)
	while (!feof(fp))
	{
		memset((void*)readBuf, 0x00, readBufSize + 1);
		readCount = fread((void*)readBuf, sizeof(char), readBufSize, fp);

		couldAddCount = readCount <= (realSizeLimit - fileContent.size()) ? readCount : (realSizeLimit - fileContent.size());
		
		if (couldAddCount <= 0)
		{
			break;
		}
		else
		{
			fileContent.append(readBuf, couldAddCount);
		}
	}

	if (NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return true;
}

bool MQFileUtil::SafeWriteFile(const std::string & content, const std::string& filepath, bool autoCreateDir, const std::string& bakFileSuffix, const std::string& tmpFileSuffix)
{
#ifdef WIN32
#error 本接口目前只支持 LINUX 平台
#else

	std::string dirPath = GetDirPathSegment(filepath);
	std::string filename = GetFileNameSegment(filepath);
	if (dirPath.empty() || filename.empty())
	{
		MqLogWarn("文件的文件夹路径部分[%s]或文件名部分[%s]为空", dirPath.c_str(), filename.c_str());
		return false;
	}

	//文件夹路径（dirPath）处理：如果不存在，根据参数确定是否自动创建文件夹
	struct stat dirStat;
	if (-1 == stat(dirPath.c_str(), &dirStat))
	{
		if (autoCreateDir)
		{
			system(std::string("mkdir -p ").append(dirPath).c_str());
		}
		else
		{
			MqLogWarn("获取文件夹[%s]状态失败, errno:%d, errmsg:%s", filepath.c_str(), errno, strerror(errno));
			return false;
		}
	}

	memset(&dirStat, 0x00, sizeof(dirStat));
	if (-1 == stat(dirPath.c_str(), &dirStat))
	{
		MqLogWarn("文件夹[%s]创建失败", dirPath.c_str());
		return false;
	}

	//原文件处理（filepath 指定的文件）：不存在不管；存在备份之
	struct stat fileStat;
	if (-1 == stat(filepath.c_str(), &fileStat))
	{
		//nothing to do
	}
	else
	{
		std::string bakFilepath = CombinePath(dirPath, filename, bakFileSuffix);
		system(std::string("cp ").append(filepath).append(" ").append(bakFilepath).c_str());
	}

	//数据写入临时文件。如果写入成功，重命名（如果存在，覆盖原文件）
	std::string tmpFilepath = CombinePath(dirPath, filename, tmpFileSuffix);
	FILE* fp = fopen(tmpFilepath.c_str(), "wb");
	if (NULL == fp)
	{
		MqLogWarn("临时文件[%s]打开失败", tmpFilepath.c_str());
		return false;
	}
	else
	{
		fwrite(content.data(), sizeof(char), content.size(), fp);
		fclose(fp);
		fp = NULL;

		system(std::string("mv ").append(tmpFilepath).append(" ").append(filepath).c_str());
	}

	return true;
#endif
}
