#ifndef _MQ_FILE_UTIL_H_
#define _MQ_FILE_UTIL_H_

/** 文件辅助操作【！当前只支持 LINUX。尚未提供 WINDOWNS 平台的实现】
	@date 2016-9-18
	@author lin.qs
*/

#include <string>



class MQFileUtil
{
public:
	static const std::string PATH_SEPARATOR;

	/** 获取文件夹路径部分（不真正判断文件夹或文件是否存在）
		e.g.
			"a/b/c" --> "a/b/"
			"a/b/"  --> "a/b/"
			"a/b"   --> "a/"
			"/"     --> "/"
			"a"     --> "a"
	*/
	static std::string GetDirPathSegment(const std::string& url);
	/** 获取文件名部分（不真正判断文件夹或文件是否存在）
		e.g.
		  "a/b/c"	--> "c"
		  "a/b/"	--> ""
		  "a/b"		--> "b"
		  "/"		--> ""
		  "a"		--> "a"
	*/
	static std::string GetFileNameSegment(const std::string& url);
	/** 组装完整路径
		dirPath：文件夹路径
		filename：文件名，不包含目录部分
		fileSuffix：文件的额外后缀，非必须
	*/
	static std::string CombinePath(const std::string& dirPath, const std::string& filename, const std::string& fileSuffix="");
	/** 读取整个文件到内存
		@param fileContent [out] 保存读取的文件内容。如果已有数据会先被清空
		@param readBufSize	读取文件时，读取的区块大小，默认 4KB
		@param sizeLimit  最大读取大小。0（默认）表示不限制。但是为了安全起见，内部实现强制有一个最大上限（10M）
	*/
	static bool ReadWholeFile(std::string& fileContent, const std::string& filepath, int readBufSize =4096, size_t sizeLimit=0);
	
	//安全的写入文件（实际是先写入临时文件，如果目标文件已存在，则备份之；然后将临时文件重命名为原文件的名字）
	static bool SafeWriteFile(const std::string& content, const std::string& filepath, bool autoCreateDir=true, const std::string& bakFileSuffix=".bak", const std::string& tmpFileSuffix=".tmp");
private:
};

#endif
