// FreeLongLog.cpp: implementation of the CFreeLongLog class.
//
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
#include "stdafx.h"
#else
#endif
#include "FreeLongLog.h"
#include "SafePrint.h"
#include <string.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define CONDEBUG 1
#ifdef CONDEBUG
#define CON_PRINTF printf
#else
#define CON_PRINTF /\
/printf
#endif

#define FREELONG_CLEAN_CHAR_BUFFER(p) (*((char*)p)='\0')

CFreeLongLog::CFreeLongLog(
        char* szLogPath,
        char* szAppName,
        int nHoldFileMax,
        bool bSyslogFlag,
        bool bDebugFlag,
        bool bDebug2Flag,
        bool bDebug3Flag,
        bool bPrint2ScrFlag)
{
    //基准名称
    FULL_NAME(szLogPath, szAppName, m_szFilePath, "");


    //保存文件名
    FULL_NAME(szLogPath, szAppName, m_szFileInfoName, "info");

    //清空当前文件名缓冲区
    FREELONG_CLEAN_CHAR_BUFFER(m_szFileName);

    //当前文件尺寸设置
    m_nFileSize = 0;
    m_bSyslogFlag = bSyslogFlag;
    m_bDebugFlag = bDebugFlag;
    m_bDebug2Flag = bDebug2Flag;
    m_bDebug3Flag = bDebug3Flag;
    m_bPrint2ScrFlag = bPrint2ScrFlag;
    m_nHoleFileMax = nHoldFileMax;
    if (m_nHoleFileMax >= LOG_FILE_DEFAULT_HOLD)
    {
        m_nHoleFileMax = LOG_FILE_DEFAULT_HOLD - 1;
    }


    m_nFileNameIndex = 0;
    for (int i = 0; i < LOG_FILE_DEFAULT_HOLD; i++)
    {
        m_chFileName[i] = new char[256];
    }


    InitFileInfo();
    MakeFileName();

}

CFreeLongLog::~CFreeLongLog()
{
    for (int i = 0; i < LOG_FILE_DEFAULT_HOLD; i++)
    {
        delete [] m_chFileName[i];
    }
}

void CFreeLongLog::GetFileName()
{

    time_t tNow;
    unsigned long ulDeltaT = 0;
    if (m_szFileName[0] == '\0') //第一次启动，文件名为空
    {
        MakeFileName();
        goto CFreeLongLog_GetFileName_End_Process;

    }

    time(&tNow);

    ulDeltaT = (unsigned long) tNow - m_tFileNameMake; //计算时间差
    if (LOG_FILE_CHANGE_NAME_PRE_SECONDS <= ulDeltaT)
    {
        MakeFileName(); //时间超过３６００秒
        goto CFreeLongLog_GetFileName_End_Process;
    }

    if (LOG_FILE_SIZE_MAX <= m_nFileSize)
    {
        MakeFileName(); //大小超过１G
        goto CFreeLongLog_GetFileName_End_Process;
    }

CFreeLongLog_GetFileName_End_Process:
    return;
}

void CFreeLongLog::MakeFileName()
{

    char szTemp[LOG_ITEM_LENGTH_MAX];
    MakeATimeString(szTemp, LOG_ITEM_LENGTH_MAX);

    FixFileInfo();

    int nLen = SafePrint(
            m_szFileName,
            FILENAME_STRING_LENGTH * 2,
            "%s_%s.log",
            m_szFilePath,
            szTemp);

    nLen++;

    //将新的文件名增加到队列
    int nSize = m_FileNameVec.size();
    strcpy(m_chFileName[m_nFileNameIndex], m_szFileName);
    m_FileNameVec.push_back(m_chFileName[m_nFileNameIndex]);
    m_nFileNameIndex++;
    if (m_nFileNameIndex > LOG_FILE_DEFAULT_HOLD - 1)
    {
        m_nFileNameIndex = 0;
    }

    FILE* fp = fopen(m_szFileInfoName, "wb"); //wb的形式打开文件，都将以前的记录覆盖
    if (fp)
    {
        int nSize = m_FileNameVec.size();
        fwrite((void*) & nSize, sizeof (char), 4, fp);

        for (int i = 0; i < m_FileNameVec.size(); i++)
        {
            char* chFileName = m_FileNameVec[i];
            int nNameLen = strlen(chFileName) + 1;
            fwrite((void*) & nNameLen, sizeof (char), 4, fp);
            fwrite((void*) chFileName, sizeof (char), nNameLen, fp);
        }

        fclose(fp);
    }


    m_nFileSize = 0;
    time(&m_tFileNameMake);
    {
        //由于这里是非业务打印，不希望输出到屏幕，先把屏幕开关关闭
        bool bPrint2Src = m_bPrint2ScrFlag;
        m_bPrint2ScrFlag = false;
        m_bPrint2ScrFlag = bPrint2Src; //恢复屏幕开关
    }
}

