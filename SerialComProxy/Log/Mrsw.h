#if !defined _MULTI_READ_SINGLE_WRITE_H_
#define _MULTI_READ_SINGLE_WRITE_H_
#include "MutexLock.h"
typedef struct _FREE_LONG_MULTI_READ_SINGLE_WRITE_LOCK_
{
	int         m_nReadCount;             //读计数器
	bool        m_bWriteFlag;             //写标志
	MUTEX       m_Lock;                   //读写锁
}FreeLongMultiReadSingleWriteLock;

const ULONG FreeLongMultiReadSingleWriteLockSize=sizeof(FreeLongMultiReadSingleWriteLock);


inline void FreeLongMinSleep()
{
#ifdef WIN32
	Sleep(10);
#else
	struct timespec slptm;
	timespec.tv_sec=0;
	timespec.tv_nsec=1000;
	if(nanosleep(&slptm,NULL)==-1)
	{
		usleep(1);
	}
#endif
}

void inline MRSWLock_Create(FreeLongMultiReadSingleWriteLock* pLock)
{
	MUTEXINIT(&(pLock->m_Lock));              //初始化内部锁
	pLock->m_nReadCount=0;
	pLock->m_bWriteFlag=false;
}


void inline MRSWLock_Destroy(FreeLongMultiReadSingleWriteLock* pLock)
{
	MUTEXLOCK(&(pLock->m_Lock));
	MUTEXUNLOCK(&(pLock->m_Lock));
	MUTEXDESTROY(&(pLock->m_Lock));
}

bool inline MRSWLock_GetWrites(FreeLongMultiReadSingleWriteLock* pLock)
{
	bool bRet=false;
	MUTEXLOCK(&(pLock->m_Lock));
	{
		bRet=pLock->m_bWriteFlag;
	}
	MUTEXUNLOCK(&(pLock->m_Lock));
	return bRet;
}


int inline MRSWLock_GetRead(FreeLongMultiReadSingleWriteLock* pLock)
{
	int  nRet=0;
	MUTEXLOCK(&(pLock->m_Lock));
	{
		nRet=pLock->m_nReadCount;
	}
	MUTEXUNLOCK(&(pLock->m_Lock));
	return nRet;
}


void inline MRSWLock_EnableWrite(FreeLongMultiReadSingleWriteLock* pLock)
{
	while(1)                   //死循环等待
	{
		MUTEXLOCK(&(pLock->m_Lock));
		{
			if(!pLock->m_bWriteFlag)
			{
				//立即设置写标志为真
				pLock->m_bWriteFlag=true;

				//解锁退出
				MUTEXUNLOCK(&(pLock->m_Lock));

				//采用goto精确定位
				goto MRSWLock_EnableWrite_Wait_Read_Clean;
			}
		}

		//先释放内部锁，再等待
		MUTEXUNLOCK(&(pLock->m_Lock));
		//Sleep线程
		FreeLongMinSleep();
	}

	//程序运行到这里，表示已经获取写的权利
MRSWLock_EnableWrite_Wait_Read_Clean:
	//等待其他读操作完成
	while (MRSWLock_GetRead(pLock))
	{
		//等待所有的读操作完成
		FreeLongMinSleep();
	}
}

void inline MRSWLock_DisableWrite(FreeLongMultiReadSingleWriteLock* pLock)
{
	MUTEXLOCK(&(pLock->m_Lock));
	{
		pLock->m_bWriteFlag=false;
	}
	MUTEXUNLOCK(&(pLock->m_Lock));			
}


int inline MRSWLock_AddRead(FreeLongMultiReadSingleWriteLock* pLock)
{
	while(1)                   //死循环等待,即使有其他线程操作，也会很快完成
	{
		MUTEXLOCK(&(pLock->m_Lock));
		{
			if(!pLock->m_bWriteFlag)
			{
				//先累加，再释放
				pLock->m_nReadCount++;
				//解锁退出
				MUTEXUNLOCK(&(pLock->m_Lock));
			
				return MRSWLock_GetRead(pLock);
			}
		}
		
		//先释放内部锁，再等待
		MUTEXUNLOCK(&(pLock->m_Lock));
		//Sleep线程
		FreeLongMinSleep();
	}
}


int inline MRSWLock_DecRead(FreeLongMultiReadSingleWriteLock* pLock)
{
	int nRet=0;
	{
		MUTEXLOCK(&(pLock->m_Lock));
		{
			if(pLock->m_nReadCount>0)
			{
				//先累加，再释放
				pLock->m_nReadCount--;
				nRet=pLock->m_nReadCount;
			}
		}
		
		//先释放内部锁，再等待
		MUTEXUNLOCK(&(pLock->m_Lock));
		return nRet;
	}
}


void inline MRSWLock_Read2Write(FreeLongMultiReadSingleWriteLock* pLock)
{
	while(1)                   //死循环等待
	{
		MUTEXLOCK(&(pLock->m_Lock));
		{
			if(!pLock->m_bWriteFlag)
			{
				//立即设置写标志为真
				pLock->m_bWriteFlag=true;

				if(pLock->m_nReadCount>0)
				{
					//先累加，再释放
					pLock->m_nReadCount--;
				}
				
				//解锁退出
				MUTEXUNLOCK(&(pLock->m_Lock));
				//采用goto精确定位
				goto MRSWLock_Read2Write_Wait_Read_Clean;
			}
		}
		
		//先释放内部锁，再等待
		MUTEXUNLOCK(&(pLock->m_Lock));
		//Sleep线程
		FreeLongMinSleep();
	}
	
	//程序运行到这里，表示已经获取写的权利
MRSWLock_Read2Write_Wait_Read_Clean:
	//等待其他读操作完成
	while (MRSWLock_GetRead(pLock))
	{
		//等待所有的读操作完成
		FreeLongMinSleep();
	}
}

#endif