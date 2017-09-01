/**
 * @file DateInterface.h
 * 
 * 
 * 
 * @author zhang.fengli@zte.com.cn
 * @version 1.0
 * @date 2011/03/21
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
 * @warning 接口函数为非线程安全，请注意使用
 */
#ifndef __DATE_INTERFACE_H__
#define __DATE_INTERFACE_H__

#include "Date.h"
/**
 * @brief 取两日期间间隔的月份数
 * 
 * @param psztTime1 [in] 
 * @param psztTime2 [in]
 * @return int
 * @retval 相差月份值
 * @retval -1 失败
 */
int	IntervalMonth(char* psztTime1, char* psztTime2);
/**
 * @brief 取两日期间间隔的天数
 * 
 * @param psztTime1 [in] 
 * @param psztTime2 [in]
 * @return bool
 * @retval true
 * @retval false
 */
bool IntervalDay(const time_t tmBeginTime,const time_t tmEndTime,int &nDayCnt);
/**
 * @brief 将日期字符串格式转换成int
 * 
 * @param szAnswerTime [in] 
 * @param  [out]
 * @return int
 * @retval 日期值
 * @retval -1
 */
int iGetTime(char szAnswerTime[15]);
/**
 * @brief 给字符串szTime加nSeconds秒
 * 
 * @param szTime [in/out] 
 * @param nSeconds [in]
 * @return int
 * @retval 0
 */
int AddSeconds(char szTime[],int nSeconds);
/**
 * @brief 将日期字符串yyyymmddhh24miss转换成time_t格式
 * 
 * @param szAnswerTime [in] 
 * @param  [out]
 * @return time_t
 * @retval 时间time_t
 */
time_t tGetTime(const char szAnswerTime[15]);
/**
 * @brief 将日期字符串yyyy-mm-dd hh:mm:ss转换成time_t格式
 * 
 * @param szAnswerTime [in] 
 * @param  [out]
 * @return time_t
 * @retval 时间time_t
 * @retval 
 */
time_t tGetInfTime (const char szAnswerTime[20]);
/**
 * @brief 将time_t格式转换成日期字符串yyyymmddhh24miss
 * 
 * @param tTime [in] 
 * @param  [out]
 * @return char*
 * @retval 字符串日期
 * @retval 
 */
char * Time2Asc(time_t tTime);
/**
 * @brief 将time_t格式转换成日期字符串yyyy-mm-dd hh:mm:ss
 * 
 * @param tTime [in] 
 * @param  [out]
 * @return char*
 * @retval 字符串日期
 * @retval 
 */
char * Time2InfAsc(time_t tTime);
/**
 * @brief 计算两字符串日期之间的天数差值
 * 
 * @param szEndDate [in] 
 * @param szBeginDate [in]
 * @return long
 * @retval 天数差值
 * @retval 
 */
long DiffDays(char *szEndDate,char *szBeginDate);
/**
 * @brief 将日期字符串转换成time_t格式，时分秒取0
 * 
 * @param pszBeginDate [in] 
 * @param  [out]
 * @return time_t
 * @retval time_t日期值
 * @retval 
 */
time_t tGetDate_t(char *pszBeginDate);
/**
 * @brief 获取日期字符串的星期值
 * 
 * @param szDate [in] 
 * @param  [out]
 * @return int
 * @retval 1-7的星期值
 * @retval -1
 */
int ToWeek(char szDate[]);
/**
 * @brief 获取系统时间，格式yyyymmddhh24miss
 * 
 * @param szSysDate [in/out] 
 * @param  [out]
 * @return int
 * @retval 1
 */
int GetSysDate(char szSysDate[]);
/**
 * @brief 获取系统时间，格式yyyy-mm-dd hh:mm:ss
 * 
 * @param szSysDate [in/out] 
 * @param  [out]
 * @return int
 * @retval 1
 */
int GetSysDateStr(char szSysDate[]);
/**
 * @brief 给日期增加nDay天
 * 
 * @param szDate [in/out] 
 * @param nDay [in]
 * @return int
 * @retval 1
 * @retval 
 */
int AddDay(char szDate[],int nDay);
/**
 * @brief 给日期增加1天
 * 
 * @param szDate [in/out] 
 * @return int
 * @retval 1
 * @retval 
 */
int AddDate(char szDate[]);
/**
 * @brief 给日期增加-1天
 * 
 * @param szDate [in/out]
 * @return int
 * @retval 1
 * @retval 
 */
int SubDate(char szDate[15]);
/**
 * @brief 将yyyymmddhh24miss格式转换成日期字符串yyyy-mm-dd hh:mm:ss
 * 
 * @param szDateTime [in] 
 * @return char*
 * @retval 字符串
 * @retval 
 */
char * GetFormatDateTime(char *szDateTime);
/**
 * @brief 判断日期pszInDate是否在区间pszBeginDate，pszEndDate之间
 * 
 * @param pszInDate [in] 
 * @param pszBeginDate [in] 
 * @param pszEndDate [in]
 * @return bool
 * @retval true
 * @retval false
 */
