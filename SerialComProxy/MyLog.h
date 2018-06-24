// MyLog.h: interface for the CMyLog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_MYLOG_H_)
#define _MYLOG_H_


#include "ace/Task.h"
#include <ace/Semaphore.h>
#include "Log/FreeLongLog.h"
#include "Log/FreeLongSourceRegister.h"
#include <string>
#include <map>
#include <list>
using namespace std;

class CMyLog
{
public:
	static int PrintRespurceInfo();
	static int UnResourceInfo(void *pResource);
	static int RegisterResourceInfo(void *pResource, char *szSourceInfo);
	static void Release();
	static void Init();
	static int LogSysOperation(char* chMsg);
	static ACE_Thread_Mutex log_lock_;


	static CFreeLongSourceRegister*   m_pResourceRegister;
	static CFreeLongLog* m_pLog;
private:
	static bool   m_bInit;
	
};

#endif










































