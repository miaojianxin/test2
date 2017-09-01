#include "DateInterface.h"
static char szDate[DATE_TIME_LEN+1] = "";
static char szLongDate[DATE_TIME_LONG_LEN+1] = "";
using namespace std;
int	IntervalMonth(char* psztTime1, char* psztTime2)
{
    int nMonth = 0;
    if(CDate::IntervalMonth(psztTime1,psztTime2,nMonth) != 0)
    {
        return -1;
    }
    return nMonth;
}

bool IntervalDay(const time_t tmBeginTime,const time_t tmEndTime,int &nDayCnt)
{
    if(CDate::IntervalDay(tmBeginTime,tmEndTime,nDayCnt)!=0)
    {
        return false;
    }
    return true;
}

int iGetTime(char szAnswerTime[15])
{
    int nRetValue = 0;
    if(CDate::FormatDateTimeStoI(szAnswerTime,nRetValue)<0)
    {
        return -1;
    }
    return nRetValue;
}

int AddSeconds(char szTime[],int nSeconds)
{
    char szSrc[DATE_TIME_LEN+1] = "";
    if (szTime == NULL)
    {
        return -1;
    }
    strncpy(szSrc,szTime,DATE_TIME_LEN);
    CDate::AddSeconds(szSrc,nSeconds,szTime);
    return 0;
}

time_t tGetTime(const char szAnswerTime[15])
{
    time_t tTime;
    CDate::FormatDateTimeStoT(szAnswerTime,tTime);
    return tTime;
}

time_t tGetInfTime (const char szAnswerTime[20])
{
    time_t tTime;
    CDate::FormatDateTimeLtoT(szAnswerTime,tTime);
    return tTime;
}

char * Time2Asc(time_t tTime)
{
    CDate::FormatDateTimeTtoS(tTime,szDate);
    return szDate;
}

char * Time2InfAsc(time_t tTime)
{
    CDate::FormatDateTimeTtoL(tTime,szLongDate);
    return szLongDate;
}

long DiffDays(char *szEndDate,char *szBeginDate)
{
    time_t tTimeEnd;
    time_t tTimeBegin;
    int nDays = 0;
    if (szEndDate == NULL || szBeginDate == NULL)
    {
        return -1;
    }
    CDate::FormatDateTimeStoT(szEndDate,tTimeEnd);
    CDate::FormatDateTimeStoT(szBeginDate,tTimeBegin);
    return (long)(tTimeEnd-tTimeBegin)/86400;
}

time_t tGetDate_t(char *pszBeginDate)
{
    time_t tTime;
    CDate::FormatStrToTime(pszBeginDate,tTime);
    return tTime;
}

int ToWeek(char szDate[])
{
    return CDate::GetWeek(szDate);
}

int GetSysDate(char szSysDate[])
{
    if (szSysDate == NULL)
    {
        return 0;
    }
    CDate::GetSysDateTime(szSysDate);
    return 1;
}

int GetSysDateStr(char szSysDate[])
{
    if (szSysDate == NULL)
    {
        return 0;
    }
    CDate::GetSysDateTime(szSysDate,true);
    return 1;
}

int AddDay(char szDate[],int nDay)
{
    char szRetDate[DATE_TIME_LEN+1] = "";
    int nLen = strlen(szDate);
    if(CDate::AddDays(szDate,nDay,szRetDate,nLen)!=0)
    {
        return -1;
    }
    strncpy(szDate,szRetDate,nLen);
    return 1;
}

int AddDate(char szDate[])
{
    return AddDay(szDate,1);
}

int SubDate(char szDate[15])
{
    return AddDay(szDate,-1);
}

char * GetFormatDateTime(char *szDateTime)
{
    CDate::FormatDateTimeStoL(szDateTime,szLongDate);
    return szLongDate;
}

bool BetweenDate(const char *pszInDate,const char *pszBeginDate,const char *pszEndDate)
{
    return CDate::BetweenDate(pszInDate,pszBeginDate,pszEndDate);
}

void GetNowDateTime(char *szDateTime,int nDateTimeFlag)
{
    CDate::GetSysDateTime(szDateTime,true,nDateTimeFlag);
    return;
}

void ConvertTimeText2ToTimeText1(char *szSrcTime,char *szDestTime)
{
    CDate::FormatDateTimeLtoS(szSrcTime,szDestTime);
    return;
}
void ConvertTimeText1ToTimeText2(char *szSrcTime,char *szDestTime)
{
    CDate::FormatDateTimeStoL(szSrcTime,szDestTime);
    return;
}

void ConvStrToTimet(const char* pszTimeStr,time_t* ptDate)
{
    CDate::FormatDateTimeStoT(pszTimeStr,*ptDate);
    return;
}

void Getstime(time_t tTime, char *pszTime)
{
    CDate::FormatDateTimeTtoL(tTime,pszTime);
    return;
}

void GetTimeOfTimet(char * pszStrTime,time_t *ptTime)
{
    CDate::FormatDateTimeLtoT(pszStrTime,*ptTime);
    return;
}

time_t VerifyDateTime(const char* pszDateTime)
{
    return CDate::VerifyDateTime(pszDateTime);
}

