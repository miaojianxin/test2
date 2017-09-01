/**
 * @file Date.h
 * @brief ����������ڹ�����
 *
 * ��Ϊʵ�ָ������ں�ʱ������ĺ������ϡ�
 * ���ڲ���������Ϊ3����𣺻������ܣ����㹦�ܺ͸�ʽת������
 * �������ܰ�������ȡ��ͬ��ʽ�ĵ�ǰϵͳʱ�䣬���ߣ�ʱ���ʽУ�飬ʱ�������жϣ�
 *               ȡ�����ڡ�ʱ�䡢�ꡢ�¡��ա����ڡ�ʱ���֡���ȡ�
 * ���㹦�ܰ������������������������������·ݣ������·ݣ�����������������ȡ�
 * ��ʽת�����ܰ�����yyyymmddhhmiss��yyyy-mm-dd hh:mi:ss��time_t��int���⼸�ָ�ʽ���໥ת��
 *
 * @author zhang.fengli@zte.com.cn
 * @version 1.0
 * @date 2011/2/22
 * @bug  �½�����bug
 * @bug  ��Ԫ���ԣ���bug
 * @warning δ���쳣������ԣ�ʹ��ʱ��ע�����
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
    DATE_TYPE_YEAR = 0,     ///<��
    DATE_TYPE_MONTH = 1,    ///<��
    DATE_TYPE_DAY = 2,      ///<��
    DATE_TYPE_HOUR = 3,     ///<Сʱ
    DATE_TYPE_MINUTE = 4,   ///<����
    DATE_TYPE_SECOND = 5,   ///<��
    DATE_TYPE_DATE = 6,     ///<����
    DATE_TYPE_TIME = 7,     ///<ʱ��
    DATE_TYPE_WEEK = 8      ///<����
};

#define DATE_TIME_LEN       14
#define DATE_TIME_LONG_LEN  19
#define DATE_TIME_SHORT_LEN 8
#define DATE_TIME_SIZE      15

/**
 * @brief ������
 *
 * ���ڴ����ܺ�������
 *
 */
class CDate
{
public:
    /**
     * @brief ���캯��
     *
     * @param
     * @return
     * @retval
     */
    CDate();
    /**
     * @brief ��������
     *
     * @param
     * @return
     * @retval
     */
    ~CDate();

	
    /**
     * @brief ��ȡ��ǰϵͳʱ�䣬��ʽtime_t
     *
     * @return time_t
     * @retval ����0 ��ȷ
     * @retval ���� ʧ��
     */
    static time_t GetSysDateTime();
	
