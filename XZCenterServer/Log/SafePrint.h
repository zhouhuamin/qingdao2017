#if !defined _SAFE_PRINT_H_
#define _SAFE_PRINT_H_
#include "SysInclude.h"
#include <string.h>
#include <stdio.h>
#ifdef WIN32
#define Linux_Win_vsnprintf _vsnprintf
#else
#define Linux_Win_vsnprintf vsnprintf
#endif

#define CONDEBUG 1
#ifdef CONDEBUG
#define CON_PRINTF printf
#else
#define CON_PRINTF /\
/printf
#endif

#define DEBUG_BUFFER_LENGTH            1024                         //最大缓冲区大小

int inline SafePrint(char* szBuf, int nMaxLength, char* szFormat, ...)
{
    int nListCount = 0;
    va_list pArgList;
    //对输入的指针进行有效性检查
    if (!szBuf)
    {
        goto SafePrint_END_PROCESS;
    }

    va_start(pArgList, szFormat);
    nListCount += Linux_Win_vsnprintf(szBuf + nListCount, nMaxLength - nListCount, szFormat, pArgList);
    va_end(pArgList);

    //进行缓冲区越界检测
    if (nListCount > (nMaxLength - 1))
    {
        nListCount = nMaxLength - 1;
    }

    *(szBuf + nListCount) = '\0';

SafePrint_END_PROCESS:
    return nListCount;
}




#ifdef WIN32
#define PATH_CHAR "\\"
#else
#define PATH_CHAR "/"
#endif

#define FILENAME_STRING_LENGTH 256                         //统一文件名长度

//注意参数的表意，依次为路径，文件名，缓冲区，扩展名

int inline FULL_NAME(char* path, char* name, char* fullname, char* ext_name)
{
    if (strlen(path))
    {
        if (strlen(ext_name))
        {
            return SafePrint(fullname,
                    FILENAME_STRING_LENGTH,
                    "%s%s%s.%s",
                    path,
                    PATH_CHAR,
                    name,
                    ext_name);
        }
        else
        {
            return SafePrint(fullname,
                    FILENAME_STRING_LENGTH,
                    "%s%s%s",
                    path,
                    PATH_CHAR,
                    name);
        }
    }
    else
    {
        if (strlen(ext_name))
        {
            return SafePrint(fullname,
                    FILENAME_STRING_LENGTH,
                    "%s.%s",
                    name,
                    ext_name);
        }
        else
        {
            return SafePrint(fullname,
                    FILENAME_STRING_LENGTH,
                    "%s",
                    name);
        }
    }
}

static int dbg_bin_acsii(char* pPrintBuffer, char* pBuffer, int nLength)
{
    int i = 0;
    int nCount = 0;
    for (i = 0; i < nLength; i++)
    {
        //ASCII字符表中，可显示字符代码>32
        if (*(pBuffer + i) >= 32)
        {
            nCount += SafePrint(pPrintBuffer + nCount, 256, "%c", *(pBuffer + i));
        }
        else
        {
            //不可显示字符代码以"."代替占位，保持格式整齐，避免出错
            nCount += SafePrint(pPrintBuffer + nCount, 256, ".");
        }
    }

    return nCount;
}

static int dbg_bin_hex(char* pPrintBuffer, char* pBuffer, int nLength)
{
    int i = 0;
    int j = 0;
    int nCount = 0;
    for (i = 0; i < nLength; i++)
    {

        nCount += SafePrint(pPrintBuffer + nCount, 256, "%02X", (unsigned char) *(pBuffer + i));
        //ASCII字符表中，可显示字符代码>32
        j++;
        if (j == 4)
        {
            j = 0;
            nCount += SafePrint(pPrintBuffer + nCount, 256, " ");
        }
    }

    if (16 > nLength) //每行打印16个在字节
    {
        for (; i < 16; j++)
        {
            nCount += SafePrint(pPrintBuffer + nCount, 256, "  ");
            j++;

            if (j == 4)
            {
                j = 0;
                nCount += SafePrint(pPrintBuffer + nCount, 256, " ");
            }

        }
    }

    return nCount;
}

