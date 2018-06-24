// MRSWInt.h: interface for the CMRSWInt class.
//
//////////////////////////////////////////////////////////////////////

#if !defined _MULTI_READ_SINGLE_WRITE_INT_H_
#define _MULTI_READ_SINGLE_WRITE_INT_H_

#include "MultiReadSingleWriteLock.h"

class CMRSWInt  
{
public:
	CMRSWInt()
	{
		m_Lock.EnableWrite();
		m_nValue=0;
		m_Lock.DisableWrite();
	}


	~CMRSWInt(){}

public:
	int Get()
	{
		int nRet=0;
		m_Lock.AddRead();
		nRet=m_nValue;
		m_Lock.DecRead();
		return nRet;
	}


	int Set(int nValue)
	{
		int nRet=0;
		m_Lock.EnableWrite();
		{
			m_nValue=nValue;
			nRet=m_nValue;
		}
		
		m_Lock.DisableWrite();
		return nRet;
	}


	int Add(int nValue=1)
	{
		int nRet=0;
		m_Lock.EnableWrite();
		{
			m_nValue+=nValue;
			nRet=m_nValue;
		}
		
		m_Lock.DisableWrite();
		return nRet;
	}



	int Dec(int nValue=1)
	{
		int nRet=0;
		m_Lock.EnableWrite();
		{
			m_nValue-=nValue;
			nRet=m_nValue;
		}
		
		m_Lock.DisableWrite();
		return nRet;
	}

private:
	int                        m_nValue;
	CMultiReadSingleWriteLock  m_Lock;
};


class CMRSWBool  
{
public:
	CMRSWBool(){}
	~CMRSWBool(){}
public:
	bool Get()
	{
		return (bool)m_nValue.Get();
	}
	
	
	int Set(bool bFlag)
	{
		return m_nValue.Set((int)bFlag);
	}	
private:
	CMRSWInt     m_nValue;
};

#endif 