    /**
     * @brief ��ȡ��ǰϵͳʱ�䣬��ʽtime_t
     *
     * @param tDateTime [out]  ���time_t
     * @return time_t
     * @retval ����0 ��ȷ
     * @retval ���� ʧ��
     */
    static time_t GetSysDateTime(time_t &tDateTime);
    /**
     * @brief ��ȡ��ǰϵͳʱ�䣬��ʽyyyymmddhh24miss��yyyy-mm-dd hh:mi:ss
     *
     * ��bLongFlag = falseʱ�������ʽΪyyyymmddhh24miss
     * ��bLongFlag = trueʱ�������ʽΪyyyy-mm-dd hh:mi:ss
     *
     * @param pszDateTime [out]  ���YYYYMMDDHH24MISS���ַ���
     * @param bLongFlag [in] �Ƿ�����������ַ���yyyy-mm-dd hh:mi:ss��ʽ
     * @param nFlag [in] 0�������ʱ�䣬1������ڣ�2���ʱ��
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int GetSysDateTime(char *pszDateTime,const bool bLongFlag=false,const int nFlag = 0);
    /**
     * @brief ����
     *
     * @param nTime [in]
     * @return
     * @retval
     */
    static void Sleep(const int nTime);
    /**
     * @brief ���ڸ�ʽУ�飬У���ʽΪ�ַ���yyyymmddhh24miss
     *
     * @param pszDateTime [in] ����YYYYMMDDHH24MISS���ַ���
     * @return time_t
     * @retval ����0 ��ȷ
     * @retval ���� ʧ��
     */
    static time_t VerifyDateTime(const char *pszDateTime);
    /**
     * @brief �жϸ��ַ����Ƿ��� HHMMSS��ʽ��ʱ�����ַ���
     *
     * @param pszTime [in] Դ�ַ���
     * @return bool
     * @retval true    �����ַ�����HHMMSS��ʽ���ַ���
     * @retval false   �����ַ�������HHMMSS��ʽ���ַ���
     */
    static bool VerifyTimeStr(const char * pszTime);
    /**
     * @brief �жϸ��ַ����Ƿ��� YYYYMMDD��ʽ���������ַ���
     *
     * @param pszDate [in] Դ�ַ���
     * @return bool
     * @retval true    �����ַ�����YYYYMMDD��ʽ���ַ���
     * @retval false   �����ַ�������YYYYMMDD��ʽ���ַ���
     */
    static bool VerifyDateStr(const char * pszDate);
    /**
     * @brief �ж�ָ�����ڣ��Ƿ�����������֮��
     *
     * ����pszBeginDate��pszEndDate������һ������Ϊ�գ���pszInDate����Ϊ��
     * ���pszBeginDate==NULL����Ƚ�pszInDate<=pszEndDate
     * ���pszEndDate==NULL����Ƚ�pszBeginDate<=pszInDate
     * �����ζ���Ϊ��ʱ���Ƚ�pszBeginDate<=pszInDate && pszInDate<=pszEndDate
     *
     * @param pszInDate [in] ����YYYYMMDDHH24MISS���ַ���
     * @param pszBeginDate [in] ����YYYYMMDDHH24MISS���ַ���
     * @param pszEndDate [in] ����YYYYMMDDHH24MISS���ַ���
     * @return bool
     * @retval true
     * @retval false
     */
    static bool BetweenDate(const char *pszInDate,const char *pszBeginDate,const char *pszEndDate);
    /**
     * @brief ȡ�������ַ���������
     *
     * ���pszDateTimeΪ�գ�nValueȡ��ǰϵͳʱ�������
     *
     * @param pszDateTime [in] ����YYYYMMDD������
     * @return int
     * @retval 1-7 ��ȷ
     * @retval ���� ʧ��
     */
    static int GetWeek(const char *pszDateTime = NULL);
    /**
     * @brief ȡ����n�µĽ���ʱ�� ������23:59:59
     *
     * ��nCount=0ʱ��Ϊ��ǰ��
     * ��nCount<0ʱ��Ϊ֮ǰ|n|��
     * ��nCount>0ʱ��Ϊ֮��n��
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
     * @brief ȡ����n���µĿ�ʼʱ�� 1��00:00:00
     *
     * ��nCount=0ʱ��Ϊ��ǰ��
     * ��nCount<0ʱ��Ϊ֮ǰ|n|��
     * ��nCount>0ʱ��Ϊ֮��n��
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
     * @brief �ж��Ƿ�Ϊ����
     *
     * @param pszYear [in]
     * @return bool
     * @retval true
     * @retval false
     */
    static bool IfLeapYear(char* pszYear);
    /**
     * @brief ȡ�������һ��,������ڸ�ʽyyyymmdd
     *
     * @param pszDate [in]
     * @param pszRetDate [out]
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int GetYearBegin(char *pszDate, char *pszRetDate);
    /**
     * @brief ȡ�������һ��
     *
     * @param tDate [in]
     * @return time_t
     * @retval ����0
     */
    static time_t GetYearBegin(time_t tDate);
    /**
     * @brief ȡ����ĩ���һ��,������ڸ�ʽyyyymmdd
     *
     * @param pszDate [in]
     * @param pszRetDate [out]
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int GetYearEnd(char *pszDate, char *pszRetDate);
    /**
     * @brief ȡ����ĩ���һ��
     *
     * @param tDate [in]
     * @return time_t
     * @retval ����0
     */
    static time_t GetYearEnd(time_t tDate);