int CFreeLongLog::MakeATimeString(char* szBuffer, int nBufferSize)
{
    int i = 0;
    time_t t;
    struct tm *pTM = NULL;

    int nLength = 0;
    if (LOG_TIME_STRING_MAX > nBufferSize)
    {
        goto CFreeLongLog_MakeATimeString_End_Process;

    }

    time(&t);
    pTM = localtime(&t);
    nLength = SafePrint(szBuffer, LOG_ITEM_LENGTH_MAX, "%04d%02d%02d_%02d%02d%02d",
            pTM->tm_year + 1900,
            pTM->tm_mon + 1,
            pTM->tm_mday,
            pTM->tm_hour,
            pTM->tm_min,
            pTM->tm_sec);

    szBuffer[nLength - 1] = '\0';

    for (i = 0; i < nLength; i++)
    {
        if (szBuffer[i] == ' ')
        {
            szBuffer[i] = '_';
        }

        if (szBuffer[i] == ':')
        {
            szBuffer[i] = '_';
        }
    }


CFreeLongLog_MakeATimeString_End_Process:
    return nLength;
}

void CFreeLongLog::FixFileInfo()
{
    int nAddLastRet = 0;
    while (m_FileNameVec.size() > m_nHoleFileMax - 1)
    {
        DeleteFirstFile();
    }
}

void CFreeLongLog::DeleteFirstFile()
{
    char szFirstFile[FILENAME_STRING_LENGTH];


    std::vector<char*>::iterator iter = m_FileNameVec.begin();
    try
    {
        strcpy(szFirstFile, (char*) * iter);
        remove(szFirstFile);
    }
    catch (...)
    {

    }

    m_FileNameVec.erase(iter);

    return;
}

int CFreeLongLog::_XGSysLog(char* szFormat, ...)
{
    char szBuf[LOG_ITEM_LENGTH_MAX];
    int nMaxLength = LOG_ITEM_LENGTH_MAX;
    int nListCount = 0;
    va_list pArgList;

    va_start(pArgList, szFormat);
    nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
    va_end(pArgList);

    if (nListCount > (nMaxLength - 1))
    {
        nListCount = nMaxLength - 1;
    }

    *(szBuf + nListCount) = '\0';

    if (m_bSyslogFlag)
    {
        m_Lock.Lock();
        {
            _Printf("%s", szBuf);
        }
        m_Lock.Unlock();
    }

    return nListCount;
}

int CFreeLongLog::_XGDebug(char* szFormat, ...)
{

    char szBuf[LOG_ITEM_LENGTH_MAX];
    int nMaxLength = LOG_ITEM_LENGTH_MAX;
    int nListCount = 0;
    va_list pArgList;

    va_start(pArgList, szFormat);
    nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
    va_end(pArgList);

    if (nListCount > (nMaxLength - 1))
    {
        nListCount = nMaxLength - 1;
    }

    *(szBuf + nListCount) = '\0';

    if (m_bDebugFlag)
    {
        m_Lock.Lock();
        {
            _Printf("%s", szBuf);
        }
        m_Lock.Unlock();
    }

    return nListCount;

}

