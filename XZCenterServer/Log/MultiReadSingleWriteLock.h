// MultiReadSingleWriteLock.h: interface for the CMultiReadSingleWriteLock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined _MULTI_READ_SINGLE_WRITE_LOCK_H_
#define _MULTI_READ_SINGLE_WRITE_LOCK_H_

#include "Mrsw.h"

class CMultiReadSingleWriteLock  
{
public:
	CMultiReadSingleWriteLock()
	{
		MRSWLock_Create(&m_Lock);
	}

	~CMultiReadSingleWriteLock()
	{
		MRSWLock_Destroy(&m_Lock);
	}

public:
	void EnableWrite()
	{
		MRSWLock_EnableWrite(&m_Lock);
	}

	void DisableWrite()
	{
		MRSWLock_DisableWrite(&m_Lock);
	}


	void Read2Write()
	{
		MRSWLock_Read2Write(&m_Lock);
	}

	void DecRead()
	{
		MRSWLock_DecRead(&m_Lock);
	}


	void AddRead()
	{
		MRSWLock_AddRead(&m_Lock);
	}


	void GetWrite()
	{
		MRSWLock_GetWrites(&m_Lock);
	}

	int GetRead()
	{
		MRSWLock_GetRead(&m_Lock);
	}


private:
	FreeLongMultiReadSingleWriteLock m_Lock;

};

#endif 
