// SysMutex.cpp: implementation of the CSysMutex class.
//
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
#include "stdafx.h"
#endif

#include "SysMutex.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSysMutex::CSysMutex()
{
    MUTEXINIT(&m_crtRealDataList);
}

CSysMutex::~CSysMutex()
{
    MUTEXDESTROY(&m_crtRealDataList);
}

void CSysMutex::Lock()
{
    MUTEXLOCK(&m_crtRealDataList);
}

void CSysMutex::UnLock()
{
    MUTEXUNLOCK(&m_crtRealDataList);
}
