/**
 * @file Date.h
 * @brief 基础框架日期功能类
 *
 * 作为实现各种日期和时间操作的函数集合。
 * 日期操作函数分为3大类别：基本功能，计算功能和格式转换功能
 * 基本功能包含：获取不同格式的当前系统时间，休眠，时间格式校验，时间区域判断，
 *               取得日期、时间、年、月、日、星期、时、分、秒等。
 * 计算功能包含：增加秒数，增加天数，增加月份，计算月份（天数、秒数）间隔等。
 * 格式转换功能包含：yyyymmddhhmiss、yyyy-mm-dd hh:mi:ss、time_t及int，这几种格式的相互转换
 *
 * @author zhang.fengli@zte.com.cn
 * @version 1.0
 * @date 2011/2/22
 * @bug  新建，无bug
 * @bug  单元测试，无bug
 * @warning 未做异常情况测试，使用时请注意入参
 */
#ifndef __C_DATE_H__
#define __C_DATE_H__
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

enum ENUM_DATE_TYPE
{
    DATE_TYPE_YEAR = 0,     ///<年
    DATE_TYPE_MONTH = 1,    ///<月
    DATE_TYPE_DAY = 2,      ///<日
    DATE_TYPE_HOUR = 3,     ///<小时
    DATE_TYPE_MINUTE = 4,   ///<分钟
    DATE_TYPE_SECOND = 5,   ///<秒
    DATE_TYPE_DATE = 6,     ///<日期
    DATE_TYPE_TIME = 7,     ///<时间
    DATE_TYPE_WEEK = 8      ///<星期
};

#define DATE_TIME_LEN       14
#define DATE_TIME_LONG_LEN  19
#define DATE_TIME_SHORT_LEN 8
#define DATE_TIME_SIZE      15

/**
 * @brief 日期类
 *
 * 日期处理功能函数集合
 *
 */
class CDate
{
public:
    /**
     * @brief 构造函数
     *
     * @param
     * @return
     * @retval
     */
    CDate();
    /**
     * @brief 析构函数
     *
     * @param
     * @return
     * @retval
     */
    ~CDate();

	
    /**
     * @brief 获取当前系统时间，格式time_t
     *
     * @return time_t
     * @retval 大于0 正确
     * @retval 其他 失败
     */
    static time_t GetSysDateTime();
	
