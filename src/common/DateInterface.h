/**
 * @file DateInterface.h
 * 
 * 
 * 
 * @author zhang.fengli@zte.com.cn
 * @version 1.0
 * @date 2011/03/21
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
 * @warning �ӿں���Ϊ���̰߳�ȫ����ע��ʹ��
 */
#ifndef __DATE_INTERFACE_H__
#define __DATE_INTERFACE_H__

#include "Date.h"
/**
 * @brief ȡ�����ڼ������·���
 * 
 * @param psztTime1 [in] 
 * @param psztTime2 [in]
 * @return int
 * @retval ����·�ֵ
 * @retval -1 ʧ��
 */
int	IntervalMonth(char* psztTime1, char* psztTime2);
/**
 * @brief ȡ�����ڼ���������
 * 
 * @param psztTime1 [in] 
 * @param psztTime2 [in]
 * @return bool
 * @retval true
 * @retval false
 */
bool IntervalDay(const time_t tmBeginTime,const time_t tmEndTime,int &nDayCnt);
/**
 * @brief �������ַ�����ʽת����int
 * 
 * @param szAnswerTime [in] 
 * @param  [out]
 * @return int
 * @retval ����ֵ
 * @retval -1
 */
int iGetTime(char szAnswerTime[15]);
/**
 * @brief ���ַ���szTime��nSeconds��
 * 
 * @param szTime [in/out] 
 * @param nSeconds [in]
 * @return int
 * @retval 0
 */
int AddSeconds(char szTime[],int nSeconds);
/**
 * @brief �������ַ���yyyymmddhh24missת����time_t��ʽ
 * 
 * @param szAnswerTime [in] 
 * @param  [out]
 * @return time_t
 * @retval ʱ��time_t
 */
time_t tGetTime(const char szAnswerTime[15]);
/**
 * @brief �������ַ���yyyy-mm-dd hh:mm:ssת����time_t��ʽ
 * 
 * @param szAnswerTime [in] 
 * @param  [out]
 * @return time_t
 * @retval ʱ��time_t
 * @retval 
 */
time_t tGetInfTime (const char szAnswerTime[20]);
/**
 * @brief ��time_t��ʽת���������ַ���yyyymmddhh24miss
 * 
 * @param tTime [in] 
 * @param  [out]
 * @return char*
 * @retval �ַ�������
 * @retval 
 */
char * Time2Asc(time_t tTime);
/**
 * @brief ��time_t��ʽת���������ַ���yyyy-mm-dd hh:mm:ss
 * 
 * @param tTime [in] 
 * @param  [out]
 * @return char*
 * @retval �ַ�������
 * @retval 
 */
char * Time2InfAsc(time_t tTime);
/**
 * @brief �������ַ�������֮���������ֵ
 * 
 * @param szEndDate [in] 
 * @param szBeginDate [in]
 * @return long
 * @retval ������ֵ
 * @retval 
 */
long DiffDays(char *szEndDate,char *szBeginDate);
/**
 * @brief �������ַ���ת����time_t��ʽ��ʱ����ȡ0
 * 
 * @param pszBeginDate [in] 
 * @param  [out]
 * @return time_t
 * @retval time_t����ֵ
 * @retval 
 */
time_t tGetDate_t(char *pszBeginDate);
/**
 * @brief ��ȡ�����ַ���������ֵ
 * 
 * @param szDate [in] 
 * @param  [out]
 * @return int
 * @retval 1-7������ֵ
 * @retval -1
 */
int ToWeek(char szDate[]);
/**
 * @brief ��ȡϵͳʱ�䣬��ʽyyyymmddhh24miss
 * 
 * @param szSysDate [in/out] 
 * @param  [out]
 * @return int
 * @retval 1
 */
int GetSysDate(char szSysDate[]);
/**
 * @brief ��ȡϵͳʱ�䣬��ʽyyyy-mm-dd hh:mm:ss
 * 
 * @param szSysDate [in/out] 
 * @param  [out]
 * @return int
 * @retval 1
 */
int GetSysDateStr(char szSysDate[]);
/**
 * @brief ����������nDay��
 * 
 * @param szDate [in/out] 
 * @param nDay [in]
 * @return int
 * @retval 1
 * @retval 
 */
int AddDay(char szDate[],int nDay);
/**
 * @brief ����������1��
 * 
 * @param szDate [in/out] 
 * @return int
 * @retval 1
 * @retval 
 */
int AddDate(char szDate[]);
/**
 * @brief ����������-1��
 * 
 * @param szDate [in/out]
 * @return int
 * @retval 1
 * @retval 
 */
