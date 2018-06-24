/* 
 * File:   CJZPlatformProxy.cpp
 * Author: root
 * 
 * Created on 2015年1月15日, 下午11:03
 */

#include "CJZPlatformProxy.h"
#include "Encrypter.h"
#include "UtilTools.h"

#include <strings.h>
#include <string>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/std/utility.hpp>
#include <boost/shared_array.hpp>
#include <syslog.h>

//using std::string;
using namespace std;
using namespace boost;

extern "C" CPlatformProxy* create() {
    return new CJZPlatformProxy;
}

extern "C" void destroy(CPlatformProxy* p) {
    delete p;
}

CJZPlatformProxy::CJZPlatformProxy() {
    m_pCrtlCmdCallback = NULL;
    m_pBasicDataAccess = new CBasicDataAccess;
}

CJZPlatformProxy::~CJZPlatformProxy() {
}

int CJZPlatformProxy::PackUploadData(char* szPlatName, char* pData, int nLen, char* szPackedData, int& nPacketDataLen,int nIsRegather) {
    if (strcmp(szPlatName, "CUSTOMS") == 0) 
    {
        structGatherInfo gatherInfo;
        m_pBasicDataAccess->RecordPassVehicleInfo(gatherInfo, pData,nIsRegather);

        // later transfer
        char cpYearMonDay[256] = {0};
        struct tm* tmNow;
        time_t tmTime = time(NULL);
        tmNow = localtime(&tmTime);
        sprintf(cpYearMonDay, "%04d-%02d-%02dT%02d:%02d:%02d", tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec);
        gatherInfo.DATE_TIME = cpYearMonDay;
        
        
        
        const char packetHead[4] = {(char)0xE2, (char)0x5C, (char)0x4B, (char)0x89};
        const char packetEnd[2]  = {(char)0xFF, (char)0xFF};
	using boost::property_tree::ptree;
	ptree pt;
	ptree pattr1;
	ptree pattr2;
	ptree pattr3;
	ptree pattr4;
        
        char szAreaID2[10 + 1] = {0};
        char szChnlNo2[10 + 1] = {0};  
        strcpy(szAreaID2, gatherInfo.AREA_ID.c_str());
        strcpy(szChnlNo2, gatherInfo.CHNL_NO.c_str());
        
        for (int i = 0; i < 10; ++i)
        {
            if (szAreaID2[i] == '0')
                continue;
            {
                gatherInfo.AREA_ID = szAreaID2 + i;
                break;
            }
        }
        
        for (int i = 0; i < 10; ++i)
        {
            if (szChnlNo2[i] == '0')
                continue;
            {
                gatherInfo.CHNL_NO = szChnlNo2 + i;
                break;
            }
        }        
        
        char szOP_REASON[256] = {0};
	string strOpReason = gatherInfo.VE_NAME;
	CUtilTools::Gb2312ToUtf8(szOP_REASON, 256, strOpReason.c_str(), strOpReason.size());    
        gatherInfo.VE_NAME = szOP_REASON;
        
        

	pattr1.add<std::string>("<xmlattr>.AREA_ID",    gatherInfo.AREA_ID);
	pattr1.add<std::string>("<xmlattr>.CHNL_NO",    gatherInfo.CHNL_NO);
	pattr1.add<std::string>("<xmlattr>.I_E_TYPE",   gatherInfo.IE_FLAG);
	pattr1.add<std::string>("<xmlattr>.SEQ_NO",     gatherInfo.SEQ_NO);
        pattr1.add<std::string>("<xmlattr>.BUSINESS_CODE",  gatherInfo.BUSINESS_CODE);

	pt.add_child("GATHER_INFO", pattr1);        
        pt.put("GATHER_INFO.CONTA_ID1",     gatherInfo.CONTA_ID_F);
        pt.put("GATHER_INFO.CONTA_ID2",     gatherInfo.CONTA_ID_B);
        pt.put("GATHER_INFO.TRUCK_NO",      gatherInfo.VE_NAME);
        pt.put("GATHER_INFO.WEIGHT_BRIDGE", gatherInfo.WEIGHT_BRIDGE);
        pt.put("GATHER_INFO.TRUCK_WEIGHT",  gatherInfo.GROSS_WT);
        pt.put("GATHER_INFO.IDENTIFY_FLAG", gatherInfo.CONTA_RECO);
        pt.put("GATHER_INFO.DR_IC_NO",      gatherInfo.DR_IC_NO);
        pt.put("GATHER_INFO.CAR_EC_NO",     gatherInfo.CAR_EC_NO);
        pt.put("GATHER_INFO.DATE_TIME",     gatherInfo.DATE_TIME);

	// 格式化输出，指定编码（默认utf-8）
	boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "UTF-8");
	//write_xml(filename, pt, std::locale(), settings);
        
        ostringstream oss;
        write_xml(oss, pt, settings);
        
        string strGatherXml = oss.str();
        
        int nXmlLen = strGatherXml.size(); // strSendXml.size();
        int PackDataLen = 34 + 4 + 3 + nXmlLen + 2 + 4 + 4 + 128;

        boost::shared_array<BYTE> spPacketData;
        spPacketData.reset(new BYTE[PackDataLen + 1]);
        bzero(spPacketData.get(), PackDataLen + 1);

        BYTE *p = spPacketData.get();
        memcpy(p, packetHead ,4);
        p += 4;

        memcpy(p, (BYTE*)&PackDataLen, sizeof(int));
        p += 4;

        char szDataType[4 + 1] = {0x00, 0x00, 0x00, 0x21};
        memcpy(p, szDataType, 4);
        p += 4;	
        
        
        int k = gatherInfo.AREA_ID.size();
        char szAreaID[10 + 1] = {0};
        for (int i = 10 - k, j = 0; i < 10 && j < k; ++i,++j)
        {
            szAreaID[i] = gatherInfo.AREA_ID[j];
        }
        memcpy(p, szAreaID, 10);
        p += 10;
        
        k = gatherInfo.CHNL_NO.size();
        char szChnlNo[10 + 1] = {0};
        for (int i = 10 - k, j = 0; i < 10 && j < k; ++i,++j)
        {
            szChnlNo[i] = gatherInfo.CHNL_NO[j];
        }
        memcpy(p, szChnlNo, 10);
        p += 10;
        

        memcpy(p, gatherInfo.IE_FLAG.c_str(), 1);
        p += 1;							

        memset(p, 0xFF, 4);
        p += 4;	

        memcpy(p, &nXmlLen, 4);
        p+=4;

        memcpy(p, strGatherXml.c_str(), nXmlLen);
        p += nXmlLen;
        
        //
        //10	标识符	0xFF 0xFF 0xFF 0xFF	取固定值，为方便识别后文数字签名  + 4
        //11	数字签名长度		数字签名的长度（4 bytes）               + 4
        //12	数字签名	长度等于RSA密钥长度	数字签名                      + 128
        //13	包尾	0xFF 0xFF	取固定值

        int nSignedLen = 0;
        char szSignedData[128 + 1] = {0};
        Encrypter crypt;
        std::string strMessageHash = crypt.GetMessageHash(strGatherXml);
        std::string strPrivateFile = "/root/zhouhm/myopenssl/test/aprikey.pem";
        crypt.EncodeRSAKeyFile(strPrivateFile, strMessageHash, szSignedData, nSignedLen);
        
        memset(p, 0xFF, 4);
        p += 4;	        
        memcpy(p, &nSignedLen, 4);
        p += 4;
        memcpy(p, szSignedData, nSignedLen);
        p += nSignedLen;
        //
        
        memcpy(p, packetEnd ,2);
        p += 2;
        // end
        
        memcpy(szPackedData, (char*)spPacketData.get(), PackDataLen);
        nPacketDataLen = PackDataLen;
        
        syslog(LOG_DEBUG, "genetate gather messsage:%s\n", strGatherXml.c_str());
        syslog(LOG_DEBUG, "private key file:%s\n", strPrivateFile.c_str());
        syslog(LOG_DEBUG, "generate digust messsage:%s\n", strMessageHash.c_str());
        syslog(LOG_DEBUG, "generate digital signature messsage\n");
    }

    return 0;
}

