#include "Date.h"
#include <unistd.h>

static struct tm *localtime_zx(time_t time, struct tm *ret_time);
static time_t mktime_zx(struct tm *tm);

#ifdef _localtime
#undef _localtime
#endif
#ifdef _mktime
#undef _mktime
#endif
#ifdef _THREAD_SAFE_
#define _localtime(timet, tm)	localtime_zx(timet, tm)
#define _mktime(tm)				mktime_zx(tm)
#else
#define _localtime(timet, tm)	localtime_r(&timet, tm)
#define _mktime(tm)				mktime(tm)
#endif

CDate::CDate()
{

}

CDate::~CDate()
{

}

time_t CDate::GetSysDateTime()
{
	time_t tDateTime = -1;
    time(&tDateTime);
    return tDateTime;
}

time_t CDate::GetSysDateTime(time_t &tDateTime)
{
    time(&tDateTime);
    return tDateTime;
}

int CDate::GetSysDateTime(char *pszDateTime,const bool bLongFlag/* =false */,const int nFlag/* = 0*/)
{
    int nRet = 0;
    time_t tCurrent;
    struct tm *tmCur = NULL, tmData;
   

    if(pszDateTime==NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }

    memset(pszDateTime,0x00,sizeof(pszDateTime));

    time(&tCurrent); //取得当前时间的time_t值
//    tmCur = localtime(&tCurrent); //取得当前时间的tm值
//    tmCur = localtime_zx(tCurrent, &tmData);
    tmCur = _localtime(tCurrent, &tmData);

    if(bLongFlag)
    {
        if (nFlag == 0)
        {
            sprintf(pszDateTime,"%04d-%02d-%02d %02d:%02d:%02d",tmCur->tm_year+1900,tmCur->tm_mon+1,tmCur->tm_mday,tmCur->tm_hour,tmCur->tm_min,tmCur->tm_sec);
        }
        else if (nFlag == 1)
        {
            sprintf(pszDateTime,"%04d-%02d-%02d",tmCur->tm_year+1900,tmCur->tm_mon+1,tmCur->tm_mday);
        }
        else if (nFlag == 2)
        {
            sprintf(pszDateTime,"%02d:%02d:%02d",tmCur->tm_hour,tmCur->tm_min,tmCur->tm_sec);
        }
        
    }
    else
    {
        if (nFlag == 0)
        {
            sprintf(pszDateTime,"%04d%02d%02d%02d%02d%02d",tmCur->tm_year+1900,tmCur->tm_mon+1,tmCur->tm_mday,tmCur->tm_hour,tmCur->tm_min,tmCur->tm_sec);
        }
        else if (nFlag == 1)
        {
            sprintf(pszDateTime,"%04d%02d%02d",tmCur->tm_year+1900,tmCur->tm_mon+1,tmCur->tm_mday);
        }
        else if (nFlag == 2)
        {
            sprintf(pszDateTime,"%02d%02d%02d",tmCur->tm_hour,tmCur->tm_min,tmCur->tm_sec);
        }        
    }
    tmCur = NULL;
    return nRet;
}

void CDate::Sleep(const int nTime)
{
#ifdef _MSVC
    ::Sleep(nTime*1000);
#else
    sleep(nTime);
#endif
    return;
}

time_t CDate::VerifyDateTime(const char *pszDateTime)
{
    int nRet = 0;
    if (pszDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return (time_t)-1;
    }
    int len = strlen(pszDateTime);
    if(len < 14)
    {
        //printf("字符串长度太短!\n");
        return (time_t)-1;
    }

    char szTemp[20] = "";
    memset(szTemp,0,sizeof(szTemp));
    strncpy(szTemp,pszDateTime,4);
    strncpy(szTemp+5,pszDateTime+4,2);
    strncpy(szTemp+8,pszDateTime+6,2);
    strncpy(szTemp+11,pszDateTime+8,2);
    strncpy(szTemp+14,pszDateTime+10,2);
    strncpy(szTemp+17,pszDateTime+12,2);

    struct tm tmTmp;
    struct tm tmTmp1;

    char* pszEnd = NULL;

    tmTmp.tm_year = strtol(szTemp, &pszEnd,10) - 1900;
    if(*pszEnd != '\0')
    {
        pszEnd = NULL;
        return (time_t)-1;
    }

    tmTmp.tm_mon  = strtol(szTemp+5, &pszEnd,10) - 1;
    if(*pszEnd != '\0')
    {
        pszEnd = NULL;
        return (time_t)-1;
    }

    tmTmp.tm_mday = strtol(szTemp+8, &pszEnd,10);
    if(*pszEnd != '\0')
    {
        pszEnd = NULL;
        return (time_t)-1;
    }

    tmTmp.tm_hour = strtol(szTemp+11,&pszEnd,10);
    if(*pszEnd != '\0')
    {
        pszEnd = NULL;
        return (time_t)-1;
    }

    tmTmp.tm_min  = strtol(szTemp+14,&pszEnd,10);
    if(*pszEnd != '\0')
    {
        pszEnd = NULL;
        return (time_t)-1;
    }

    tmTmp.tm_sec  = strtol(szTemp+17,&pszEnd,10);
    if(*pszEnd != '\0')
    {
        pszEnd = NULL;
        return (time_t)-1;
    }

    tmTmp.tm_isdst = -1;

    tmTmp1 = tmTmp;

//    time_t tmTime = mktime(&tmTmp);
//    time_t tmTime = mktime_zx(&tmTmp);
    time_t tmTime = _mktime(&tmTmp);
    if(tmTime == (time_t)-1)
    {
        pszEnd = NULL;
        return (time_t)-1;
    }

    if( tmTmp.tm_year   != tmTmp1.tm_year    ||
        tmTmp.tm_mon    != tmTmp1.tm_mon     ||
        tmTmp.tm_mday   != tmTmp1.tm_mday    ||
        tmTmp.tm_hour   != tmTmp1.tm_hour    ||
        tmTmp.tm_min    != tmTmp1.tm_min     ||
        tmTmp.tm_sec    != tmTmp1.tm_sec)
    {
        pszEnd = NULL;
        return (time_t)-1;
    }

    if(tmTmp.tm_isdst == 1)
    {
        tmTime += 3600;
    }
    pszEnd = NULL;
    return tmTime;
}