    /**
     * @brief 获取当前系统时间，格式time_t
     *
     * @param tDateTime [out]  输出time_t
     * @return time_t
     * @retval 大于0 正确
     * @retval 其他 失败
     */
    static time_t GetSysDateTime(time_t &tDateTime);
    /**
     * @brief 获取当前系统时间，格式yyyymmddhh24miss或yyyy-mm-dd hh:mi:ss
     *
     * 当bLongFlag = false时，输出格式为yyyymmddhh24miss
     * 当bLongFlag = true时，输出格式为yyyy-mm-dd hh:mi:ss
     *
     * @param pszDateTime [out]  输出YYYYMMDDHH24MISS的字符串
     * @param bLongFlag [in] 是否输出长日期字符串yyyy-mm-dd hh:mi:ss格式
     * @param nFlag [in] 0输出日期时间，1输出日期，2输出时间
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int GetSysDateTime(char *pszDateTime,const bool bLongFlag=false,const int nFlag = 0);
    /**
     * @brief 休眠
     *
     * @param nTime [in]
     * @return
     * @retval
     */
    static void Sleep(const int nTime);
    /**
     * @brief 日期格式校验，校验格式为字符串yyyymmddhh24miss
     *
     * @param pszDateTime [in] 输入YYYYMMDDHH24MISS的字符串
     * @return time_t
     * @retval 大于0 正确
     * @retval 其他 失败
     */
    static time_t VerifyDateTime(const char *pszDateTime);
    /**
     * @brief 判断该字符串是否是 HHMMSS格式的时间型字符串
     *
     * @param pszTime [in] 源字符串
     * @return bool
     * @retval true    ：该字符串是HHMMSS格式的字符串
     * @retval false   ：该字符串不是HHMMSS格式的字符串
     */
    static bool VerifyTimeStr(const char * pszTime);
    /**
     * @brief 判断该字符串是否是 YYYYMMDD格式的日期型字符串
     *
     * @param pszDate [in] 源字符串
     * @return bool
     * @retval true    ：该字符串是YYYYMMDD格式的字符串
     * @retval false   ：该字符串不是YYYYMMDD格式的字符串
     */
    static bool VerifyDateStr(const char * pszDate);
    /**
     * @brief 判断指定日期，是否在两个日期之间
     *
     * 参数pszBeginDate，pszEndDate至少有一个不能为空，且pszInDate不能为空
     * 如果pszBeginDate==NULL，则比较pszInDate<=pszEndDate
     * 如果pszEndDate==NULL，则比较pszBeginDate<=pszInDate
     * 如果入参都不为空时，比较pszBeginDate<=pszInDate && pszInDate<=pszEndDate
     *
     * @param pszInDate [in] 输入YYYYMMDDHH24MISS的字符串
     * @param pszBeginDate [in] 输入YYYYMMDDHH24MISS的字符串
     * @param pszEndDate [in] 输入YYYYMMDDHH24MISS的字符串
     * @return bool
     * @retval true
     * @retval false
     */
    static bool BetweenDate(const char *pszInDate,const char *pszBeginDate,const char *pszEndDate);
    /**
     * @brief 取得日期字符串的星期
     *
     * 如果pszDateTime为空，nValue取当前系统时间的星期
     *
     * @param pszDateTime [in] 输入YYYYMMDD的日期
     * @return int
     * @retval 1-7 正确
     * @retval 其他 失败
     */
    static int GetWeek(const char *pszDateTime = NULL);
    /**
     * @brief 取得下n月的结束时间 结束日23:59:59
     *
     * 当nCount=0时，为当前月
     * 当nCount<0时，为之前|n|月
     * 当nCount>0时，为之后n月
     *
     * @param tSpecialTime [in]
     * @param ptMonthEnd [out]
     * @param nCount [in]
     * @return
     * @retval
     * @retval
     */
    static int GetMonthEndDTime(time_t tSpecialTime,time_t *ptMonthEnd,int nCount = 0);
    /**
     * @brief 取得下n个月的开始时间 1日00:00:00
     *
     * 当nCount=0时，为当前月
     * 当nCount<0时，为之前|n|月
     * 当nCount>0时，为之后n月
     *
     * @param tSpecialTime [in]
     * @param ptMonthBegin [out]
     * @param nCount [in]
     * @return
     * @retval
     * @retval
     */
    static int GetMonthBeginDTime(time_t tSpecialTime,time_t *ptMonthBegin,int nCount = 0);
    /**
     * @brief 判断是否为闰年
     *
     * @param pszYear [in]
     * @return bool
     * @retval true
     * @retval false
     */
    static bool IfLeapYear(char* pszYear);
    /**
     * @brief 取得年初第一天,输出日期格式yyyymmdd
     *
     * @param pszDate [in]
     * @param pszRetDate [out]
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int GetYearBegin(char *pszDate, char *pszRetDate);
    /**
     * @brief 取得年初第一天
     *
     * @param tDate [in]
     * @return time_t
     * @retval 大于0
     */
    static time_t GetYearBegin(time_t tDate);
    /**
     * @brief 取得年末最后一天,输出日期格式yyyymmdd
     *
     * @param pszDate [in]
     * @param pszRetDate [out]
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int GetYearEnd(char *pszDate, char *pszRetDate);
    /**
     * @brief 取得年末最后一天
     *
     * @param tDate [in]
     * @return time_t
     * @retval 大于0
     */
    static time_t GetYearEnd(time_t tDate);