int CJZPlatformProxy::UnpackCtrlData(char* pData, int nLen) {
    char szPlatName[32] = {0};

    if (VerifyBasicPlatformPacket(pData, nLen) == 0) {
        strcpy(szPlatName, "CUSTOMS");
    }


    T_PassResultInfo passResult;
    memset(&passResult, 0, sizeof (T_PassResultInfo));

    char m_szResultCtrlXML[4096] = {0};

    if (strcmp(szPlatName, "CUSTOMS") == 0) {
        m_pBasicDataAccess->RecordPassResult(pData, &passResult);


//        std::string strControlData = "";
//        strControlData = \
//                "<LED>\r\n"
//                "<LED_CODE>00</LED_CODE>\r\n"
//                "<LED_MSG>直接放行</LED_MSG>\r\n"
//                "</LED>\r\n"
//                "<BROADCAST>\r\n"
//                "<BROADCAST_CODE>00</BROADCAST_CODE>\r\n"
//                "<BROADCAST_MSG>直接放行</BROADCAST_MSG>\r\n"
//                "</BROADCAST>\r\n"
//                "<PRINT>\r\n"
//                "<VENAME>2</VENAME>\r\n"
//                "<CONTA_ID_F>HLXU4074106</CONTA_ID_F>\r\n"
//                "<CONTA_ID_B>FCIU9176921</CONTA_ID_B>\r\n"
//                "<CONTA_MODEL_F>20</CONTA_MODEL_F>\r\n"
//                "<CONTA_MODEL_B>20</CONTA_MODEL_B>\r\n"
//                "<Oper_Name></Oper_Name>\r\n"
//                "</PRINT>\r\n";
//
//
//        sprintf(m_szResultCtrlXML, "%s", "<CONTROL_INFO>");
//        sprintf(m_szResultCtrlXML, "%s <EVENT type=\"CAR\" hint=\"临时车辆放行\" eventid=\"%s\"><![CDATA[%s]]></EVENT>", m_szResultCtrlXML, "EC_FREE_PLATFORM", strControlData.c_str());
//        sprintf(m_szResultCtrlXML, "%s </CONTROL_INFO>", m_szResultCtrlXML);
//        
//        
//        m_pBasicDataAccess->RecordPassResult(pDatabase,passResult.szSeqNo,0,"临时车辆放行","EC_FREE_PLATFORM",m_szResultCtrlXML);
        
        
                int nPASS_FLAG                  = 0;
		std::string strControlData      = "";
                std::string strLED_CODE         = "";
                std::string strLED_MSG          = "";
                std::string strBROADCAST_CODE   = "";
                std::string strBROADCAST_MSG    = "";
                std::string strEventID          = "";
                if (passResult.szCheckResult[1] == '0')
                {
                    nPASS_FLAG          = 0;
                    strLED_CODE         = "00";
                    strLED_MSG          = passResult.szOPHint;
                    strBROADCAST_CODE   = "00";
                    strBROADCAST_MSG    = passResult.szOPHint;
                    strEventID          = "EC_FREE_PLATFORM";
                    
//                    if (strLED_MSG == "不查验，请直行")
//                    {
//                        strLED_CODE         = "1";
//                        strBROADCAST_CODE   = "1";
//                    }
                    //if (strLED_MSG == "放行抬杆")
                    strLED_MSG = strBROADCAST_MSG = "放行抬杆";
                    {
                        strLED_CODE         = "3";
                        strBROADCAST_CODE   = "3";
                    }
                }
                else
                {
                    nPASS_FLAG          = 1;
                    strLED_CODE         = "11";
                    strLED_MSG          = passResult.szOPHint;
                    strBROADCAST_CODE   = "11";
                    strBROADCAST_MSG    = passResult.szOPHint;
                    strEventID          = "EC_FORBID_PLATFORM";
                    
//                    if (strLED_MSG == "请到查验区进行查验")
//                    {
//                        strLED_CODE         = "2";
//                        strBROADCAST_CODE   = "2";
//                    }
                    //else if (strLED_MSG == "不放行")
                    strLED_MSG = strBROADCAST_MSG = passResult.szOPHint; // "不放行";
                    {
                        strLED_CODE         = "4";
                        strBROADCAST_CODE   = "4";
                    }
//                    else if (strLED_MSG == "重量异常不抬杆")
//                    {
//                        strLED_CODE         = "5";
//                        strBROADCAST_CODE   = "5";
//                    }
                }
                
                
                strControlData = \
"<LED>\r\n"
"<LED_CODE>";
                strControlData += strLED_CODE;  
                strControlData += "</LED_CODE>\r\n"
"<LED_MSG>";
            strControlData += strLED_MSG;    
            strControlData += "</LED_MSG>\r\n"
"</LED>\r\n"
"<BROADCAST>\r\n"
"<BROADCAST_CODE>";
            strControlData += strBROADCAST_CODE;
            strControlData += "</BROADCAST_CODE>\r\n"
"<BROADCAST_MSG>";
            strControlData += strBROADCAST_MSG;
            strControlData += "</BROADCAST_MSG>\r\n"
"</BROADCAST>\r\n"
"<PRINT>\r\n"
"<VENAME></VENAME>\r\n"
"<CONTA_ID_F></CONTA_ID_F>\r\n"
"<CONTA_ID_B></CONTA_ID_B>\r\n"
"<CONTA_MODEL_F></CONTA_MODEL_F>\r\n"
"<CONTA_MODEL_B></CONTA_MODEL_B>\r\n"
"<Oper_Name></Oper_Name>\r\n"
"</PRINT>\r\n";

		sprintf(m_szResultCtrlXML, "%s", "<CONTROL_INFO>");
		sprintf(m_szResultCtrlXML, "%s <EVENT type=\"CAR\" hint=\"%s\" eventid=\"%s\"><![CDATA[%s]]></EVENT>", m_szResultCtrlXML, strLED_MSG.c_str(), strEventID.c_str(), strControlData.c_str());
		sprintf(m_szResultCtrlXML, "%s </CONTROL_INFO>", m_szResultCtrlXML);


		m_pBasicDataAccess->RecordPassResult(passResult.szSeqNo, nPASS_FLAG,strLED_MSG.c_str(), strEventID.c_str(),m_szResultCtrlXML);    
                
    }

    if (m_pCrtlCmdCallback) {

        m_pCrtlCmdCallback(passResult.szAeraID, passResult.szChlNo, passResult.szIEType, passResult.szSeqNo, m_szResultCtrlXML, strlen(m_szResultCtrlXML) + 1);
    }

    return 0;
}