bool CDate::VerifyTimeStr(const char * pszTime)
{
    int nNum = 0;
    int i = 0;
    char sztmp[10];

    if(strlen(pszTime) != 6)
    {
        return false;
    }
    int iLen = strlen(pszTime);
    for (i=0;i<iLen;i++)
    {
        if ((pszTime[i]<'0')||(pszTime[i]>'9'))
        {
            return false;
        }
    }
    strncpy(sztmp , pszTime , 2);
    sztmp[2] = 0;
    nNum = atoi(sztmp);
    if((nNum < 0) || (nNum > 23))
    {
        return false;
    }
    strncpy(sztmp , pszTime + 2 , 2);
    sztmp[2] = 0;
    nNum = atoi(sztmp);
    if((nNum < 0) || (nNum > 59))
    {
        return false;
    }
    strncpy(sztmp , pszTime + 4 , 2);
    sztmp[2] = 0;
    nNum = atoi(sztmp);
    if((nNum < 0) || (nNum > 59))
    {
        return false;
    }

    return true;
}

bool CDate::VerifyDateStr(const char * pszDate)
{
    int nNum = 0;
    char sztmp[10] = "";
    int nYear = 0;
    int nMonth = 0;
    int i = 0;

    if(strlen(pszDate) != 8)
    {
        return false;
    }
    int nLen = strlen(pszDate);
    for (i = 0; i < nLen; i++)
    {
        if ((pszDate[i] < '0') || (pszDate[i] > '9'))
        {
            return false;
        }
    }

    strncpy(sztmp , pszDate , 4);
    sztmp[4] = 0;
    nNum = atoi(sztmp);
    nYear = nNum;
    if((nNum < 1900) || (nNum > 2099))
    {
        return false;
    }
    strncpy(sztmp , pszDate + 4 , 2);
    sztmp[2] = 0;
    nNum = atoi(sztmp);
    nMonth = nNum;
    if((nNum < 1) || (nNum > 12))
    {
        return false;
    }
    strncpy(sztmp, pszDate + 6 , 2);
    sztmp[2] = 0;
    nNum = atoi(sztmp);
    if((nNum < 1) || (nNum > 31))
    {
        return false;
    }
    switch(nMonth)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        if((nNum < 1) || (nNum > 31))
        {
            return false;
        }
        break;
    case 2:
        if((nYear%4==0&&nYear%100!=0)||nYear%400==0) //judge leap year
        {
            if((nNum < 1) || (nNum > 29))
            {
                return false;
            }
        }
        else
        {
            if((nNum < 1) || (nNum > 28))
            {
                return false;
            }
        }
        break;
    default :
        if((nNum < 1) || (nNum > 30))
        {
            return false;
        }
        break;
    }
    return true;
}

bool CDate::BetweenDate(const char *pszInDate,const char *pszBeginDate,const char *pszEndDate)
{
    if(pszBeginDate == NULL && pszInDate != NULL && pszEndDate != NULL)
    {
        return strcmp(pszInDate,pszEndDate)<=0;
    }
    else if(pszEndDate == NULL && pszInDate != NULL && pszBeginDate != NULL)
    {
        return strcmp(pszInDate,pszBeginDate)>=0;
    }
    else if(pszBeginDate != NULL && pszInDate != NULL && pszEndDate != NULL)
    {
        return strcmp(pszInDate,pszBeginDate)>=0 && strcmp(pszInDate,pszEndDate)<=0;
    }
    //printf("传入指针为空!\n");
    return false;
}

int CDate::GetWeek(const char *pszDateTime /* = NULL */)
{
    int nRet = 0;
    struct tm tmTime;
	memset(&tmTime, 0x00, sizeof(tm));	// add by lin.qiongshan, ur120035, 20130107
    char szDate[DATE_TIME_LEN+1] = "";
    memset(szDate,0x00,DATE_TIME_LEN);
    if (pszDateTime == NULL)
    {
        time_t tCurrent;
        time(&tCurrent);
//		  tmTime = *(localtime(&tCurrent));
//        tmTime = *(localtime_zx(tCurrent, &tmTime));
        _localtime(tCurrent, &tmTime);
    }
    else
    {
        if (strlen(pszDateTime) < DATE_TIME_SHORT_LEN)
        {
            //printf("入参长度不够!\n");
            return -1;
        }
        strncpy(szDate,pszDateTime,DATE_TIME_SHORT_LEN);
        sscanf(szDate, "%04d%02d%02d", &tmTime.tm_year, &tmTime.tm_mon, &tmTime.tm_mday);
        tmTime.tm_year -= 1900;
        tmTime.tm_mon--;
        tmTime.tm_isdst = -1;
    }    
//    mktime(&tmTime);
//    mktime_zx(&tmTime);
    _mktime(&tmTime);
    if (tmTime.tm_wday == 0)
    {
        tmTime.tm_wday = 7;
    }
    return tmTime.tm_wday;
}