bool BetweenDate(const char *pszInDate,const char *pszBeginDate,const char *pszEndDate);
/**
 * @brief 获取当前系统时间
 *
 * 根据nDateTimeFlag值返回不同的字符串，0：yyyy-mm-dd hh:mm:ss，1：yyyy-mm-dd；2：hh:mm:ss
 * 
 * @param szDateTime [out] 
 * @param nDateTimeFlag [in]
 * @return void
 * @retval 
 * @retval 
 */
void GetNowDateTime(char *szDateTime,int nDateTimeFlag);
/**
 * @brief 格式转换，YYYY-MM-DD HH:MI:SS->YYYYMMDDHHMISS
 *
 * @param szSrcTime [in] 
 * @param szDestTime [out] 
 * @return void
 * @retval 
 * @retval 
 */
void ConvertTimeText2ToTimeText1(char *szSrcTime,char *szDestTime);
/**
 * @brief 格式转换，YYYYMMDDHHMISS->YYYY-MM-DD HH:MI:SS
 *
 * @param szSrcTime [in] 
 * @param szDestTime [out] 
 * @return void
 * @retval 
 * @retval 
 */
void ConvertTimeText1ToTimeText2(char *szSrcTime,char *szDestTime);
/**
 * @brief 格式转换，YYYYMMDDHHMISS->time_t
 *
 * @param szSrcTime [in] 
 * @param ptDate [out] 
 * @return void
 * @retval 
 * @retval 
 */
void ConvStrToTimet(const char* pszTimeStr,time_t* ptDate);
/**
 * @brief 格式转换，time_t->YYYY-MM-DD HH:MI:SS
 *
 * @param tTime [in] 
 * @param pszTime [out] 
 * @return void
 * @retval 
 * @retval 
 */
void Getstime(time_t tTime, char *pszTime);
/**
 * @brief 格式转换，YYYY-MM-DD HH:MI:SS->time_t
 *
 * @param szSrcTime [in] 
 * @param ptTime [out] 
 * @return void
 * @retval 
 * @retval 
 */
void GetTimeOfTimet(char * pszStrTime,time_t *ptTime);
/**
 * @brief 日期格式校验，校验格式为字符串yyyymmddhh24miss
 *
 * @param pszDateTime [in] 输入YYYYMMDDHH24MISS的字符串
 * @return time_t
 * @retval 大于0 正确
 * @retval 其他 失败
 */
time_t VerifyDateTime(const char* pszDateTime);
/**
 * @brief 格式转换，time_t->YYYYMMDDHHMISS
 *
 * @param tTime [in] 
 * @param pszDestVaue [out] 
 * @return char*
 * @retval pszDateTime
 * @retval 
 */
char* TimetToStr(char* pszDestVaue,time_t tTime);
/**
 * @brief 格式转换，time_t->YYYYMMDDHHMISS
 *
 * @param tTime [in] 
 * @return char*
 * @retval 字符串格式的日期
 * @retval 
 */
char* TimetToStr(time_t tTime);
/**
 * @brief 为tTime增加1个月
 *
 * @param tTime [in] 
 * @return time_t
 * @retval time_t格式日期
 * @retval 
 */
time_t AddMonth(time_t tTime);
/**
 * @brief 判断是否闰年
 *
 * @param szYear [in] 
 * @return bool
 * @retval true
 * @retval false
 */
bool IsLeapYear(char szYear[5]);
/**
 * @brief 为字符串增加1天
 *
 * @param szDate [in/out] 
 * @return int
 * @retval 0
 * @retval 
 */
int AddOneDay(char szDate[15]);
/**
 * @brief 为字符串减去1天
 *
 * @param szDate [in/out] 
 * @return int
 * @retval 0
 * @retval 
 */
int SubOneDay(char szDate[15]);
/**
 * @brief 取得日期的对应月份的最后一天
 *
 * @param szStrEndTime [in/out] 
 * @return void
 * @retval 
 * @retval 
 */
void GetMonthEndTime(char *szStrEndTime);

/**
 * @brief 类名
 *
 * 类说明
 * 
 */
class TDateTimeFunc {
public:
    static time_t StringToTime(const char szTime[]);

    static const char * GetCurrentTimeStr(char sTime[]=NULL,size_t iLen=0,const bool bLongFlag=false);

    static const char * GetTimeAddSeconds(const char sTime[],long iSeconds,bool bSysFlag=true);

    static void Sleep(const int iSeconds);

    static const char * TimeToString(const time_t tTime,const bool bLongFlag=false);

    static const char * GMTimeToString(const time_t tTime,const bool bLongFlag=false);

    static long GetDiffTime(const char szTime1[], const char szTime2[]);

    static long GetDiffDay(const char szTime1[], const char szTime2[]);

    static void AddDay(char szDate[],long lDays);

    static void AddDate(char szDate[]);

    static void SubDate(char szDate[]);

    static int  GetWeek(const char szDate[]);

private:
};

#endif//__DATE_INTERFACE_H__



