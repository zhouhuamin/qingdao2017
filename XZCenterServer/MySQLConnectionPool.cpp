// MySQLConnectionPool.cpp: implementation of the CMySQLConnectionPool class.
//
//////////////////////////////////////////////////////////////////////
#include "MySQLConnectionPool.h"
#include "MutexGuard.h"
#include "MyLog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CppMySQL3DB* CMySQLConnectionPool::m_pDatabase[MAX_DATABASE_CONNECTION];
int CMySQLConnectionPool::m_nDataIndex = 0;
int CMySQLConnectionPool::m_nTestDataIndex = 0;
CSysMutex CMySQLConnectionPool::access_lock_;
bool CMySQLConnectionPool::m_bInit = false;
int CMySQLConnectionPool::m_nPort = 3306;

char CMySQLConnectionPool::m_szHost[64] = {0};
char CMySQLConnectionPool::m_szUser[64] = {0};
char CMySQLConnectionPool::m_szPass[64] = {0};
char CMySQLConnectionPool::m_szDB[64] = {0};

CMySQLConnectionPool::CMySQLConnectionPool()
{

}

CMySQLConnectionPool::~CMySQLConnectionPool()
{

}

int CMySQLConnectionPool::GetDataAccess(CppMySQL3DB *&pDatabase)
{
    pDatabase = NULL;
    CMutexGuard guard(access_lock_);

    int m_nSavedDataIndex = m_nDataIndex;
    for (;;)
    {
        CppMySQL3DB* pDataAccess = m_pDatabase[m_nDataIndex];

        if (pDataAccess->isConnect())
        {
            if (!pDataAccess->isBusy())
            {
                pDatabase = pDataAccess;
                break;
            }
        }

        m_nDataIndex++;
        if (m_nDataIndex > MAX_DATABASE_CONNECTION - 1)
        {
            m_nDataIndex = 0;
        }

        if (m_nSavedDataIndex == m_nDataIndex)
        {
            break; //overlapped
        }
    }

    if (pDatabase)
    {
        pDatabase->setBusy();
    }

    //printf("------------access_lock_ GetDataAccess out\n");
    return 0;
}



#ifdef WIN32

unsigned __stdcall DatabaseInitFunc(void* pArg)
{
    Sleep(60 * 1000);

    while (1)
    {
        CMySQLConnectionPool::TestDatabase();
        ;
        Sleep(120 * 1000);
    }

    _endthreadex(0);
    return 0;
}
#else


extern "C"
{
    typedef void*(*THREADFUNC)(void*);
}

void* DatabaseInitFunc(void * pParam)
{

    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    select(0, 0, NULL, NULL, &tv);

    while (1)
    {
        CMySQLConnectionPool::TestDatabase();

        struct timeval tv;
        tv.tv_sec = 120;
        tv.tv_usec = 0;
        select(0, 0, NULL, NULL, &tv);
    }

    pthread_exit(NULL);
    return 0;
}




#endif

void CMySQLConnectionPool::Init(const char* host, const char* user, const char* passwd, const char* db, int nPort)
{
    for (int i = 0; i < MAX_DATABASE_CONNECTION; i++)
    {
        CppMySQL3DB* pDatabase = new CppMySQL3DB;
        int nRet = pDatabase->open(host, user, passwd, db);
        m_pDatabase[i] = pDatabase;

        if (nRet != 0)
        {
            printf("open data base fail!\n");
        } else
        {
            printf("open database succ!\n");
        }
    }

    if (!m_bInit)
    {
        m_bInit = true;
        strcpy(m_szHost, host);
        strcpy(m_szUser, user);
        strcpy(m_szPass, passwd);
        strcpy(m_szDB, db);
        m_nPort = nPort;


#ifdef WIN32
        unsigned threadID = 0;
        HANDLE hThread = (HANDLE) _beginthreadex(NULL, 0, &DatabaseInitFunc, NULL, NULL, &threadID);
        CloseHandle(hThread);
#else
        pthread_t localThreadId;
        int nThreadErr = pthread_create(&localThreadId, NULL,
                (THREADFUNC) DatabaseInitFunc, NULL);
        if (nThreadErr == 0)
        {
            pthread_detach(localThreadId); //	�ͷ��߳�˽�����?���صȴ�pthread_join();

        }

#endif
    }

}

int CMySQLConnectionPool::TestDatabase()
{
    CppMySQL3DB* pDatabase = NULL;
    CMySQLConnectionPool::GetDataAccess(pDatabase);
    if (!pDatabase) //none connected or all occupied
    {
        for (int i = 0; i < MAX_DATABASE_CONNECTION; i++)
        {
            pDatabase = m_pDatabase[i];
            if (!pDatabase->isConnect())
            {
                int nRet = pDatabase->open(m_szHost, m_szUser, m_szPass, m_szDB);
                if (nRet != 0)
                {
                    CMyLog::m_pLog->_XGSysLog("fail to open database .\n");
                } else
                {
                    CMyLog::m_pLog->_XGSysLog("open database success .\n");
                }
            }
        }
    }

    if (pDatabase->isConnect())
    {
        int nRet = pDatabase->ping();
        pDatabase->setIdle();
        if (nRet == 0)
        {
            return 0;
        }
        else
        {
            pDatabase->close();
            pDatabase->open(m_szHost, m_szUser, m_szPass, m_szDB);
        }
    } else
    {
        pDatabase->setIdle();
        pDatabase->open(m_szHost, m_szUser, m_szPass, m_szDB);
    }

    return 0;
}
