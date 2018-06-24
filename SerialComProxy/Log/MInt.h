// MInt.h: interface for the CMInt class.
//
//////////////////////////////////////////////////////////////////////

#if !defined _MINT_H_
#define _MINT_H_
#include "Mtvar.h"
class CMInt  
{
public:
	CMInt(int nValue=0)
	{
		XMI(m_nValue,nValue);
	}

	virtual ~CMInt()
	{
		XME(m_nValue);	
	}

public:
	int Get()
	{
		return XMG(m_nValue);
	}


	int Set(int nValue)
	{
		return XMS(m_nValue,nValue);
	}

	int Add(int nValue=1)
	{
		return XMA(m_nValue,nValue);
	}


	int Dec(int nValue=1)
	{
		return XMD(m_nValue,nValue);
	}

private:
	MINT m_nValue;
};


class CMBOOL  
{
public:
	CMBOOL(bool nValue=false)
	{
		XMI(m_nValue,nValue);
	}
	
	virtual ~CMBOOL()
	{
		XME(m_nValue);	
	}
	
public:
	int Get()
	{
		return XMG(m_nValue);
	}
	
	
	int Set(bool nValue)
	{
		return XMS(m_nValue,nValue);
	}
	
private:
	MBOOL m_nValue;
};


#endif 