    /**
     * @brief 增加秒数
     *
     * @param pszDateTime [in] 输入YYYYMMDDHH24MISS的字符串
     * @param nSeconds [in] 增加的秒数
     * @param pszRetValue [out] 输出YYYYMMDDHH24MISS的字符串
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int AddSeconds(const char *pszDateTime,const int nSeconds,char *pszRetValue);
    /**
     * @brief 增加天数
     *
     * @param pszDateTime [in] 输入YYYYMMDDHH24MISS的字符串
     * @param nDays [in] 增加的天数
     * @param pszRetValue [out] 输出YYYYMMDDHH24MISS的字符串
     * @param nRetLen [in] 返回字符串长度
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int AddDays(const char *pszDateTime,const int nDays,char *pszRetValue,int nRetLen);
    /**
     * @brief 增加月份
     *
     * @param nDateTime [in] 输入YYYYMM
     * @param nMonths [in] 增加的月份
     * @return int
     * @retval 大于0的月份值
     * @retval <=0 失败
     */
    static int AddMonths(const int nDateTime,const int nMonths);
    /**
    * @brief 增加月份
    *
    * 缺少对闰秒的考虑
    *
    * @param pszDateTime [in] 输入YYYYMMDDHH24MISS的字符串
    * @param nMonths [in] 增加的月份
    * @param pszRetValue [out] 输出YYYYMMDDHH24MISS的字符串
    * @param nRetLen [in] 返回字符串长度
    * @return int
    * @retval 0 正确
    * @retval 其他 失败
    */
    static int AddMonths(const char *pszDateTime,const int nMonths,char *pszRetValue,int nRetLen);
    /**
     * @brief 计算两个日期之间的月份间隔
     *
     * @param pszDateTime1 [in] 输入YYYYMMDDHH24MISS的字符串
     * @param pszDateTime2 [in] 输入YYYYMMDDHH24MISS的字符串
     * @param nMonths [out] 相差的月份值
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int IntervalMonth(const char *pszDateTime1, const char *pszDateTime2, int &nMonths);
    /**
     * @brief 计算两个时间的天数间隔
     *
     * @param tBeginTime [in]
     * @param tEndTime [in]
     * @param nDays [out] 相差的天数值
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int IntervalDay(const time_t tBeginTime,const time_t tEndTime, int &nDays);
    /**
     * @brief 计算两个时间的秒数间隔
     *
     * @param pszDateTime1 [in] 输入YYYYMMDDHH24MISS的字符串
     * @param pszDateTime2 [in] 输入YYYYMMDDHH24MISS的字符串
     * @param lSeconds [out] 相差的天数值
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int IntervalSecond(const char *pszDateTime1, const char *pszDateTime2, long &lSeconds);


    /**
     * @brief 格式转换，YYYYMMDDHHMISS->YYYY-MM-DD HH:MI:SS
     *
     * @param pszDateTime [in]
     * @param pszRetDateTime [out]
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int FormatDateTimeStoL(const char *pszDateTime, char *pszRetDateTime);
    /**
     * @brief 格式转换，YYYYMMDDHHMISS->int
     *
     * @param pszDateTime [in]
     * @param nRetValue [out]
     * @return int
     * @retval >0 正确
     * @retval -1 失败
     */
    static int FormatDateTimeStoI(const char *pszDateTime,int &nRetValue);
    /**
     * @brief 格式转换，YYYYMMDDHHMISS->time_t
     *
     * @param pszDateTime [in] 输入YYYYMMDDHHMISS
     * @param tRetDateTime [out] 输出time_t
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int FormatDateTimeStoT(const char *pszDateTime, time_t &tRetDateTime);
    /**
     * @brief 格式转换，YYYYMMDDHHMISS->time_t，时分秒取0
     *
     * @param pszDateTime [in] 输入YYYYMMDDHHMISS
     * @param tRetDateTime [out] 输出time_t
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int FormatStrToTime(const char *pszDateTime, time_t &tRetDateTime);

    /**
     * @brief 格式转换，time_t->YYYY-MM-DD HH:MI:SS
     *
     * @param tDateTime [in] 输入time_t
     * @param pszDateTime [out] 输出YYYY-MM-DD HH:MI:SS
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int FormatDateTimeTtoL(const time_t tDateTime,char *pszDateTime);
    /**
     * @brief 格式转换，time_t->YYYYMMDDHHMISS
     *
     * @param tDateTime [in] 输入time_t
     * @param pszDateTime [out] 输出YYYYMMDDHHMISS
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int FormatDateTimeTtoS(const time_t tDateTime,char *pszDateTime);

    /**
     * @brief 格式转换，YYYY-MM-DD HH:MI:SS->time_t
     *
     * @param pszDateTime [in] 输入YYYY-MM-DD HH:MI:SS
     * @param tRetDateTime [out] 输出time_t
     * @param bLongFlag [in] 是否长日期格式 为true时 pszDateTime格式为YYYY-MM-DD HH:MI:SS，为false时 pszDateTime格式为YYYY-MM-DD
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int FormatDateTimeLtoT(const char *pszDateTime, time_t &tRetDateTime,const bool bLongFlag=true);
    /**
     * @brief 格式转换，YYYY-MM-DD HH:MI:SS->YYYYMMDDHHMISS
     *
     * @param pszDateTime [in]
     * @param pszRetDateTime [out]
     * @param nDateTimeFlag [in] 为日期时间标识，为0时返回当前日期时间，为1返回当前日期，为2返回当前时间
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static int FormatDateTimeLtoS(const char *pszDateTime, char *pszRetDateTime, const int nDateTimeFlag=0);
    /**
     * @brief 格式转换，time_t->YYYYMMDDHHMISS
     *
     * @param tDateTime [in] 输入time_t
     * @param pszDateTime [out] 输出YYYYMMDDHHMISS
     * @return int
     * @retval 0 正确
     * @retval 其他 失败
     */
    static const char * FormatGMTimeToString(const time_t tTime,const bool bLongFlag=false);

protected:
private:
};
#endif ///<__C_DATE_H__

