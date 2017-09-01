#ifndef _MQ_FILE_UTIL_H_
#define _MQ_FILE_UTIL_H_

/** �ļ���������������ǰֻ֧�� LINUX����δ�ṩ WINDOWNS ƽ̨��ʵ�֡�
	@date 2016-9-18
	@author lin.qs
*/

#include <string>



class MQFileUtil
{
public:
	static const std::string PATH_SEPARATOR;

	/** ��ȡ�ļ���·�����֣��������ж��ļ��л��ļ��Ƿ���ڣ�
		e.g.
			"a/b/c" --> "a/b/"
			"a/b/"  --> "a/b/"
			"a/b"   --> "a/"
			"/"     --> "/"
			"a"     --> "a"
	*/
	static std::string GetDirPathSegment(const std::string& url);
	/** ��ȡ�ļ������֣��������ж��ļ��л��ļ��Ƿ���ڣ�
		e.g.
		  "a/b/c"	--> "c"
		  "a/b/"	--> ""
		  "a/b"		--> "b"
		  "/"		--> ""
		  "a"		--> "a"
	*/
	static std::string GetFileNameSegment(const std::string& url);
	/** ��װ����·��
		dirPath���ļ���·��
		filename���ļ�����������Ŀ¼����
		fileSuffix���ļ��Ķ����׺���Ǳ���
	*/
	static std::string CombinePath(const std::string& dirPath, const std::string& filename, const std::string& fileSuffix="");
	/** ��ȡ�����ļ����ڴ�
		@param fileContent [out] �����ȡ���ļ����ݡ�����������ݻ��ȱ����
		@param readBufSize	��ȡ�ļ�ʱ����ȡ�������С��Ĭ�� 4KB
		@param sizeLimit  ����ȡ��С��0��Ĭ�ϣ���ʾ�����ơ�����Ϊ�˰�ȫ������ڲ�ʵ��ǿ����һ��������ޣ�10M��
	*/
	static bool ReadWholeFile(std::string& fileContent, const std::string& filepath, int readBufSize =4096, size_t sizeLimit=0);
	
	//��ȫ��д���ļ���ʵ������д����ʱ�ļ������Ŀ���ļ��Ѵ��ڣ��򱸷�֮��Ȼ����ʱ�ļ�������Ϊԭ�ļ������֣�
	static bool SafeWriteFile(const std::string& content, const std::string& filepath, bool autoCreateDir=true, const std::string& bakFileSuffix=".bak", const std::string& tmpFileSuffix=".tmp");
private:
};

#endif