int CDate::GetMonthEndDTime(time_t tSpecialTime,time_t *ptMonthEnd,int nCount /* = 0 */)
{
    int nRet = 0;
    if (ptMonthEnd == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
#ifdef _WIN32
    return nRet;
#else
    struct tm tmMonthEnd ;

//    localtime_r(&tSpecialTime , &tmMonthEnd);
//    localtime_zx(tSpecialTime, &tmMonthEnd);
    _localtime(tSpecialTime , &tmMonthEnd);
    if (nCount == 0)
    {        
        tmMonthEnd.tm_mon = tmMonthEnd.tm_mon + 1 ;
    }
    else if (nCount > 0)
    {
        tmMonthEnd.tm_mon = tmMonthEnd.tm_mon + nCount + 1;
        while (tmMonthEnd.tm_mon > 12)
        {
            tmMonthEnd.tm_mon -= 12;
            tmMonthEnd.tm_year++;
        }
    }
    else if (nCount < 0)
    {
        tmMonthEnd.tm_mon = tmMonthEnd.tm_mon + nCount + 1;
        while (tmMonthEnd.tm_mon < 0)
        {
            tmMonthEnd.tm_mon += 12;
            tmMonthEnd.tm_year--;
        }
    }
    tmMonthEnd.tm_mday = 1;
    tmMonthEnd.tm_hour = 0;
    tmMonthEnd.tm_min = 0;
    tmMonthEnd.tm_sec = 0;
//    (*ptMonthEnd) = mktime(&tmMonthEnd) - 1;
//    (*ptMonthEnd) = mktime_zx(&tmMonthEnd) - 1;
    (*ptMonthEnd) = _mktime(&tmMonthEnd) - 1;
#endif
    return nRet;
}

int CDate::GetMonthBeginDTime(time_t tSpecialTime,time_t *ptMonthBegin,int nCount /* = 0 */)
{
    int nRet = 0;
    if (ptMonthBegin == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
#ifdef _WIN32
    return;
#else
    struct tm tmMonthBegin ;

//    localtime_r(&tSpecialTime , &tmMonthBegin);
//    localtime_zx(tSpecialTime , &tmMonthBegin);
    _localtime(tSpecialTime , &tmMonthBegin);
    tmMonthBegin.tm_mon = tmMonthBegin.tm_mon + nCount;
    if (nCount > 0)
    {        
        while (tmMonthBegin.tm_mon > 12)
        {
            tmMonthBegin.tm_mon -= 12;
            tmMonthBegin.tm_year++;
        }
    }
    else if (nCount < 0)
    {
        while (tmMonthBegin.tm_mon < 0)
        {
            tmMonthBegin.tm_mon += 12;
            tmMonthBegin.tm_year--;
        }
    }
    tmMonthBegin.tm_mday = 1;
    tmMonthBegin.tm_hour = 0;
    tmMonthBegin.tm_min = 0;
    tmMonthBegin.tm_sec = 0;
//    (*ptMonthBegin) = mktime(&tmMonthBegin);
//    (*ptMonthBegin) = mktime_zx(&tmMonthBegin);
    (*ptMonthBegin) = _mktime(&tmMonthBegin);
#endif
    return nRet;
}

int CDate::AddSeconds(const char *pszDateTime,const int nSeconds,char *pszRetValue)
{
    int nRet = 0;
    time_t tTime;
    struct tm * tm = NULL, tmData;
    int nValue = 0;
    if (pszDateTime == NULL || pszRetValue == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    nRet = FormatDateTimeStoI(pszDateTime,nValue);
    if (nRet < 0)
    {
        //printf("调用函数FormatDateTimeStoI()失败\n");
        return nRet;
    }
    tTime = (time_t)(nValue+nSeconds);
//    tm = localtime(&tTime);
//    tm = localtime_zx(tTime, &tmData);
    tm = _localtime(tTime, &tmData);
    sprintf(pszRetValue, "%04d%02d%02d%02d%02d%02d", 
        tm->tm_year+1900, tm->tm_mon+1,tm->tm_mday, 
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    tm = NULL;
    return nRet;
}

int CDate::AddDays(const char *pszDateTime,const int nDays,char *pszRetValue,int nRetLen)
{
    int nRet = 0;
    struct tm tmInDate,tmOutDate;
    time_t tTmp;
    char szTmp[5] = "";
    char szDayTemp[51] = "";
    /*change szInDate to tmDate1*/
    if (pszDateTime == NULL || pszRetValue == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if (strlen(pszDateTime) < DATE_TIME_SHORT_LEN)
    {
        //printf("传入日期有误!\n");
        return -1;
    }
    
    memset(&tmInDate,0,sizeof(struct tm));
    memcpy(szTmp,pszDateTime,4);
    szTmp[4] = '\0';
    tmInDate.tm_year = atoi(szTmp) - 1900;
    memcpy(szTmp,pszDateTime + 4,2);
    szTmp[2] = '\0';
    tmInDate.tm_mon = atoi(szTmp) - 1;
    memcpy(szTmp,pszDateTime + 6,2);
    szTmp[2] = '\0';
    tmInDate.tm_mday = atoi(szTmp);
    if (strlen(pszDateTime) == DATE_TIME_LEN)
    {
        memcpy(szTmp,pszDateTime + 8,2);
        szTmp[2] = '\0';
        tmInDate.tm_hour = atoi(szTmp);
        memcpy(szTmp,pszDateTime + 10,2);
        szTmp[2] = '\0';
        tmInDate.tm_min = atoi(szTmp);
        memcpy(szTmp,pszDateTime + 12,2);
        szTmp[2] = '\0';
        tmInDate.tm_sec = atoi(szTmp);
    }
    
//    if ((tTmp = mktime(&tmInDate)) == (time_t)-1)
//    if ((tTmp = mktime_zx(&tmInDate)) == (time_t)-1)
    if ((tTmp = _mktime(&tmInDate)) == (time_t)-1)
    {
        return -1;
    }
    tTmp = tTmp + (time_t)nDays * 86400;	/*ndays之后*/
//    tmOutDate = *localtime(&tTmp);
//    localtime_zx(tTmp, &tmOutDate);
    _localtime(tTmp, &tmOutDate);
    sprintf(szDayTemp,"%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d",
        tmOutDate.tm_year + 1900,
        tmOutDate.tm_mon + 1,
        tmOutDate.tm_mday,
        tmOutDate.tm_hour,
        tmOutDate.tm_min,
        tmOutDate.tm_sec);
    strncpy(pszRetValue, szDayTemp, nRetLen);

    return nRet;
}

int CDate::AddMonths(const int nDateTime,const int nMonths)
{
    int	nRetAcctMonth = 0;

    int	nCurYear = nDateTime / 100;
    int nCurMonth = nDateTime % 100;

    if (nMonths >= 0)
    {
        if ((nCurMonth+nMonths)%12 != 0)
            nRetAcctMonth = (nCurYear+(nCurMonth+nMonths)/12)*100 + (nCurMonth+nMonths)%12;
        else
            nRetAcctMonth = (nCurYear+(nCurMonth+nMonths)/12-1)*100 + 12;
    }
    else
    {
        int nYearOffset = 0;
        nCurYear = nCurYear + nMonths/12;
        nYearOffset = -nMonths/12;
        if (nCurMonth+nMonths%12 <= 0)
        {
            nCurYear--;
            nYearOffset++;
        }

        nRetAcctMonth = nCurYear*100 + (nYearOffset*12+nCurMonth+nMonths);
    }
    return nRetAcctMonth;
}

int CDate::AddMonths(const char *pszDateTime,const int nMonths,char *pszRetValue,int nRetLen)
{
    int nRet = 0;
    int nSizeBuf = DATE_TIME_LEN;
    char szRetDate[DATE_TIME_LEN+1] = "";
    if (pszDateTime == NULL || pszRetValue == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if (nRetLen < nSizeBuf)
    {
        nSizeBuf = nRetLen;
    }
    if (strlen(pszDateTime) != DATE_TIME_LEN)
    {
        //printf("输入的时间日期格式非法\n");
        return -1;
    }

    char szTemp[50];
    memset(szTemp, 0x00, sizeof(szTemp));
    strncpy(szTemp, pszDateTime, 6);
    int nMonth = atoi(szTemp);
    int nMonthOffseted = AddMonths(nMonth, nMonths);

    int nMonthNow = nMonthOffseted%100;
    int nYearNow = nMonthOffseted/100;
    memset(szTemp, 0x00, sizeof(szTemp));
    strncpy(szTemp, pszDateTime+6, 2);
    int nDayNow = atoi(szTemp);
    if (nMonthNow == 2)
    {
        if (nDayNow > 28)
        {
            nDayNow = 28;
            if (nYearNow%4 == 0)
            {
                nDayNow = 29;
            }
        }
    }
    else
    {
        if (nDayNow > 30)
        {
            if (nMonthNow == 4 ||
                nMonthNow == 6 ||
                nMonthNow == 9 ||
                nMonthNow == 11 )
            {
                nDayNow = 30;
            }
        }
    }

    sprintf(szRetDate, "%d%02d%s", nMonthOffseted, nDayNow, pszDateTime+8);
    strncpy(pszRetValue,szRetDate,nSizeBuf);
    return nRet;
}

int CDate::IntervalMonth(const char *pszDateTime1, const char *pszDateTime2, int &nMonths)
{
    int nRet = 0;
    char szYear1[5] = "";
    char szYear2[5] = "";
    char szMonth1[3] = "";
    char szMonth2[3] = "";

    if(pszDateTime1 == NULL || pszDateTime2 == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if (strlen(pszDateTime1) < 6 || strlen(pszDateTime2) < 6)
    {
        //printf("入参不合法!\n");
        return -1;
    }

    szYear1[0]=pszDateTime1[0];
    szYear1[1]=pszDateTime1[1];
    szYear1[2]=pszDateTime1[2];
    szYear1[3]=pszDateTime1[3];
    szYear1[4]='\0';

    szYear2[0]=pszDateTime2[0];
    szYear2[1]=pszDateTime2[1];
    szYear2[2]=pszDateTime2[2];
    szYear2[3]=pszDateTime2[3];
    szYear2[4]='\0';

    szMonth1[0]=pszDateTime1[4];
    szMonth1[1]=pszDateTime1[5];
    szMonth1[2]='\0';

    szMonth2[0]=pszDateTime2[4];
    szMonth2[1]=pszDateTime2[5];
    szMonth2[2]='\0';

    //printf("%s%s,  %s%s",szYear1,szMonth1,szYear2,szMonth2);
    nMonths = atoi(szYear2)*12+atoi(szMonth2)-atoi(szYear1)*12-atoi(szMonth1);
    return nRet;
}

int CDate::IntervalDay(const time_t tBeginTime,const time_t tEndTime, int &nDays)
{
    int nRet = 0;
    time_t BeginTime ,EndTime;
    struct tm *ptTm = NULL, tmData;
//    struct tm *ptTm = localtime(&tBeginTime);
//    ptTm = localtime_zx(tBeginTime, &tmData);
    ptTm = _localtime(tBeginTime, &tmData);

    ptTm->tm_hour = 0;
    ptTm->tm_min = 0;
    ptTm->tm_sec = 0;
//    BeginTime = mktime(ptTm);
//    BeginTime = mktime_zx(ptTm);
    BeginTime = _mktime(ptTm);
    if (-1 == BeginTime)
    {
        ptTm = NULL;
        return -1;                                         
    }

    if (tEndTime == BeginTime)
    {
        // 如果是当天的开始时间则直接返回1
        nDays = 1;
        ptTm = NULL;
        return nRet;
    }
    else
    {
        // 转换DETERMINE_TIME
//        ptTm = localtime(&tEndTime);
//        ptTm = localtime_zx(tEndTime, &tmData);
    	ptTm = _localtime(tEndTime, &tmData);
        if (ptTm->tm_hour!=0 || ptTm->tm_min!=0 || ptTm->tm_sec!=0)
        {
            // 如果非当天的开始时间，则转换为第二天的开始时间
            ptTm->tm_hour = 0;
            ptTm->tm_min = 0;
            ptTm->tm_sec = 0;    
            ptTm->tm_mday++;         
        }
//        EndTime = mktime(ptTm);
//        EndTime = mktime_zx(ptTm);
        EndTime = _mktime(ptTm);
        if (-1 == EndTime)
        {
            ptTm = NULL;
            return -1;
        }           
    }

    // 判断转换后时间的有效性
    if (BeginTime > EndTime)
    {
        ptTm = NULL;
        return -1;
    }
    // 计算天数
    nDays = (EndTime-BeginTime) / (24*3600);
    ptTm = NULL;
    return nRet;
}

int CDate::IntervalSecond(const char *pszDateTime1, const char *pszDateTime2, long &lSeconds)
{
    int nRet = 0;
    time_t tTmp1;
    time_t tTmp2;
    if(pszDateTime1 == NULL || pszDateTime2 == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if (strlen(pszDateTime1) < DATE_TIME_LEN || strlen(pszDateTime2) < DATE_TIME_LEN)
    {
        //printf("入参不合法!\n");
        return -1;
    }
    nRet = FormatDateTimeStoT(pszDateTime2,tTmp2);
    if (nRet != 0)
    {
        //printf("调用函数FormatDateTimeStoT()失败!\n");
        return -1;
    }
    nRet = FormatDateTimeStoT(pszDateTime1,tTmp1);
    if (nRet != 0)
    {
        //printf("调用函数FormatDateTimeStoT()失败!\n");
        return -1;
    }
    lSeconds = (long)(tTmp1 - tTmp2);
    return nRet;
}

int CDate::FormatDateTimeStoL(const char *pszDateTime, char *pszRetDateTime)
{
    if (pszRetDateTime == NULL || pszDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if (strlen(pszDateTime) != DATE_TIME_LEN)
    {
        //printf("入参不合法!\n");
        return -1;
    }

    char szYear[5] = "";
    char szMonth[3] = "";
    char szDay[3] = "";

    strncpy(szYear,pszDateTime,4);
    szYear[4]='\0';

    strncpy(szMonth,pszDateTime+4,2);
    szMonth[2]='\0';

    strncpy(szDay,pszDateTime+6,2);
    szDay[2]='\0';


    char szHour[3] = "";
    char szMinute[3] = "";
    char szSec[3] = "";
    strncpy(szHour,pszDateTime+8,2);
    szHour[2]='\0';

    strncpy(szMinute,pszDateTime+10,2);
    szMinute[2]='\0';

    strncpy(szSec,pszDateTime+12,2);
    szSec[2]='\0';

    sprintf(pszRetDateTime,"%s-%s-%s %s:%s:%s",szYear,szMonth,szDay,szHour,szMinute,szSec);
    return 0;
}

int CDate::FormatDateTimeStoI(const char *pszDateTime,int &nRetValue)
{
    struct tm tm1;
    if (pszDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    tm1.tm_year=(pszDateTime[0]-'0')*1000+(pszDateTime[1]-'0')*100+(pszDateTime[2]-'0')*10+(pszDateTime[3]-'0')-1900;
    tm1.tm_mon =(pszDateTime[4]-'0')*10+(pszDateTime[5]-'0')-1;
    tm1.tm_mday =(pszDateTime[6]-'0')*10+(pszDateTime[7]-'0');

    tm1.tm_hour =(pszDateTime[8]-'0')*10+(pszDateTime[9]-'0');
    tm1.tm_min =(pszDateTime[10]-'0')*10+(pszDateTime[11]-'0');
    tm1.tm_sec =(pszDateTime[12]-'0')*10+(pszDateTime[13]-'0');
    tm1.tm_isdst =0;

//    nRetValue = (int)mktime(&tm1);
//    nRetValue = (int)mktime_zx(&tm1);
    nRetValue = (int)_mktime(&tm1);
    return nRetValue;
}

int CDate::FormatDateTimeStoT(const char *pszDateTime, time_t &tRetDateTime)
{
    struct tm tm1;
    if (pszDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    tm1.tm_year=(pszDateTime[0]-'0')*1000+(pszDateTime[1]-'0')*100+(pszDateTime[2]-'0')*10+(pszDateTime[3]-'0')-1900;
    tm1.tm_mon =(pszDateTime[4]-'0')*10+(pszDateTime[5]-'0')-1;
    tm1.tm_mday =(pszDateTime[6]-'0')*10+(pszDateTime[7]-'0');

    tm1.tm_hour =(pszDateTime[8]-'0')*10+(pszDateTime[9]-'0');
    tm1.tm_min =(pszDateTime[10]-'0')*10+(pszDateTime[11]-'0');
    tm1.tm_sec =(pszDateTime[12]-'0')*10+(pszDateTime[13]-'0');
    tm1.tm_isdst =0;


//    tRetDateTime = mktime(&tm1);
//    tRetDateTime = mktime_zx(&tm1);
    tRetDateTime = _mktime(&tm1);
    return 0;
}

int CDate::FormatStrToTime(const char *pszDateTime, time_t &tRetDateTime)
{
    char szYear[5] = "";    
    char szMonth[3] = "";
    char szDay[3] = "";
    struct tm tmCheck;
    int nYear = 0;
    int nMonth = 0;
    int nDay = 0;
    int nHour = 0;
    int nMinute = 0;
    int nSec = 0;

    if (pszDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    //使用localtime，以更新::daylight，使其和系统时区保持一致
    time_t tCurrent = 0;
    localtime(&tCurrent);
    strncpy(szYear,pszDateTime,4);
    szYear[4]='\0';
    nYear=atoi(szYear);
    strncpy(szMonth,pszDateTime+4,2);
    szMonth[2]='\0';
    nMonth=atoi(szMonth);
    strncpy(szDay,pszDateTime+6,2);
    szDay[2]='\0';
    nDay=atoi(szDay);
    strncpy(szDay, pszDateTime + 8, 2);
    nHour = atoi(szDay);
    strncpy(szDay, pszDateTime + 10, 2);
    nMinute = atoi(szDay);
    strncpy(szDay, pszDateTime + 12, 2);
    nSec = atoi(szDay);
    tmCheck.tm_year = nYear - 1900;
    tmCheck.tm_mon  = nMonth - 1;
    tmCheck.tm_mday = nDay;
    tmCheck.tm_hour = nHour;
    tmCheck.tm_min  = nMinute;
    tmCheck.tm_sec  = nSec;
    tmCheck.tm_isdst = ::daylight;
//    tRetDateTime=mktime(&tmCheck);
//    tRetDateTime=mktime_zx(&tmCheck);
    tRetDateTime=_mktime(&tmCheck);
    return 0;
}

int CDate::FormatDateTimeTtoL(const time_t tDateTime,char *pszDateTime)
{
    if (pszDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    time_t tCurrent = tDateTime;
    struct tm tmData;
//    struct tm *ptmDate = localtime(&tCurrent);
//    struct tm *ptmDate = localtime_zx(tCurrent, &tmData);
    struct tm *ptmDate = _localtime(tCurrent, &tmData);
    sprintf(pszDateTime,"%04d-%02d-%02d %02d:%02d:%02d",ptmDate->tm_year+1900,ptmDate->tm_mon+1,ptmDate->tm_mday,
        ptmDate->tm_hour,ptmDate->tm_min,ptmDate->tm_sec);
    ptmDate = NULL;
    return 0;
}

int CDate::FormatDateTimeTtoS(const time_t tDateTime,char *pszDateTime)
{
    if (pszDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    time_t tCurrent = tDateTime;
    struct tm tmData;
//    struct tm *ptmDate = localtime(&tCurrent);
//    struct tm *ptmDate = localtime_zx(tCurrent, &tmData);
    struct tm *ptmDate = _localtime(tCurrent, &tmData);
    sprintf(pszDateTime,"%04d%02d%02d%02d%02d%02d",ptmDate->tm_year+1900,ptmDate->tm_mon+1,ptmDate->tm_mday,
        ptmDate->tm_hour,ptmDate->tm_min,ptmDate->tm_sec);
//#ifdef _THREAD_SAFE_
//    strcat(pszDateTime, "_s");
//#endif
    ptmDate = NULL;
    return 0;
}

int CDate::FormatDateTimeLtoT(const char *pszDateTime, time_t &tRetDateTime,const bool bLongFlag/* =true */)
{
    struct tm tmDate;
    char szTemptime[20] = "";
    char sztime1[20] = "";
    if (pszDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if ((bLongFlag && strlen(pszDateTime) < DATE_TIME_LONG_LEN) || 
        (!bLongFlag && strlen(pszDateTime) < 10))
    {
        //printf("入参不合法\n");
        return -1;
    }
    strncpy(sztime1,pszDateTime,20);
    sztime1[19] = '\0';
    strncpy(szTemptime,pszDateTime,4);
    szTemptime[4]='\0';
    tmDate.tm_year=atoi(szTemptime)-1900;

    strncpy(szTemptime,sztime1+5,2);
    szTemptime[2]='\0';
    tmDate.tm_mon = atoi(szTemptime)-1;

    strncpy(szTemptime,sztime1+8,2);
    szTemptime[2]='\0';
    tmDate.tm_mday = atoi(szTemptime);

    if (bLongFlag)
    {
        strncpy(szTemptime,sztime1+11,2);
        szTemptime[2]='\0';
        tmDate.tm_hour = atoi(szTemptime);

        strncpy(szTemptime,sztime1+14,2);
        szTemptime[2]='\0';
        tmDate.tm_min = atoi(szTemptime);

        strncpy(szTemptime,sztime1+17,2);
        szTemptime[2]='\0';
        tmDate.tm_sec = atoi(szTemptime);
    }
    else
    {
        tmDate.tm_hour = 0;
        tmDate.tm_min = 0;
        tmDate.tm_sec = 0;
    }

//    tRetDateTime = mktime(&tmDate);
//    tRetDateTime = mktime_zx(&tmDate);
    tRetDateTime = _mktime(&tmDate);
    return 0;
}

int CDate::FormatDateTimeLtoS(const char *pszDateTime, char *pszRetDateTime, const int nDateTimeFlag/* =0 */)
{
    struct tm tmDate;
    char szTemptime[20] = "";
    char sztime1[20] = "";
    if (pszDateTime == NULL || pszRetDateTime == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if (strlen(pszDateTime) < DATE_TIME_LONG_LEN)
    {
        //printf("入参不合法\n");
        return -1;
    }
    strncpy(sztime1,pszDateTime,20);
    sztime1[19] = '\0';
    strncpy(szTemptime,pszDateTime,4);
    szTemptime[4]='\0';
    tmDate.tm_year=atoi(szTemptime)-1900;

    strncpy(szTemptime,sztime1+5,2);
    szTemptime[2]='\0';
    tmDate.tm_mon = atoi(szTemptime)-1;

    strncpy(szTemptime,sztime1+8,2);
    szTemptime[2]='\0';
    tmDate.tm_mday = atoi(szTemptime);

    strncpy(szTemptime,sztime1+11,2);
    szTemptime[2]='\0';
    tmDate.tm_hour = atoi(szTemptime);

    strncpy(szTemptime,sztime1+14,2);
    szTemptime[2]='\0';
    tmDate.tm_min = atoi(szTemptime);

    strncpy(szTemptime,sztime1+17,2);
    szTemptime[2]='\0';
    tmDate.tm_sec = atoi(szTemptime);

    if (nDateTimeFlag == 1)
    {
        sprintf(pszRetDateTime,"%4.4d%2.2d%2.2d",
            tmDate.tm_year + 1900,
            tmDate.tm_mon + 1,
            tmDate.tm_mday);
    }
    else if (nDateTimeFlag == 2)
    {
        sprintf(pszRetDateTime,"%2.2d%2.2d%2.2d",
            tmDate.tm_hour,
            tmDate.tm_min,
            tmDate.tm_sec);
    }
    else
    {
        sprintf(pszRetDateTime,"%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d",
            tmDate.tm_year + 1900,
            tmDate.tm_mon + 1,
            tmDate.tm_mday,
            tmDate.tm_hour,
            tmDate.tm_min,
            tmDate.tm_sec);
    }
    return 0;
}

bool CDate::IfLeapYear(char* pszYear)
{
    if (pszYear == NULL)
    {
        //printf("传入指针为空!\n");
        return false;
    }
    int nYear = atoi(pszYear);
    if ( ( nYear%4 == 0 && nYear%100 !=0) || nYear %400 == 0)
    {
        return true;
    }

    return false;
}

int CDate::GetYearBegin(char *pszDate, char *pszRetDate)
{
    if (pszDate == NULL || pszRetDate == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if (strlen(pszDate) < 4)
    {
        //printf("参数不合法!\n");
        return -1;
    }
    char szTmp[5] = "";
    memset(szTmp,0x00,sizeof(szTmp));
    strncpy(szTmp,pszDate,4);
    sprintf(pszRetDate,"%s0101",szTmp);
    return 0;
}

time_t CDate::GetYearBegin(time_t tDate)
{
    struct tm *ptm = NULL, tmData;
//    ptm = localtime(&tDate);
//    ptm = localtime_zx(tDate, &tmData);
    ptm = _localtime(tDate, &tmData);
    ptm->tm_mon = 1;
    ptm->tm_mday = 1;
    ptm->tm_hour = 0;
    ptm->tm_min = 0;
    ptm->tm_sec = 0;
    ptm = NULL;
//    return mktime(ptm);
//    return mktime_zx(ptm);
    return _mktime(ptm);
}

int CDate::GetYearEnd(char *pszDate, char *pszRetDate)
{
    if (pszDate == NULL || pszRetDate == NULL)
    {
        //printf("传入指针为空!\n");
        return -1;
    }
    if (strlen(pszDate) < 4)
    {
        //printf("参数不合法!\n");
        return -1;
    }
    char szTmp[5] = "";
    memset(szTmp,0x00,sizeof(szTmp));
    strncpy(szTmp,pszDate,4);
    sprintf(pszRetDate,"%s1231",szTmp);
    return 0;
}

time_t CDate::GetYearEnd(time_t tDate)
{
    struct tm *ptm = NULL, tmData;
//    ptm = localtime(&tDate);
//    ptm = localtime_zx(tDate, &tmData);
    ptm = _localtime(tDate, &tmData);
    ptm->tm_mon = 12;
    ptm->tm_mday = 31;
    ptm->tm_hour = 0;
    ptm->tm_min = 0;
    ptm->tm_sec = 0;
    ptm = NULL;
//    return mktime(ptm);
//    return mktime_zx(ptm);
    return _mktime(ptm);
}

const char * CDate::FormatGMTimeToString(const time_t tTime,const bool bLongFlag)
{
    static char szCurtime[32];
    struct tm *tm_Cur, tmData;

    szCurtime[31]=0;
//    tm_Cur = gmtime(&tTime);
//    tm_Cur = localtime_zx(tTime, &tmData);
    tm_Cur = _localtime(tTime, &tmData);
    if(bLongFlag)
    {
        sprintf(szCurtime,"%04d-%02d-%02d %02d:%02d:%02d",tm_Cur->tm_year+1900,tm_Cur->tm_mon+1,tm_Cur->tm_mday,tm_Cur->tm_hour,tm_Cur->tm_min,tm_Cur->tm_sec);
    }
    else
    {
        sprintf(szCurtime,"%04d%02d%02d%02d%02d%02d",tm_Cur->tm_year+1900,tm_Cur->tm_mon+1,tm_Cur->tm_mday,tm_Cur->tm_hour,tm_Cur->tm_min,tm_Cur->tm_sec);
    }

    return szCurtime;
}

struct tm *localtime_zx(time_t time, struct tm *ret_time)
{
	static const char month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	static const bool leap_year[4] = { false, false, true, false };

	unsigned int leave_for_fouryear = 0;
	unsigned short four_year_count = 0;
	unsigned int temp_value = 0;

	ret_time->tm_sec = time % 60;
	temp_value = time / 60;/* 分钟 */
	ret_time->tm_min = temp_value % 60;
	temp_value /= 60; /* 小时 */

	temp_value += 8; /* 加上时区 */

	ret_time->tm_hour = temp_value % 24;
	temp_value /= 24; /* 天 */

	ret_time->tm_wday = (temp_value + 4) % 7;/* 1970-1-1是4 */

	four_year_count = temp_value / (365 * 4 + 1);
	leave_for_fouryear = temp_value % (365 * 4 + 1);
	int leave_for_year_days = leave_for_fouryear;

	int day_count = 0;
	int i = 0;

	for(i = 0; i < 4; i++)
	{
		day_count = leap_year[i] ? 366 : 365;
		if(leave_for_year_days <= day_count)
			break;
		else
			leave_for_year_days -= day_count;
	}

	ret_time->tm_year = four_year_count * 4 + i + 70;
	ret_time->tm_yday = leave_for_year_days;/* 这里不是天数，而是标记，从0开始 */

	int leave_for_month_days = leave_for_year_days;

	int j = 0;
	for(j = 0; j < 12; j++)
	{
		if(leap_year[i] && j == 1)
		{
			if(leave_for_month_days < 29)
			{
				break;
			} else if(leave_for_month_days == 29)
			{
				j++;
				leave_for_month_days = 0;
				break;
			} else
			{
				leave_for_month_days -= 29;
			}

			continue;
		}

		if(leave_for_month_days < month_days[j])
		{
			break;
		} else if(leave_for_month_days == month_days[j])
		{
			j++;
			leave_for_month_days = 0;
			break;
		} else
		{
			leave_for_month_days -= month_days[j];
		}
	}
    
	ret_time->tm_mday = leave_for_month_days + 1;
	ret_time->tm_mon = j;
	if(ret_time->tm_mon == 12 )
	{
		ret_time->tm_mon = 0;
		ret_time->tm_year++;
	}
	return ret_time;
}

time_t mktime_zx(struct tm *tm)
{
#define MINUTE	60   /*此处定义宏,都是秒为单位*/
#define HOUR	(60*MINUTE)
#define DAY		(24*HOUR)
#define YEAR	(365*DAY) /*注意此处按平年计算*/

	static int month[12] =
	{/*定义截止到每个月份所含秒数,都按二月29天算的*/
		0,
		DAY*(31),
		DAY*(31+29),
		DAY*(31+29+31),
		DAY*(31+29+31+30),
		DAY*(31+29+31+30+31),
		DAY*(31+29+31+30+31+30),
		DAY*(31+29+31+30+31+30+31),
		DAY*(31+29+31+30+31+30+31+31),
		DAY*(31+29+31+30+31+30+31+31+30),
		DAY*(31+29+31+30+31+30+31+31+30+31),
		DAY*(31+29+31+30+31+30+31+31+30+31+30)
	};
	time_t res;
	int year = tm->tm_year - 70; /*从1970年开始算(千年虫BUG?)*/
	/* magic offsets (y+1) needed to get leapyears right.*/
	res = YEAR * year + DAY * ((year + 1) / 4); /*过去多少年以及其中有多少瑞年(补多少天)*/
	res += month[tm->tm_mon];
	/* and (y+2) here. If it wasn't a leap-year, we have to adjust */
	if(tm->tm_mon > 1 && ((year + 2) % 4)) /*判断几年是否为瑞年,且是否已经过了二月*/
		res -= DAY; /*注意tm->tm_mon是0-11代表1-12月*/
	res += DAY * (tm->tm_mday - 1);
	res += HOUR * tm->tm_hour - 8 * HOUR; /*减去时区差值8*/
	res += MINUTE * tm->tm_min;
	res += tm->tm_sec; /*返回从1970年开始到现在流失的秒数*/
	return res;
}

struct tm *localtimes(time_t time, long timezone, struct tm *tm_time)
{
	const char Days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	unsigned int leap_year;
	time = time - timezone;/*计算时差*/
	if(time < 0)
		time = 0;
	tm_time->tm_sec = (int)(time % 60);/*取秒时间*/
	time /= 60;
	tm_time->tm_min = (int)(time % 60);/*取分钟时间*/
	time /= 60;
	tm_time->tm_year = ((time / (1461L * 24L)) << 2) + 70;/*计算年份, 每四年有 1461*24 小时*/
	time %= 1461L * 24L;/*四年中剩下的小时数*/
	for(;;)
	{/*校正闰年影响的年份，计算一年中剩下的小时数*/
		leap_year = 365 * 24;/*一年的小时数*/
		/*判断闰年*/
		if((tm_time->tm_year & 3) == 0)/*是闰年，一年则多24小时，即一天*/
			leap_year += 24;
		if(time < leap_year)
			break;
		time -= leap_year;
	}
	tm_time->tm_hour = (int)(time % 24);/*小时数*/
	time /= 24;/*一年中剩下的天数*/
	if((tm_time->tm_year & 3) == 0)
	{/*校正润年的误差，计算月份，日期*/
		if(time > 60)
		{
			time--;
		} else
		{
			if(time == 60)
			{
				tm_time->tm_mon = 1;
				tm_time->tm_mday = 29;
				return tm_time;
			}
		}
	}
	/*计算月日*/
	for(tm_time->tm_mon = 0; Days[tm_time->tm_mon] < time; tm_time->tm_mon)
		time -= Days[tm_time->tm_mon];
	tm_time->tm_mday = (int)(time);
	return tm_time;
}

