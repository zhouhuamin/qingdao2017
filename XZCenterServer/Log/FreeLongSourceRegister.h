// FreeLongSourceRegister.h: interface for the CFreeLongSourceRegister class.
//
//////////////////////////////////////////////////////////////////////

#if !defined  _FREELONG_SOURCE_REGISTER_H_
#define  _FREELONG_SOURCE_REGISTER_H_

#include "MutexLock.h"

#pragma warning(disable: 4786)
#include <map>
using namespace std;

struct ResourceUseInfo
{
	void*      m_pResource;
	char       szResourceInfo[256];
};

class CFreeLongSourceRegister  
{
public:
	void UnResourceInfo(void* pResource);
	int RegisterResourceInfo(void* pResource,char* szSourceInfo);
	CFreeLongSourceRegister();
	void GetResourceRegisterInfo(std::map<long,ResourceUseInfo*>& resourceUnseMap);


	virtual ~CFreeLongSourceRegister();
	std::map<long,ResourceUseInfo*>   m_pResourceInfoMap;
private:
	CMutexLock    m_Lock;
	
};

#endif 
