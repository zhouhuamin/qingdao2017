// MyLog.cpp: implementation of the CMyLog class.
//
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
#include "stdafx.h"
#endif

#include "MyLog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CFreeLongLog* CMyLog::m_pLog = NULL;
CFreeLongSourceRegister* CMyLog::m_pResourceRegister = NULL;

ACE_Thread_Mutex CMyLog::log_lock_;

bool CMyLog::m_bInit = false;

int CMyLog::LogSysOperation(char* chMsg)
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, log_lock_, -1);
    m_pLog->_XGSysLog(chMsg);
    return 0;
}

void CMyLog::Init()
{
    if (!m_bInit)
    {
        m_bInit = true;

        if (!m_pLog)
        {

            /*
             *	获得当前绝对路径
             */
            char szFullPath[256]; // MAX_PATH

#ifdef WIN32
            GetModuleFileName(NULL, szFullPath, 256); //得到程序模块名称，全路径
#else
            int iCount = 0;
            iCount = readlink("/proc/self/exe", szFullPath, 256);
            szFullPath[iCount] = '\0';

#endif


            string strCurrentpath(szFullPath);
            int nLast = strCurrentpath.find_last_of("\\");

            strCurrentpath = strCurrentpath.substr(0, nLast + 1);
            m_pLog = new CFreeLongLog((char*) strCurrentpath.c_str(), "ControlCenter", 4);
        }

        if (!m_pResourceRegister)
        {
            m_pResourceRegister = new CFreeLongSourceRegister;
        }
    }

}

void CMyLog::Release()
{
    if (m_bInit)
    {
        if (m_pResourceRegister)
        {
            std::map<long, ResourceUseInfo*> resourceMap;
            m_pResourceRegister->GetResourceRegisterInfo(resourceMap);
            if (resourceMap.size() > 0)
            {
                std::map<long, ResourceUseInfo*>::iterator iter;
                int n = resourceMap.size();
                for (iter = resourceMap.begin(); iter != resourceMap.end(); iter++)
                {
                    ResourceUseInfo* pResource = iter->second;
                    m_pLog->_XGSysLog("********Memory Leak! %p,malloc detail is %s \n", pResource->m_pResource, pResource->szResourceInfo);
                }
            }

            delete m_pResourceRegister;
            m_pResourceRegister = NULL;
        }


        if (m_pLog)
        {
            delete m_pLog;
            m_pLog = NULL;
        }
    }

}

int CMyLog::RegisterResourceInfo(void *pResource, char *szSourceInfo)
{
    return m_pResourceRegister->RegisterResourceInfo(pResource, szSourceInfo);
}

int CMyLog::UnResourceInfo(void *pResource)
{
    m_pResourceRegister->UnResourceInfo(pResource);
    return 0;
}

int CMyLog::PrintRespurceInfo()
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, log_lock_, -1);
    if (m_pResourceRegister->m_pResourceInfoMap.size() > 0)
    {
        m_pLog->_XGSysLog("Resource use info,malloc size is %d\n", m_pResourceRegister->m_pResourceInfoMap.size());
    }

    return 0;

}
