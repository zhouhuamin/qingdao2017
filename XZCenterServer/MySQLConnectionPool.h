// MySQLConnectionPool.h: interface for the CMySQLConnectionPool class.
//
//////////////////////////////////////////////////////////////////////

#if !defined _MYSQL_CONNECTION_POOL_H
#define _MYSQL_CONNECTION_POOL_H


#include "SysMutex.h"
#include "cppmysql.h"

#define MAX_DATABASE_CONNECTION 5

class CMySQLConnectionPool  
{
public:
	static int TestDatabase();
	static int GetDataAccess(CppMySQL3DB*& pDatabase);
	static void Init(const char* host, const char* user, const char* passwd, const char* db,int nPort);
	CMySQLConnectionPool();
	virtual ~CMySQLConnectionPool();

	static char             m_szHost[64];
	static char             m_szUser[64];
	static char             m_szPass[64];
	static char             m_szDB[64];
	static int              m_nPort;
private:
	static CppMySQL3DB*     m_pDatabase[MAX_DATABASE_CONNECTION];
	static int              m_nDataIndex;
	static int              m_nTestDataIndex;
	static CSysMutex        access_lock_;

	static bool             m_bInit;

};



class CSQLAutoLock
{
private:
    CppMySQL3DB *pDatabase_;
public:

    CSQLAutoLock(CppMySQL3DB* pDatabase)
    {
        pDatabase_ = pDatabase;
    };

    ~CSQLAutoLock()
    {
        if (pDatabase_ != NULL)
            pDatabase_->setIdle();
    };
};



#endif 
