// FreeLongSourceRegister.cpp: implementation of the CFreeLongSourceRegister class.
//
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
#include "stdafx.h"
#else
#endif
#include "FreeLongSourceRegister.h"
#include <string.h>
#include <stdio.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFreeLongSourceRegister::CFreeLongSourceRegister()
{

}

CFreeLongSourceRegister::~CFreeLongSourceRegister()
{
    std::map<long, ResourceUseInfo*>::iterator iter;

    for (iter = m_pResourceInfoMap.begin(); iter != m_pResourceInfoMap.end(); iter++)
    {
        ResourceUseInfo* pResourceInfo = iter->second;
        delete pResourceInfo;
        pResourceInfo = NULL;
    }

    m_pResourceInfoMap.clear();
}

int CFreeLongSourceRegister::RegisterResourceInfo(void *pResource, char *szSourceInfo)
{
    bool bHasRegistered = false;
    m_Lock.Lock();
    if (m_pResourceInfoMap[long(pResource)] == NULL)
    {
        ResourceUseInfo* pUseResource = new ResourceUseInfo;
        pUseResource->m_pResource = pResource;
        try
        {
            strcpy(pUseResource->szResourceInfo, szSourceInfo);
        }
        catch (...)
        {
            strcpy(pUseResource->szResourceInfo, "");
        }

        m_pResourceInfoMap[long(pResource)] = pUseResource;
    }
    else
    {
        bHasRegistered = true;
    }

    m_Lock.Unlock();

    if (bHasRegistered)
    {
        return -1;
    }
    else
    {
        return 0;
    }

}

void CFreeLongSourceRegister::UnResourceInfo(void *pResource)
{

    m_Lock.Lock();

    if (m_pResourceInfoMap[long(pResource)] != NULL)
    {
        ResourceUseInfo* pUseResource = m_pResourceInfoMap[long(pResource)];
        delete pUseResource;
        pUseResource = NULL;
        m_pResourceInfoMap.erase(long(pResource));
    }

    m_Lock.Unlock();

}

void CFreeLongSourceRegister::GetResourceRegisterInfo(std::map<long, ResourceUseInfo*>& resourceUnseMap)
{
    m_Lock.Lock();
    resourceUnseMap = m_pResourceInfoMap;
    m_Lock.Unlock();
}