int SubDate(char szDate[15]);
/**
 * @brief ��yyyymmddhh24miss��ʽת���������ַ���yyyy-mm-dd hh:mm:ss
 * 
 * @param szDateTime [in] 
 * @return char*
 * @retval �ַ���
 * @retval 
 */
char * GetFormatDateTime(char *szDateTime);
/**
 * @brief �ж�����pszInDate�Ƿ�������pszBeginDate��pszEndDate֮��
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
 * @brief ��ȡ��ǰϵͳʱ��
 *
 * ����nDateTimeFlagֵ���ز�ͬ���ַ�����0��yyyy-mm-dd hh:mm:ss��1��yyyy-mm-dd��2��hh:mm:ss
 * 
 * @param szDateTime [out] 
 * @param nDateTimeFlag [in]
 * @return void
 * @retval 
 * @retval 
 */
void GetNowDateTime(char *szDateTime,int nDateTimeFlag);
/**
 * @brief ��ʽת����YYYY-MM-DD HH:MI:SS->YYYYMMDDHHMISS
 *
 * @param szSrcTime [in] 
 * @param szDestTime [out] 
 * @return void
 * @retval 
 * @retval 
 */
void ConvertTimeText2ToTimeText1(char *szSrcTime,char *szDestTime);
/**
 * @brief ��ʽת����YYYYMMDDHHMISS->YYYY-MM-DD HH:MI:SS
 *
 * @param szSrcTime [in] 
 * @param szDestTime [out] 
 * @return void
 * @retval 
 * @retval 
 */
void ConvertTimeText1ToTimeText2(char *szSrcTime,char *szDestTime);
/**
 * @brief ��ʽת����YYYYMMDDHHMISS->time_t
 *
 * @param szSrcTime [in] 
 * @param ptDate [out] 
 * @return void
 * @retval 
 * @retval 
 */
void ConvStrToTimet(const char* pszTimeStr,time_t* ptDate);
/**
 * @brief ��ʽת����time_t->YYYY-MM-DD HH:MI:SS
 *
 * @param tTime [in] 
 * @param pszTime [out] 
 * @return void
 * @retval 
 * @retval 
 */
void Getstime(time_t tTime, char *pszTime);
/**
 * @brief ��ʽת����YYYY-MM-DD HH:MI:SS->time_t
 *
 * @param szSrcTime [in] 
 * @param ptTime [out] 
 * @return void
 * @retval 
 * @retval 
 */
void GetTimeOfTimet(char * pszStrTime,time_t *ptTime);
/**
 * @brief ���ڸ�ʽУ�飬У���ʽΪ�ַ���yyyymmddhh24miss
 *
 * @param pszDateTime [in] ����YYYYMMDDHH24MISS���ַ���
 * @return time_t
 * @retval ����0 ��ȷ
 * @retval ���� ʧ��
 */
time_t VerifyDateTime(const char* pszDateTime);
/**
 * @brief ��ʽת����time_t->YYYYMMDDHHMISS
 *
 * @param tTime [in] 
 * @param pszDestVaue [out] 
 * @return char*
 * @retval pszDateTime
 * @retval 
 */
char* TimetToStr(char* pszDestVaue,time_t tTime);
/**
 * @brief ��ʽת����time_t->YYYYMMDDHHMISS
 *
 * @param tTime [in] 
 * @return char*
 * @retval �ַ�����ʽ������
 * @retval 
 */
char* TimetToStr(time_t tTime);
/**
 * @brief ΪtTime����1����
 *
 * @param tTime [in] 
 * @return time_t
 * @retval time_t��ʽ����
 * @retval 
 */
time_t AddMonth(time_t tTime);
/**
 * @brief �ж��Ƿ�����
 *
 * @param szYear [in] 
 * @return bool
 * @retval true
 * @retval false
 */
bool IsLeapYear(char szYear[5]);
/**
 * @brief Ϊ�ַ�������1��
 *
 * @param szDate [in/out] 
 * @return int
 * @retval 0
 * @retval 
 */
int AddOneDay(char szDate[15]);
/**
 * @brief Ϊ�ַ�����ȥ1��
 *
 * @param szDate [in/out] 
 * @return int
 * @retval 0
 * @retval 
 */
int SubOneDay(char szDate[15]);
/**
 * @brief ȡ�����ڵĶ�Ӧ�·ݵ����һ��
 *
 * @param szStrEndTime [in/out] 
 * @return void
 * @retval 
 * @retval 
 */
void GetMonthEndTime(char *szStrEndTime);

/**
 * @brief ����
 *
 * ��˵��
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