void inline dbg_bin(char* pBuffer, int nLength)
{
    int nAddr = 0;
    int nLineCount = 0;
    int nBufferCount = nLength;

    int n = 0;
    char szLine[256];
    if (nLength > 0)
    {
        while (1)
        {
            n = 0;
            n += SafePrint(szLine + n, 256 - n, "%p-", pBuffer + nAddr);
            nLineCount = 16;
            if (nBufferCount < nLineCount)
            {
                nLineCount = nBufferCount;
            }

            n += dbg_bin_hex(szLine + n, pBuffer + nAddr, nLineCount);
            n += dbg_bin_acsii(szLine + n, pBuffer + nAddr, nLineCount);

            CON_PRINTF("%s\n", szLine);

            nAddr += 16;
            nBufferCount -= 16;
            if (nBufferCount <= 0)
            {
                break;
            }
        }

        CON_PRINTF("\n");

    }
    else
    {
        CON_PRINTF("dbg_bin error length=%d\n", nLength);
    }
}

int inline GetATimeStamp(char* szBuf, int nMaxLength)
{
    if (!szBuf)
    {
        return -1;
    }

    strcpy(szBuf, "");

#ifdef WIN32
    SYSTEMTIME tTime;
    GetLocalTime(&tTime);
    int nLen = sprintf(szBuf, "%04d-%02d-%02d %02d:%02d:%02d--%06d",
            tTime.wYear,
            tTime.wMonth,
            tTime.wDay,

            tTime.wHour,
            tTime.wMinute,
            tTime.wSecond,

            tTime.wMilliseconds * 1000
            );
    if (nLen > nMaxLength)
    {
        nLen = nMaxLength;
        szBuf[nLen - 1] = '\0';
    }

#else  
    struct tm* tmNow;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    tmNow = localtime(&tv.tv_sec);
    int nLen = sprintf(szBuf, "%04d-%02d-%02d %02d:%02d:%02d--%06d",
            tmNow->tm_year + 1900,
            tmNow->tm_mon + 1,
            tmNow->tm_mday,
            tmNow->tm_hour,
            tmNow->tm_min,
            tmNow->tm_sec,
            tv.tv_usec
            );
    if (nLen > nMaxLength)
    {
        nLen = nMaxLength;
        szBuf[nLen - 1] = '\0';
    }

#endif

    return 0;
}

int inline dbg2file(char* szFileName, char* szMode, char* szFormat, ...)
{
    char szBuf[DEBUG_BUFFER_LENGTH];
    char szTime[256];

    int nListCount = 0;
    va_list pArgList;
    va_start(pArgList, szFormat);
    nListCount += Linux_Win_vsnprintf(szBuf + nListCount, DEBUG_BUFFER_LENGTH - nListCount, szFormat, pArgList);
    va_end(pArgList);

    //进行缓冲区越界检测
    if (nListCount > (DEBUG_BUFFER_LENGTH - 1))
    {
        nListCount = DEBUG_BUFFER_LENGTH - 1;
    }

    *(szBuf + nListCount) = '\0';

    GetATimeStamp(szTime, 256);

    FILE* fp = fopen(szFileName, szMode);
    if (fp)
    {
        nListCount = fprintf(fp, "[%s]%s", szTime, szBuf);
        CON_PRINTF("[%s]%s", szTime, szBuf);
        fclose(fp);

    }
    else
    {
        nListCount = 0;
    }

    return nListCount;
}

void inline SafeStrcpy(char* pDest, char* pSource, int nCount)
{
    int nLen = (int) strlen(pSource) + 1;

    if (!pDest)
    {
        goto SafeStrcpy_END_PROCESS;
    }
    if (!pSource)
    {
        goto SafeStrcpy_END_PROCESS;
    }

    if (nLen > nCount)
    {
        nLen = nCount;
    }

    memcpy(pDest, pSource, nLen);
    *(pDest + nLen - 1) = '\0';



SafeStrcpy_END_PROCESS:
    return;
}


#endif