int CFreeLongLog::_XGDebug2(char* szFormat, ...)
{
    char szBuf[LOG_ITEM_LENGTH_MAX];
    int nMaxLength = LOG_ITEM_LENGTH_MAX;
    int nListCount = 0;
    va_list pArgList;

    va_start(pArgList, szFormat);
    nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
    va_end(pArgList);

    if (nListCount > (nMaxLength - 1))
    {
        nListCount = nMaxLength - 1;
    }

    *(szBuf + nListCount) = '\0';

    if (m_bDebug2Flag)
    {
        m_Lock.Lock();
        {
            _Printf("%s", szBuf);
        }
        m_Lock.Unlock();
    }

    return nListCount;

}

int CFreeLongLog::_XGDebug3(char* szFormat, ...)
{
    char szBuf[LOG_ITEM_LENGTH_MAX];
    int nMaxLength = LOG_ITEM_LENGTH_MAX;
    int nListCount = 0;
    va_list pArgList;

    va_start(pArgList, szFormat);
    nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
    va_end(pArgList);

    if (nListCount > (nMaxLength - 1))
    {
        nListCount = nMaxLength - 1;
    }

    *(szBuf + nListCount) = '\0';

    if (m_bDebug3Flag)
    {
        m_Lock.Lock();
        {
            _Printf("%s", szBuf);
        }
        m_Lock.Unlock();
    }

    return nListCount;

}

int CFreeLongLog::_Printf(char* szFormat, ...)
{
    char szTime[LOG_ITEM_LENGTH_MAX];
    char szTemp[LOG_ITEM_LENGTH_MAX];
    char szBuf[LOG_ITEM_LENGTH_MAX];

    int nMaxLength = LOG_ITEM_LENGTH_MAX;
    int nListCount = 0;

    time_t t;
    struct tm *pTM = NULL;
    int nLength = 0;

    time(&t);
    pTM = localtime(&t);
    nLength = SafePrint(szTemp, LOG_ITEM_LENGTH_MAX, "%s", asctime(pTM));
    szTemp[nLength - 1] = '\0';

    SafePrint(szTime, LOG_ITEM_LENGTH_MAX, "[%s]", szTemp);


    va_list pArgList;

    va_start(pArgList, szFormat);
    nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
    va_end(pArgList);

    if (nListCount > (nMaxLength - 1))
    {
        nListCount = nMaxLength - 1;
    }

    *(szBuf + nListCount) = '\0';


    //获取文件名
    GetFileName();
    //	nListCount=dbg2file(m_szFileName,"a+","%s%s",szTime,szBuf);
    nListCount = dbg2file(m_szFileName, "a+", "%s", szBuf);

    m_nFileSize += nListCount;

    return nListCount;

}

void CFreeLongLog::_XGDebug4Bin(char* pBuffer, int nLength)
{
    m_Lock.Lock();
    {
        GetFileName();
        //		dbg2file4bin(m_szFileName,"a+",pBuffer,nLength);
        dbg_bin(pBuffer, nLength);
    }
    m_Lock.Unlock();
}

int CFreeLongLog::InitFileInfo()
{


    FILE* fp = fopen(m_szFileInfoName, "rb"); //wb的形式打开文件，都将以前的记录覆盖
    if (fp)
    {
        int nSize = 0;
        fread((void*) & nSize, sizeof (char), 4, fp);

        for (int i = 0; i < nSize; i++)
        {
            int nNameLen = 0;
            fread((void*) & nNameLen, sizeof (char), 4, fp);
            fread((void*) m_chFileName[m_nFileNameIndex], sizeof (char), nNameLen, fp);
            m_FileNameVec.push_back(m_chFileName[m_nFileNameIndex]);

            m_nFileNameIndex++;
            if (m_nFileNameIndex > LOG_FILE_DEFAULT_HOLD - 1)
            {
                m_nFileNameIndex = 0;
            }
        }

        fclose(fp);
    }

    return 0;
}