int CJZPlatformProxy::BuildCtrlData(char* szChannelNo, char* szPassSequence, char* pData, char* szMemo) {
    char m_szResultCtrlXML[4096] = {0};

    string strFullChannelID(szChannelNo);

    string strAreaID = strFullChannelID.substr(0, 10);
    string strChannelNo = strFullChannelID.substr(10, 10);
    string strIEType = strFullChannelID.substr(20, 1);



    if (strcmp(pData, "00") == 0) {
        m_pBasicDataAccess->RecordManPass(szPassSequence, szMemo);

        sprintf(m_szResultCtrlXML, "%s", "<CONTROL_INFO>");
        sprintf(m_szResultCtrlXML, "%s <EVENT type=\"CAR\" hint=\"临时车辆放行\" eventid=\"%s\"></EVENT>", m_szResultCtrlXML, "M000000001");
        sprintf(m_szResultCtrlXML, "%s </CONTROL_INFO>", m_szResultCtrlXML);
    }

    if (m_pCrtlCmdCallback) {
        m_pCrtlCmdCallback((char*) strAreaID.c_str(), (char*) strChannelNo.c_str(), (char*) strIEType.c_str(), szPassSequence, m_szResultCtrlXML, strlen(m_szResultCtrlXML) + 1);
    }

}

