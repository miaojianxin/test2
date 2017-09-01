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
#error ���ӿ�Ŀǰֻ֧�� LINUX ƽ̨
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
#error ���ӿ�Ŀǰֻ֧�� LINUX ƽ̨
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

	//����ʵ�ʴ�С����
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
		MqLogWarn("���ļ�[%s]ʧ��, errno=%d, errmsg=%s", filepath.c_str(), errno, strerror(errno));
		return false;
	}


	fileContent.clear();
	char* readBuf = new char[readBufSize + 1];
	size_t readCount = 0;		//���ζ�������ݸ���
	size_t couldAddCount = 0;	//���ζ���ʱ��ʵ�ʿɱ��浽 fileContent �е����ݸ�ʽ��Ҫ���� sizeLimit��readCount �� S_MAX_FILE_SIZE)
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
#error ���ӿ�Ŀǰֻ֧�� LINUX ƽ̨
#else

	std::string dirPath = GetDirPathSegment(filepath);
	std::string filename = GetFileNameSegment(filepath);
	if (dirPath.empty() || filename.empty())
	{
		MqLogWarn("�ļ����ļ���·������[%s]���ļ�������[%s]Ϊ��", dirPath.c_str(), filename.c_str());
		return false;
	}

	//�ļ���·����dirPath��������������ڣ����ݲ���ȷ���Ƿ��Զ������ļ���
	struct stat dirStat;
	if (-1 == stat(dirPath.c_str(), &dirStat))
	{
		if (autoCreateDir)
		{
			system(std::string("mkdir -p ").append(dirPath).c_str());
		}
		else
		{
			MqLogWarn("��ȡ�ļ���[%s]״̬ʧ��, errno:%d, errmsg:%s", filepath.c_str(), errno, strerror(errno));
			return false;
		}
	}

	memset(&dirStat, 0x00, sizeof(dirStat));
	if (-1 == stat(dirPath.c_str(), &dirStat))
	{
		MqLogWarn("�ļ���[%s]����ʧ��", dirPath.c_str());
		return false;
	}

	//ԭ�ļ�����filepath ָ�����ļ����������ڲ��ܣ����ڱ���֮
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

	//����д����ʱ�ļ������д��ɹ�����������������ڣ�����ԭ�ļ���
	std::string tmpFilepath = CombinePath(dirPath, filename, tmpFileSuffix);
	FILE* fp = fopen(tmpFilepath.c_str(), "wb");
	if (NULL == fp)
	{
		MqLogWarn("��ʱ�ļ�[%s]��ʧ��", tmpFilepath.c_str());
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
