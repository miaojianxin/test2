#include "UtilAll.h"

#ifdef WIN32
#   include <sys/timeb.h>
#   include <process.h>
#else
#   include <unistd.h>
#   include <sys/types.h>
#   include <sys/time.h>
#endif


extern int G_MQLOGLEVEL = getenv("MQLOGLEVEL") ? atoi(getenv("MQLOGLEVEL")) : MQLOG_NOTICE;

int UtilAll::Split(std::vector<std::string>& out, const std::string & in, const std::string & delimiter)
{
	std::string::size_type left = 0;
	while (true)
	{
		std::string::size_type right = in.find(delimiter, left);

		/* modified by yu.guangjie at 2015-08-18, reason: = to == */
		if (right == std::string::npos)
		{
			break;
		}

		out.push_back(in.substr(left, right - left));

		left = right + delimiter.length();
	}

	out.push_back(in.substr(left));

	return out.size();
}

int UtilAll::Split(std::vector<std::string>& out, const std::string & in, const char delimiter)
{
	std::string::size_type left = 0;
	while (true)
	{
		std::string::size_type right = in.find(delimiter, left);

		/* modified by yu.guangjie at 2015-08-18, reason: = to == */
		if (right == std::string::npos)
		{
			break;
		}

		out.push_back(in.substr(left, right - left));

		left = right + 1;
	}

	out.push_back(in.substr(left));

	return out.size();
}

std::string UtilAll::Trim(const std::string & str)
{
	if (str.empty())
	{
		return str;
	}

	std::string::size_type left = str.find_first_not_of(WHITESPACE);

	if (left == std::string::npos)
	{
		return "";
	}

	std::string::size_type right = str.find_last_not_of(WHITESPACE);

	if (right == std::string::npos)
	{
		return str.substr(left);
	}

	/* modified by yu.guangjie at 2015-08-18, reason: */
	return str.substr(left, right - left + 1);
}

bool UtilAll::isBlank(const std::string & str)
{
	if (str.empty())
	{
		return true;
	}

	std::string::size_type left = str.find_first_not_of(WHITESPACE);

	if (left == std::string::npos)
	{
		return true;
	}

	return false;
}

bool UtilAll::compress(const char * pIn, int inLen, unsigned char ** pOut, int * pOutLen, int level)
{
	int ret, flush;
	int have;
	z_stream strm;

	unsigned char out[CHUNK];

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, level);
	if (ret != Z_OK)
	{
		return false;
	}

	int outBufferLen = inLen;
	unsigned char* outData = (unsigned char*)malloc(outBufferLen);


	int left = inLen;
	int used = 0;
	int outDataLen = 0;
	int outPos = 0;

	/* compress until end of buffer */
	do
	{
		strm.avail_in = left > CHUNK ? CHUNK : left;
		flush = left <= CHUNK ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = (unsigned char*)pIn + used;
		used += strm.avail_in;
		left -= strm.avail_in;

		/* run deflate() on input until output buffer not full, finish
		compression if all of source has been read in */
		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    /* no bad return value */
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;

			if (outDataLen + have > outBufferLen)
			{
				outBufferLen = outDataLen + have;
				outBufferLen <<= 1;
				unsigned char* tmp = (unsigned char*)realloc(outData, outBufferLen);
				if (!tmp)
				{
					free(outData);
					return false;
				}

				outData = tmp;
			}

			memcpy(outData + outDataLen, out, have);
			outDataLen += have;

		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);     /* all input will be used */

										/* done when last data in file processed */
	} while (flush != Z_FINISH);
	assert(ret == Z_STREAM_END);        /* stream will be complete */

	*pOutLen = outDataLen;
	*pOut = outData;

	/* clean up and return */
	(void)deflateEnd(&strm);
	return true;
}

bool UtilAll::decompress(const char * pIn, int inLen, unsigned char ** pOut, int * pOutLen)
{
	int ret;
	int have;
	z_stream strm;

	unsigned char out[CHUNK];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
	{
		return false;
	}

	int outBufferLen = inLen << 2;
	unsigned char* outData = (unsigned char*)malloc(outBufferLen);

	int left = inLen;
	int used = 0;
	int outDataLen = 0;
	int outPos = 0;

	/* decompress until deflate stream ends or end of buffer */
	do
	{
		strm.avail_in = left > CHUNK ? CHUNK : left;
		if (strm.avail_in <= 0)
		{
			break;
		}

		strm.next_in = (unsigned char*)pIn + used;
		used += strm.avail_in;
		left -= strm.avail_in;

		/* run inflate() on input until output buffer not full */
		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret)
			{
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				free(outData);
				return false;
			}
			have = CHUNK - strm.avail_out;

			if (outDataLen + have > outBufferLen)
			{
				outBufferLen = outDataLen + have;
				outBufferLen <<= 1;
				unsigned char* tmp = (unsigned char*)realloc(outData, outBufferLen);
				if (!tmp)
				{
					free(outData);
					return false;
				}

				outData = tmp;
			}

			memcpy(outData + outDataLen, out, have);
			outDataLen += have;

		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);

	if (ret == Z_STREAM_END)
	{
		*pOutLen = outDataLen;
		*pOut = outData;

		return true;
	}
	else
	{
		free(outData);

		return false;
	}
}


unsigned int GetCurrentTimeMillSecond()
{
#ifdef WIN32
	timeb tb;
	ftime(&tb);
	return tb.millitm;
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_usec/1000;
#endif
}

void UtilAll::mqLogRaw(int level, const char * msg)
{
	const char *lvl[] = { "DEBUG","VERBOSE","NOTICE","WARNING","ERROR" };
	FILE *fp = stdout;
	char buf[32];

	if (level > MQLOG_ERROR)
		level = MQLOG_ERROR;

	struct tm tmNow;
	time_t tNow = time(NULL);
	//strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",LOCALTIME_R(&tNow, &tmNow));
	LOCALTIME_R(&tNow, &tmNow);
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%3d",
		tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
		tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec, GetCurrentTimeMillSecond());

	fprintf(fp, "%s <%s> %s\n", buf, lvl[level], msg);
	fflush(fp);
}

const char * UtilAll::GetFileName(const char * sFullFileName)
{
#if defined _WIN32 || defined WIN32 || defined __WIN32__
	const char *p = strrchr(sFullFileName, '\\');
#else
	char *p = (char *)strrchr(sFullFileName, '/');
#endif

	if (p != NULL)//found
	{
		++p;
	}
	else // not found
	{
		p = (char*)sFullFileName;
	}

	return p;
}

void UtilAll::SetLogLevel(int iLevel)
{
	if (iLevel < MQLOG_DEBUG)
	{
		G_MQLOGLEVEL = MQLOG_DEBUG;
	}
	else if (iLevel > MQLOG_WARNING)
	{
		G_MQLOGLEVEL = MQLOG_WARNING;
	}
	else
	{
		G_MQLOGLEVEL = iLevel;
	}
}

std::string UtilAll::getPidStr()
{
#ifdef WIN32
	throw "Windows 平台还没有实现 UtilAll::getPidStr 方法";
#else
	char buf[32];
	memset(buf, 0x00, sizeof(buf));

	pid_t pid = getpid();
	if (sizeof(pid) <= sizeof(int))
	{
		snprintf(buf, sizeof(buf), "%d", pid);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%lld", pid);
	}
	return std::string(buf);
#endif
}