int CJZPlatformProxy::BuildExceptionFreeData(char* szChannelNo, char* szPassSequence, char* pData, int nDataLen, char* szMemo) 
{
    if (pData == NULL || nDataLen <= 0)
        return 0;
    
	char m_szResultCtrlXML[4096] = {0};

	string strFullChannelID(szChannelNo);

	string strAreaID = strFullChannelID.substr(0, 10);
	string strChannelNo = strFullChannelID.substr(10, 10);
	string strIEType = strFullChannelID.substr(20, 1);



	// if (strcmp(pData, "00") == 0) 
	{
		//m_pBasicDataAccess->RecordManPass(pDatabase, szPassSequence, szMemo);

strncpy(m_szResultCtrlXML, pData, nDataLen);
//		std::string strControlData = "";
//		strControlData = \
//			"<LED>\r\n"
//			"<LED_CODE>00</LED_CODE>\r\n"
//			"<LED_MSG>直接放行</LED_MSG>\r\n"
//			"</LED>\r\n"
//			"<BROADCAST>\r\n"
//			"<BROADCAST_CODE>00</BROADCAST_CODE>\r\n"
//			"<BROADCAST_MSG>直接放行</BROADCAST_MSG>\r\n"
//			"</BROADCAST>\r\n"
//			"<PRINT>\r\n"
//			"<VENAME>2</VENAME>\r\n"
//			"<CONTA_ID_F>HLXU4074106</CONTA_ID_F>\r\n"
//			"<CONTA_ID_B>FCIU9176921</CONTA_ID_B>\r\n"
//			"<CONTA_MODEL_F>20</CONTA_MODEL_F>\r\n"
//			"<CONTA_MODEL_B>20</CONTA_MODEL_B>\r\n"
//			"<Oper_Name></Oper_Name>\r\n"
//			"</PRINT>\r\n";
//
//		sprintf(m_szResultCtrlXML, "%s", "<CONTROL_INFO>");
//		//sprintf(m_szResultCtrlXML, "%s <EVENT type=\"CAR\" hint=\"临时车辆放行\" eventid=\"%s\"></EVENT>", m_szResultCtrlXML, "M000000001");
//		sprintf(m_szResultCtrlXML, "%s <EVENT type=\"CAR\" hint=\"临时车辆放行\" eventid=\"%s\"><![CDATA[%s]]></EVENT>", m_szResultCtrlXML, "M000000001", strControlData.c_str());
//		sprintf(m_szResultCtrlXML, "%s </CONTROL_INFO>", m_szResultCtrlXML);
		m_pBasicDataAccess->RecordPassResult(szPassSequence, 99, "货物放行", "EC_FREE_MAN",m_szResultCtrlXML);
	}

	if (m_pCrtlCmdCallback) {
		m_pCrtlCmdCallback((char*) strAreaID.c_str(), (char*) strChannelNo.c_str(), (char*) strIEType.c_str(), szPassSequence, m_szResultCtrlXML, strlen(m_szResultCtrlXML) + 1);
	}

}


int CJZPlatformProxy::VerifyBasicPlatformPacket(char *chRecvBuffer, int nRecLen) {
    /*缓冲区长度小于最小帧长度*/
    if (nRecLen < 40) {
        return -1;
    }

    unsigned char* pRecvBuffer = (unsigned char*) chRecvBuffer;

    if (pRecvBuffer[0] == 0XE2 && pRecvBuffer[1] == 0X5C && pRecvBuffer[2] == 0X4B && pRecvBuffer[3] == 0X89) {
        if (pRecvBuffer[nRecLen - 2] == 0XFF && pRecvBuffer[nRecLen - 1] == 0XFF) {
            return 0;
        }
    }

    return -1;
}


