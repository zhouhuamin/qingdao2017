/* 
 * File:   CJZPlatformProxy.cpp
 * Author: root
 * 
 * Created on 2015年1月15日, 下午11:03
 */

#include "CJZPlatformProxy.h"
#include <strings.h>
#include <string>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/std/utility.hpp>
#include <boost/shared_array.hpp>

//using std::string;
using namespace std;
using namespace boost;

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

int CJZPlatformProxy::RecordUploadData(CppMySQL3DB* pDatabase, char* pData, int nLen, int nIsRegather)
{

    structGatherInfo gatherInfo;
    m_pBasicDataAccess->RecordPassVehicleInfo(gatherInfo, pDatabase, pData, nIsRegather);



    return 0;
}

int CJZPlatformProxy::PackUploadData(CppMySQL3DB* pDatabase, char* szPlatName, char* pData, int nLen, char* szPackedData, int& nPacketDataLen, int nIsRegather)
{
    if (strcmp(szPlatName, "CUSTOMS") == 0)
    {
        structGatherInfo gatherInfo;
        m_pBasicDataAccess->RecordPassVehicleInfo(gatherInfo, pDatabase, pData, nIsRegather);

        // later transfer
        const char packetHead[4] = {(char) 0xE2, (char) 0x5C, (char) 0x4B, (char) 0x89};
        const char packetEnd[2] = {(char) 0xFF, (char) 0xFF};
        using boost::property_tree::ptree;
        ptree pt;
        ptree pattr1;
        ptree pattr2;
        ptree pattr3;
        ptree pattr4;

        pattr1.add<std::string>("<xmlattr>.AREA_ID", gatherInfo.AREA_ID);
        pattr1.add<std::string>("<xmlattr>.CHNL_NO", gatherInfo.CHNL_NO);
        pattr1.add<std::string>("<xmlattr>.I_E_TYPE", gatherInfo.IE_FLAG);
        pattr1.add<std::string>("<xmlattr>.SEQ_NO", gatherInfo.SEQ_NO);
        pattr1.add<std::string>("<xmlattr>.CHNL_TYPE", gatherInfo.CHNL_TYPE);

        pt.add_child("GATHER_INFO", pattr1);
        pt.put("GATHER_INFO.IC.DR_IC_NO", gatherInfo.DR_IC_NO);
        pt.put("GATHER_INFO.IC.IC_DR_CUSTOMS_NO", gatherInfo.IC_DR_CUSTOMS_NO);
        pt.put("GATHER_INFO.IC.IC_CO_CUSTOMS_NO", gatherInfo.IC_CO_CUSTOMS_NO);
        pt.put("GATHER_INFO.IC.IC_BILL_NO", gatherInfo.IC_BILL_NO);
        pt.put("GATHER_INFO.IC.IC_GROSS_WT", gatherInfo.IC_GROSS_WT);
        pt.put("GATHER_INFO.IC.IC_VE_CUSTOMS_NO", gatherInfo.IC_VE_CUSTOMS_NO);
        pt.put("GATHER_INFO.IC.IC_VE_NAME", gatherInfo.IC_VE_NAME);
        pt.put("GATHER_INFO.IC.IC_CONTA_ID", gatherInfo.IC_CONTA_ID);
        pt.put("GATHER_INFO.IC.IC_ESEAL_ID", gatherInfo.IC_ESEAL_ID);
        pt.put("GATHER_INFO.IC.IC_REG_DATETIME", gatherInfo.IC_REG_DATETIME);
        pt.put("GATHER_INFO.IC.IC_PER_DAY_DUE", gatherInfo.IC_PER_DAY_DUE);
        pt.put("GATHER_INFO.IC.IC_FORM_TYPE", gatherInfo.IC_FORM_TYPE);

        pt.put("GATHER_INFO.WEIGHT.GROSS_WT", gatherInfo.GROSS_WT);

        if (!gatherInfo.PCAR_NO.empty())
            pt.put("GATHER_INFO.CAR.VE_NAME", gatherInfo.PCAR_NO);
        else
            pt.put("GATHER_INFO.CAR.VE_NAME", gatherInfo.VE_NAME);


        pt.put("GATHER_INFO.CAR.CAR_EC_NO", gatherInfo.CAR_EC_NO);
        pt.put("GATHER_INFO.CAR.CAR_EC_NO2", gatherInfo.CAR_EC_NO2);
        pt.put("GATHER_INFO.CAR.VE_CUSTOMS_NO", gatherInfo.VE_CUSTOMS_NO);
        pt.put("GATHER_INFO.CAR.VE_WT", gatherInfo.VE_WT);

        pt.put("GATHER_INFO.CONTA.CONTA_NUM", gatherInfo.CONTA_NUM);
        pt.put("GATHER_INFO.CONTA.CONTA_RECO", gatherInfo.CONTA_RECO);
        pt.put("GATHER_INFO.CONTA.CONTA_ID_F", gatherInfo.CONTA_ID_F);
        pt.put("GATHER_INFO.CONTA.CONTA_ID_B", gatherInfo.CONTA_ID_B);

        pt.put("GATHER_INFO.SEAL.ESEAL_ID", gatherInfo.ESEAL_ID);
        pt.put("GATHER_INFO.SEAL.SEAL_KEY", gatherInfo.SEAL_KEY);

        // 格式化输出，指定编码（默认utf-8）
        boost::property_tree::xml_writer_settings<char> settings('\t', 1, "GB2312");
        //write_xml(filename, pt, std::locale(), settings);

        ostringstream oss;
        write_xml(oss, pt, settings);

        string strGatherXml = oss.str();

        int nXmlLen = strGatherXml.size(); // strSendXml.size();
        int PackDataLen = 34 + 4 + nXmlLen + 2;

        boost::shared_array<BYTE> spPacketData;
        spPacketData.reset(new BYTE[PackDataLen + 1]);
        bzero(spPacketData.get(), PackDataLen + 1);

        BYTE *p = spPacketData.get();
        memcpy(p, packetHead, 4);
        p += 4;

        memcpy(p, (BYTE*) & PackDataLen, sizeof (int));
        p += 4;

        char szDataType[2] = {0x21, 0x00};
        memcpy(p, szDataType, 1);
        p += 1;

        memcpy(p, gatherInfo.AREA_ID.c_str(), gatherInfo.AREA_ID.size());
        p += 10;

        memcpy(p, gatherInfo.CHNL_NO.c_str(), gatherInfo.CHNL_NO.size());
        p += 10;

        memcpy(p, gatherInfo.IE_FLAG.c_str(), 1);
        p += 1;

        memset(p, 0x00, 4);
        p += 4;

        memcpy(p, &nXmlLen, 4);
        p += 4;

        memcpy(p, strGatherXml.c_str(), nXmlLen);
        p += nXmlLen;
        memcpy(p, packetEnd, 2);
        // end

        memcpy(szPackedData, (char*) spPacketData.get(), PackDataLen);
        nPacketDataLen = PackDataLen;
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

    char m_szResultCtrlXML[4096] = {0};

    if (strcmp(szPlatName, "CUSTOMS") == 0)
    {
        m_pBasicDataAccess->RecordPassResult(pDatabase, pData, &passResult);


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


        int nPASS_FLAG = 0;
        std::string strControlData = "";
        std::string strLED_CODE = "";
        std::string strLED_MSG = "";
        std::string strBROADCAST_CODE = "";
        std::string strBROADCAST_MSG = "";
        std::string strEventID = "";
        if (passResult.szCheckResult[1] == '0')
        {
            nPASS_FLAG = 0;
            strLED_CODE = "00";
            strLED_MSG = passResult.szOPHint;
            strBROADCAST_CODE = "00";
            strBROADCAST_MSG = passResult.szOPHint;
            strEventID = "EC_FREE_PLATFORM";

            //                    if (strLED_MSG == "不查验，请直行")
            //                    {
            //                        strLED_CODE         = "1";
            //                        strBROADCAST_CODE   = "1";
            //                    }
            //if (strLED_MSG == "放行抬杆")
            strLED_MSG = strBROADCAST_MSG = "放行抬杆";
            {
                strLED_CODE = "3";
                strBROADCAST_CODE = "3";
            }
        }
        else
        {
            nPASS_FLAG = 1;
            strLED_CODE = "11";
            strLED_MSG = passResult.szOPHint;
            strBROADCAST_CODE = "11";
            strBROADCAST_MSG = passResult.szOPHint;
            strEventID = "EC_FORBID_PLATFORM";

            //                    if (strLED_MSG == "请到查验区进行查验")
            //                    {
            //                        strLED_CODE         = "2";
            //                        strBROADCAST_CODE   = "2";
            //                    }
            //else if (strLED_MSG == "不放行")
            strLED_MSG = strBROADCAST_MSG = passResult.szOPHint; // "不放行";
            {
                strLED_CODE = "4";
                strBROADCAST_CODE = "4";
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


        m_pBasicDataAccess->RecordPassResult(pDatabase, passResult.szSeqNo, nPASS_FLAG, strLED_MSG.c_str(), strEventID.c_str(), m_szResultCtrlXML);

    }

    if (m_pCrtlCmdCallback)
    {

        m_pCrtlCmdCallback(passResult.szAeraID, passResult.szChlNo, passResult.szIEType, passResult.szSeqNo, m_szResultCtrlXML, strlen(m_szResultCtrlXML) + 1);
    }

    return 0;
}

int CJZPlatformProxy::BuildCtrlData(CppMySQL3DB* pDatabase, char* szChannelNo, char* szPassSequence, char* pData, char* szMemo)
{
    char m_szResultCtrlXML[4096] = {0};

    string strFullChannelID(szChannelNo);

    string strAreaID = strFullChannelID.substr(0, 10);
    string strChannelNo = strFullChannelID.substr(10, 10);
    string strIEType = strFullChannelID.substr(20, 1);



    if (strcmp(pData, "00") == 0)
    {
        m_pBasicDataAccess->RecordManPass(pDatabase, szPassSequence, szMemo);

        sprintf(m_szResultCtrlXML, "%s", "<CONTROL_INFO>");
        sprintf(m_szResultCtrlXML, "%s <EVENT type=\"CAR\" hint=\"临时车辆放行\" eventid=\"%s\"></EVENT>", m_szResultCtrlXML, "M000000001");
        sprintf(m_szResultCtrlXML, "%s </CONTROL_INFO>", m_szResultCtrlXML);
    }

    if (m_pCrtlCmdCallback)
    {
        m_pCrtlCmdCallback((char*) strAreaID.c_str(), (char*) strChannelNo.c_str(), (char*) strIEType.c_str(), szPassSequence, m_szResultCtrlXML, strlen(m_szResultCtrlXML) + 1);
    }

}

int CJZPlatformProxy::BuildExceptionFreeData(CppMySQL3DB* pDatabase, char* szChannelNo, char* szPassSequence, char* pData, int nDataLen, char* szMemo)
{
    if (pData == NULL || nDataLen <= 0)
        return 0;

    char m_szResultCtrlXML[4096] = {0};

    string strFullChannelID(szChannelNo);

    string strAreaID = strFullChannelID.substr(0, 10);
    string strChannelNo = strFullChannelID.substr(10, 10);
    string strIEType = strFullChannelID.substr(20, 1);

    {
        sprintf(m_szResultCtrlXML, "%s", "<?xml version=\"1.0\" encoding=\"GB2312\"?>");


        char szExceptionData[10 * 1024] = {0};
        sprintf(szExceptionData,
                "<CONTROL_INFO>\n"
                "<EVENT type = \"goods\" hint = \"货物放行\" eventid = \"EC_FREE_MAN\">\n"
                "<LED>\n"
                "<LED_CODE>00</LED_CODE>\n"
                "<LED_MSG>直接放行</LED_MSG>\n"
                "</LED>\n"
                "<BROADCAST>\n"
                "<BROADCAST_CODE>00</BROADCAST_CODE>\n"
                "<BROADCAST_MSG>直接放行</BROADCAST_MSG>\n"
                "</BROADCAST>\n"
                "<PRINT>\n"
                "<VENAME></VENAME>\n"
                "<CONTA_ID_F></CONTA_ID_F>\n"
                "<CONTA_ID_B></CONTA_ID_B>\n"
                "<CONTA_MODEL_F></CONTA_MODEL_F>\n"
                "<CONTA_MODEL_B></CONTA_MODEL_B>\n"
                "<Oper_Name></Oper_Name>\n"
                "</PRINT>\n"
                "</EVENT>\n"
                "</CONTROL_INFO>\n");


        sprintf(m_szResultCtrlXML, "%s%s",m_szResultCtrlXML,szExceptionData);
        m_pBasicDataAccess->RecordPassResult(pDatabase, szPassSequence, 99, "货物放行", "EC_FREE_MAN", m_szResultCtrlXML);
    }

    if (m_pCrtlCmdCallback)
    {
        m_pCrtlCmdCallback((char*) strAreaID.c_str(), (char*) strChannelNo.c_str(), (char*) strIEType.c_str(), szPassSequence, m_szResultCtrlXML, strlen(m_szResultCtrlXML) + 1);
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

int CJZPlatformProxy::PackUploadDataProxy(char* szPlatName, char* pData, int nLen, char* szPackedData, int& nPacketDataLen)
{
    if (strcmp(szPlatName, "CUSTOMS") == 0)
    {
        structGatherInfo gatherInfo;
        m_pBasicDataAccess->RecordPassVehicleInfoProxy(gatherInfo, pData);

        // later transfer
        const char packetHead[4] = {(char) 0xE2, (char) 0x5C, (char) 0x4B, (char) 0x89};
        const char packetEnd[2] = {(char) 0xFF, (char) 0xFF};
        using boost::property_tree::ptree;
        ptree pt;
        ptree pattr1;
        ptree pattr2;
        ptree pattr3;
        ptree pattr4;

        pattr1.add<std::string>("<xmlattr>.AREA_ID", gatherInfo.AREA_ID);
        pattr1.add<std::string>("<xmlattr>.CHNL_NO", gatherInfo.CHNL_NO);
        pattr1.add<std::string>("<xmlattr>.I_E_TYPE", gatherInfo.IE_FLAG);
        pattr1.add<std::string>("<xmlattr>.SEQ_NO", gatherInfo.SEQ_NO);
        pattr1.add<std::string>("<xmlattr>.CHNL_TYPE", gatherInfo.CHNL_TYPE);

        pt.add_child("GATHER_INFO", pattr1);
        pt.put("GATHER_INFO.IC.DR_IC_NO", gatherInfo.DR_IC_NO);
        pt.put("GATHER_INFO.IC.IC_DR_CUSTOMS_NO", gatherInfo.IC_DR_CUSTOMS_NO);
        pt.put("GATHER_INFO.IC.IC_CO_CUSTOMS_NO", gatherInfo.IC_CO_CUSTOMS_NO);
        pt.put("GATHER_INFO.IC.IC_BILL_NO", gatherInfo.IC_BILL_NO);
        pt.put("GATHER_INFO.IC.IC_GROSS_WT", gatherInfo.IC_GROSS_WT);
        pt.put("GATHER_INFO.IC.IC_VE_CUSTOMS_NO", gatherInfo.IC_VE_CUSTOMS_NO);
        pt.put("GATHER_INFO.IC.IC_VE_NAME", gatherInfo.IC_VE_NAME);
        pt.put("GATHER_INFO.IC.IC_CONTA_ID", gatherInfo.IC_CONTA_ID);
        pt.put("GATHER_INFO.IC.IC_ESEAL_ID", gatherInfo.IC_ESEAL_ID);
        pt.put("GATHER_INFO.IC.IC_REG_DATETIME", gatherInfo.IC_REG_DATETIME);
        pt.put("GATHER_INFO.IC.IC_PER_DAY_DUE", gatherInfo.IC_PER_DAY_DUE);
        pt.put("GATHER_INFO.IC.IC_FORM_TYPE", gatherInfo.IC_FORM_TYPE);

        pt.put("GATHER_INFO.WEIGHT.GROSS_WT", gatherInfo.GROSS_WT);

        if (!gatherInfo.PCAR_NO.empty())
            pt.put("GATHER_INFO.CAR.VE_NAME", gatherInfo.PCAR_NO);
        else
            pt.put("GATHER_INFO.CAR.VE_NAME", gatherInfo.VE_NAME);


        pt.put("GATHER_INFO.CAR.CAR_EC_NO", gatherInfo.CAR_EC_NO);
        pt.put("GATHER_INFO.CAR.CAR_EC_NO2", gatherInfo.CAR_EC_NO2);
        pt.put("GATHER_INFO.CAR.VE_CUSTOMS_NO", gatherInfo.VE_CUSTOMS_NO);
        pt.put("GATHER_INFO.CAR.VE_WT", gatherInfo.VE_WT);

        pt.put("GATHER_INFO.CONTA.CONTA_NUM", gatherInfo.CONTA_NUM);
        pt.put("GATHER_INFO.CONTA.CONTA_RECO", gatherInfo.CONTA_RECO);
        pt.put("GATHER_INFO.CONTA.CONTA_ID_F", gatherInfo.CONTA_ID_F);
        pt.put("GATHER_INFO.CONTA.CONTA_ID_B", gatherInfo.CONTA_ID_B);

        pt.put("GATHER_INFO.SEAL.ESEAL_ID", gatherInfo.ESEAL_ID);
        pt.put("GATHER_INFO.SEAL.SEAL_KEY", gatherInfo.SEAL_KEY);

        // 格式化输出，指定编码（默认utf-8）
        boost::property_tree::xml_writer_settings<char> settings('\t', 1, "GB2312");
        //write_xml(filename, pt, std::locale(), settings);

        ostringstream oss;
        write_xml(oss, pt, settings);

        string strGatherXml = oss.str();

        int nXmlLen = strGatherXml.size(); // strSendXml.size();
        int PackDataLen = 34 + 4 + nXmlLen + 2;

        boost::shared_array<BYTE> spPacketData;
        spPacketData.reset(new BYTE[PackDataLen + 1]);
        bzero(spPacketData.get(), PackDataLen + 1);

        BYTE *p = spPacketData.get();
        memcpy(p, packetHead, 4);
        p += 4;

        memcpy(p, (BYTE*) & PackDataLen, sizeof (int));
        p += 4;

        char szDataType[2] = {0x21, 0x00};
        memcpy(p, szDataType, 1);
        p += 1;

        memcpy(p, gatherInfo.AREA_ID.c_str(), gatherInfo.AREA_ID.size());
        p += 10;

        memcpy(p, gatherInfo.CHNL_NO.c_str(), gatherInfo.CHNL_NO.size());
        p += 10;

        memcpy(p, gatherInfo.IE_FLAG.c_str(), 1);
        p += 1;

        memset(p, 0x00, 4);
        p += 4;

        memcpy(p, &nXmlLen, 4);
        p += 4;

        memcpy(p, strGatherXml.c_str(), nXmlLen);
        p += nXmlLen;
        memcpy(p, packetEnd, 2);
        // end

        memcpy(szPackedData, (char*) spPacketData.get(), PackDataLen);
        nPacketDataLen = PackDataLen;
    }

    return 0;
    return 0;
}