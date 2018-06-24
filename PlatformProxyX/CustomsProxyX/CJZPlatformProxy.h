/* 
 * File:   CJZPlatformProxy.h
 * Author: root
 *
 * Created on 2015年1月15日, 下午11:03
 */

#ifndef CJZPLATFORMPROXY_H
#define	CJZPLATFORMPROXY_H

#include "../include/PlatProxy.h"
#include "../include/cppmysql.h"
#include "tinyXML/tinyxml.h"
#include "cbasicdataaccess.h"

typedef unsigned char BYTE;

class CJZPlatformProxy : public CPlatformProxy
{
public:
    CJZPlatformProxy();
    virtual ~CJZPlatformProxy();

    int SetCtrlCmdCallback(_CONTROL_CMD_CALLBACK pCtrlCmdcallback) {
        m_pCrtlCmdCallback = pCtrlCmdcallback;
    }

    int RecordUploadData(CppMySQL3DB* pDatabase, char* pData, int nLen,int nIsRegather=0);
    int PackUploadData(CppMySQL3DB* pDatabase, char* szPlatName, char* pData, int nLen, char* szPackedData, int& nPacketDataLen,int nIsRegather=0);
    int UnpackCtrlData(CppMySQL3DB* pDatabase, char* pData, int nLen);
    int BuildCtrlData(CppMySQL3DB* pDatabase,char* szChannelNo,char* szPassSequence,char* pData,char* szMemo);
    int BuildExceptionFreeData(CppMySQL3DB* pDatabase,char* szChannelNo,char* szPassSequence,char* pData, int nDataLen, char* szMemo);
    
    //平台代理调用，不写数据库，只负责数据的打包和组包
    int PackUploadDataProxy(char* szPlatName, char* pData, int nLen, char* szPackedData, int& nPacketDataLen);
    
private:
    int VerifyBasicPlatformPacket(char *chRecvBuffer, int nRecLen);

private:
    CBasicDataAccess* m_pBasicDataAccess;
    _CONTROL_CMD_CALLBACK m_pCrtlCmdCallback;

};

#endif	/* CJZPLATFORMPROXY_H */