char* TimetToStr(char* pszDestValue,time_t tTime)
{
    CDate::FormatDateTimeTtoS(tTime,pszDestValue);
    return pszDestValue;
}

char* TimetToStr(time_t tTime)
{
    CDate::FormatDateTimeTtoS(tTime,szDate);
    return szDate;
}

time_t AddMonth(time_t tTime)
{
    time_t tRetTime;
    char szDateTime[DATE_TIME_LEN+1] = "";
    char szRetDateTime[DATE_TIME_LEN+1] = "";
    CDate::FormatDateTimeTtoS(tTime,szDateTime);
    CDate::AddMonths(szDateTime,1,szRetDateTime,DATE_TIME_LEN);
    CDate::FormatDateTimeStoT(szRetDateTime,tRetTime);
    return tRetTime;
}

bool IsLeapYear(char szYear[5])
{
    return CDate::IfLeapYear(szYear);
}

int AddOneDay(char szDate[15])
{
    char szRetDate[DATE_TIME_LEN+1] = "";
    int nLen = strlen(szDate);
    if(CDate::AddDays(szDate,1,szRetDate,nLen)!=0)
    {
        return -1;
    }
    strncpy(szDate,szRetDate,nLen);
    return 0;
}

int SubOneDay(char szDate[15])
{
    char szRetDate[DATE_TIME_LEN+1] = "";
    int nLen = strlen(szDate);
    if(CDate::AddDays(szDate,-1,szRetDate,nLen)!=0)
    {
        return -1;
    }
    strncpy(szDate,szRetDate,nLen);
    return 0;
}

void GetMonthEndTime(char *szStrEndTime)
{
    time_t tTime;
    time_t tTimeEnd;
    int nRet = 0;
    if (szStrEndTime == NULL)
    {
        return;
    }
    nRet = CDate::FormatDateTimeStoT(szStrEndTime,tTime);
    if (nRet != 0)
    {
        return;
    }
    CDate::GetMonthEndDTime(tTime,&tTimeEnd);
    return;
}

time_t TDateTimeFunc::StringToTime(const char szTime[])
{
    time_t tTime;
    if (szTime == NULL)
    {
        return (time_t)0;
    }
    CDate::FormatDateTimeStoT(szTime,tTime);
    return tTime;
}

const char * TDateTimeFunc::GetCurrentTimeStr(char szTime[],size_t nLen,const bool bLongFlag)
{
    CDate::GetSysDateTime(szLongDate,bLongFlag);
    if(szTime != NULL)
    {
        if (nLen != 0)
        {
            strncpy(szTime,szLongDate,nLen-1);
        }
        else
        {
            strcpy(szTime,szLongDate);
        }
    }    
    return szLongDate;
}

const char * TDateTimeFunc::GetTimeAddSeconds(const char szTime[],long lSeconds,bool bSysFlag)
{
    CDate::AddSeconds(szTime,(int)lSeconds,szDate);
    return szDate;
}

void TDateTimeFunc::Sleep(const int nSeconds)
{
    CDate::Sleep(nSeconds);
    return;
}

const char * TDateTimeFunc::TimeToString(const time_t tTime,const bool bLongFlag)
{
    if (bLongFlag)
    {
        CDate::FormatDateTimeTtoS(tTime,szLongDate);
    }
    else
    {
        CDate::FormatDateTimeTtoS(tTime,szLongDate);
    }
    return szLongDate;
}

const char * TDateTimeFunc::GMTimeToString(const time_t tTime,const bool bLongFlag)
{
    return CDate::FormatGMTimeToString(tTime,bLongFlag);
}

long TDateTimeFunc::GetDiffTime(const char szTime1[], const char szTime2[])
{
    long lTime = 0;
    CDate::IntervalSecond(szTime1,szTime2,lTime);
    return lTime;
}

long TDateTimeFunc::GetDiffDay(const char szTime1[], const char szTime2[])
{
    int nDay = 0;
    time_t tTime1 = CDate::VerifyDateTime(szTime1);
    time_t tTime2 = CDate::VerifyDateTime(szTime2);
    if (tTime1 < 0 || tTime2 < 0)
    {
        return -1;
    }
    CDate::IntervalDay(tTime1,tTime2,nDay);
    return (long)nDay;
}

void TDateTimeFunc::AddDay(char szDate[],long lDays)
{
    char szRetDate[DATE_TIME_LEN+1] = "";
    CDate::AddDays(szDate,(int)lDays,szRetDate,DATE_TIME_LEN);
    strcpy(szDate,szRetDate);
}

void TDateTimeFunc::AddDate(char szDate[])
{
    char szRetDate[DATE_TIME_LEN+1] = "";
    CDate::AddDays(szDate,1,szRetDate,DATE_TIME_LEN);
    strcpy(szDate,szRetDate);
    return;
}

void TDateTimeFunc::SubDate(char szDate[])
{
    char szRetDate[DATE_TIME_LEN+1] = "";
    CDate::AddDays(szDate,-1,szRetDate,DATE_TIME_LEN);
    strcpy(szDate,szRetDate);
    return;
}

int TDateTimeFunc::GetWeek(const char szDate[])
{
    return CDate::GetWeek(szDate);
}


