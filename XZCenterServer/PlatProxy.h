#if !defined PLAT_PROXY_H
#define PLAT_PROXY_H

#include "cppmysql.h"

#include <map>
#include <string>
using namespace std;

typedef void (*_CONTROL_CMD_CALLBACK)(char* szAreaID,char* szChannelNo,char* sIEType,char* szSequenceNo,char* szCtrlXML,int nLen);

class CPlatformProxy
{
public:

    CPlatformProxy()
    {
    }
    ;

    virtual ~CPlatformProxy()
    {
    };

public:
    virtual int SetCtrlCmdCallback(_CONTROL_CMD_CALLBACK pCtrlCmdcallback )
    {
        return 0;
    }
    
    virtual int PackUploadData(CppMySQL3DB* pDatabase,char* szPlatName,char* pData, int nLen,char* szPackedData,int& nPacketDataLen,int nIsRegather=0)
    {
        return 0;
    }
    

    virtual int UnpackCtrlData(CppMySQL3DB* pDatabase,char* pData, int nLen)
    {
        return 0;
    }
    
    virtual int BuildCtrlData(CppMySQL3DB* pDatabase,char* szChannelNo,char* szPassSequence,char* pData,char* szMemo)
    {
        return 0;
    }
    
	virtual int BuildExceptionFreeData(CppMySQL3DB* pDatabase,char* szChannelNo,char* szPassSequence,char* pData,int nDataLen,char* szMemo)
	{
		return 0;
	}
};

// the types of the class factories
typedef CPlatformProxy* create_t();
typedef void destroy_t(CPlatformProxy*);


#endif

