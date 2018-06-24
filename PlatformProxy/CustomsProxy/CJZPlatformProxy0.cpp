/* 
 * File:   CJZPlatformProxy.cpp
 * Author: root
 * 
 * Created on 2015年1月15日, 下午11:03
 */

#include "CJZPlatformProxy.h"

extern "C" CPlatformProxy* create()
{
    return new CJZPlatformProxy;
}

extern "C" void destroy(CPlatformProxy* p)
{
    delete p;
}

CJZPlatformProxy::CJZPlatformProxy()
{
    m_pCrtlCmdCallback = NULL;
    m_pBasicDataAccess = new CBasicDataAccess;
}

CJZPlatformProxy::~CJZPlatformProxy()
{
}

int CJZPlatformProxy::PackUploadData(CppMySQL3DB* pDatabase, char* szPlatName, char* pData, int nLen, char* szPackedData, int& nPacketDataLen)
{
    if (strcmp(szPlatName, "CUSTOMS") == 0)
    {
        m_pBasicDataAccess->RecordPassVehicleInfo(pDatabase, pData);
        memcpy(szPackedData, pData, nLen);
        nPacketDataLen = nLen;
    }

    return 0;
}

int CJZPlatformProxy::UnpackCtrlData(CppMySQL3DB* pDatabase, char* pData, int nLen)
{
    char szPlatName[32] = {0};

    if (VerifyBasicPlatformPacket(pData, nLen) == 0)
    {
        strcpy(szPlatName, "CUSTOMS");
    }


    T_PassResultInfo passResult;
    memset(&passResult, 0, sizeof (T_PassResultInfo));

    char m_szResultCtrlXML[4096]={0};

    if (strcmp(szPlatName, "CUSTOMS") == 0)
    {
        m_pBasicDataAccess->RecordPassResult(pDatabase, pData, &passResult);

        sprintf(m_szResultCtrlXML, "%s", "<CONTROL_INFO>");
        sprintf(m_szResultCtrlXML, "%s <EVENT type=\"CAR\" hint=\"临时车辆放行\" eventid=\"%s\"><![CDATA[%s]]></EVENT>", m_szResultCtrlXML, "S000000001", "");
        sprintf(m_szResultCtrlXML, "%s </CONTROL_INFO>", m_szResultCtrlXML);
    }

    if (m_pCrtlCmdCallback)
    {
      
        m_pCrtlCmdCallback(passResult.szAeraID,passResult.szChlNo,passResult.szIEType,passResult.szSeqNo, m_szResultCtrlXML, strlen(m_szResultCtrlXML) + 1);
    }

    return 0;
}

int CJZPlatformProxy::BuildCtrlData(CppMySQL3DB* pDatabase, char* szChannelNo,char* szPassSequence,char* pData,char* szMemo)
{
    char m_szResultCtrlXML[4096]={0};
    
    string strFullChannelID(szChannelNo);
    
    string strAreaID=strFullChannelID.substr(0,10);
    string strChannelNo=strFullChannelID.substr(10,10);
    string strIEType=strFullChannelID.substr(20,1);
    
    
    
    if (strcmp(pData, "00") == 0)
    {
        m_pBasicDataAccess->RecordManPass(pDatabase,szPassSequence,szMemo );

        sprintf(m_szResultCtrlXML, "%s", "<CONTROL_INFO>");
        sprintf(m_szResultCtrlXML, "%s <EVENT type=\"CAR\" hint=\"临时车辆放行\" eventid=\"%s\"></EVENT>", m_szResultCtrlXML, "M000000001");
        sprintf(m_szResultCtrlXML, "%s </CONTROL_INFO>", m_szResultCtrlXML);
    }
    
    if (m_pCrtlCmdCallback)
    {
        m_pCrtlCmdCallback((char*)strAreaID.c_str(),(char*)strChannelNo.c_str(),(char*)strIEType.c_str(),szPassSequence, m_szResultCtrlXML, strlen(m_szResultCtrlXML) + 1);
    }
    
}



int CJZPlatformProxy::VerifyBasicPlatformPacket(char *chRecvBuffer, int nRecLen)
{
    /*缓冲区长度小于最小帧长度*/
    if (nRecLen < 40)
    {
        return -1;
    }

    unsigned char* pRecvBuffer = (unsigned char*) chRecvBuffer;

    if (pRecvBuffer[0] == 0XE2 && pRecvBuffer[1] == 0X5C && pRecvBuffer[2] == 0X4B && pRecvBuffer[3] == 0X89)
    {
        if (pRecvBuffer[nRecLen - 2] == 0XFF && pRecvBuffer[nRecLen - 1] == 0XFF)
        {
            return 0;
        }
    }

    return -1;
}