    /**
     * @brief ��������
     *
     * @param pszDateTime [in] ����YYYYMMDDHH24MISS���ַ���
     * @param nSeconds [in] ���ӵ�����
     * @param pszRetValue [out] ���YYYYMMDDHH24MISS���ַ���
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int AddSeconds(const char *pszDateTime,const int nSeconds,char *pszRetValue);
    /**
     * @brief ��������
     *
     * @param pszDateTime [in] ����YYYYMMDDHH24MISS���ַ���
     * @param nDays [in] ���ӵ�����
     * @param pszRetValue [out] ���YYYYMMDDHH24MISS���ַ���
     * @param nRetLen [in] �����ַ�������
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int AddDays(const char *pszDateTime,const int nDays,char *pszRetValue,int nRetLen);
    /**
     * @brief �����·�
     *
     * @param nDateTime [in] ����YYYYMM
     * @param nMonths [in] ���ӵ��·�
     * @return int
     * @retval ����0���·�ֵ
     * @retval <=0 ʧ��
     */
    static int AddMonths(const int nDateTime,const int nMonths);
    /**
    * @brief �����·�
    *
    * ȱ�ٶ�����Ŀ���
    *
    * @param pszDateTime [in] ����YYYYMMDDHH24MISS���ַ���
    * @param nMonths [in] ���ӵ��·�
    * @param pszRetValue [out] ���YYYYMMDDHH24MISS���ַ���
    * @param nRetLen [in] �����ַ�������
    * @return int
    * @retval 0 ��ȷ
    * @retval ���� ʧ��
    */
    static int AddMonths(const char *pszDateTime,const int nMonths,char *pszRetValue,int nRetLen);
    /**
     * @brief ������������֮����·ݼ��
     *
     * @param pszDateTime1 [in] ����YYYYMMDDHH24MISS���ַ���
     * @param pszDateTime2 [in] ����YYYYMMDDHH24MISS���ַ���
     * @param nMonths [out] �����·�ֵ
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int IntervalMonth(const char *pszDateTime1, const char *pszDateTime2, int &nMonths);
    /**
     * @brief ��������ʱ����������
     *
     * @param tBeginTime [in]
     * @param tEndTime [in]
     * @param nDays [out] ��������ֵ
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int IntervalDay(const time_t tBeginTime,const time_t tEndTime, int &nDays);
    /**
     * @brief ��������ʱ����������
     *
     * @param pszDateTime1 [in] ����YYYYMMDDHH24MISS���ַ���
     * @param pszDateTime2 [in] ����YYYYMMDDHH24MISS���ַ���
     * @param lSeconds [out] ��������ֵ
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int IntervalSecond(const char *pszDateTime1, const char *pszDateTime2, long &lSeconds);


    /**
     * @brief ��ʽת����YYYYMMDDHHMISS->YYYY-MM-DD HH:MI:SS
     *
     * @param pszDateTime [in]
     * @param pszRetDateTime [out]
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int FormatDateTimeStoL(const char *pszDateTime, char *pszRetDateTime);
    /**
     * @brief ��ʽת����YYYYMMDDHHMISS->int
     *
     * @param pszDateTime [in]
     * @param nRetValue [out]
     * @return int
     * @retval >0 ��ȷ
     * @retval -1 ʧ��
     */
    static int FormatDateTimeStoI(const char *pszDateTime,int &nRetValue);
    /**
     * @brief ��ʽת����YYYYMMDDHHMISS->time_t
     *
     * @param pszDateTime [in] ����YYYYMMDDHHMISS
     * @param tRetDateTime [out] ���time_t
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int FormatDateTimeStoT(const char *pszDateTime, time_t &tRetDateTime);
    /**
     * @brief ��ʽת����YYYYMMDDHHMISS->time_t��ʱ����ȡ0
     *
     * @param pszDateTime [in] ����YYYYMMDDHHMISS
     * @param tRetDateTime [out] ���time_t
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int FormatStrToTime(const char *pszDateTime, time_t &tRetDateTime);

    /**
     * @brief ��ʽת����time_t->YYYY-MM-DD HH:MI:SS
     *
     * @param tDateTime [in] ����time_t
     * @param pszDateTime [out] ���YYYY-MM-DD HH:MI:SS
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int FormatDateTimeTtoL(const time_t tDateTime,char *pszDateTime);
    /**
     * @brief ��ʽת����time_t->YYYYMMDDHHMISS
     *
     * @param tDateTime [in] ����time_t
     * @param pszDateTime [out] ���YYYYMMDDHHMISS
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int FormatDateTimeTtoS(const time_t tDateTime,char *pszDateTime);

    /**
     * @brief ��ʽת����YYYY-MM-DD HH:MI:SS->time_t
     *
     * @param pszDateTime [in] ����YYYY-MM-DD HH:MI:SS
     * @param tRetDateTime [out] ���time_t
     * @param bLongFlag [in] �Ƿ����ڸ�ʽ Ϊtrueʱ pszDateTime��ʽΪYYYY-MM-DD HH:MI:SS��Ϊfalseʱ pszDateTime��ʽΪYYYY-MM-DD
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int FormatDateTimeLtoT(const char *pszDateTime, time_t &tRetDateTime,const bool bLongFlag=true);
    /**
     * @brief ��ʽת����YYYY-MM-DD HH:MI:SS->YYYYMMDDHHMISS
     *
     * @param pszDateTime [in]
     * @param pszRetDateTime [out]
     * @param nDateTimeFlag [in] Ϊ����ʱ���ʶ��Ϊ0ʱ���ص�ǰ����ʱ�䣬Ϊ1���ص�ǰ���ڣ�Ϊ2���ص�ǰʱ��
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static int FormatDateTimeLtoS(const char *pszDateTime, char *pszRetDateTime, const int nDateTimeFlag=0);
    /**
     * @brief ��ʽת����time_t->YYYYMMDDHHMISS
     *
     * @param tDateTime [in] ����time_t
     * @param pszDateTime [out] ���YYYYMMDDHHMISS
     * @return int
     * @retval 0 ��ȷ
     * @retval ���� ʧ��
     */
    static const char * FormatGMTimeToString(const time_t tTime,const bool bLongFlag=false);

protected:
private:
};
#endif ///<__C_DATE_H__

