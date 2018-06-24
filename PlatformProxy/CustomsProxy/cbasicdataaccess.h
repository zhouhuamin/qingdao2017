/* 
 * File:   cbasicdataaccess.h
 * Author: root
 *
 * Created on 2014年11月22日, 下午9:59
 */

#ifndef _CBASICDATAACCESS_H
#define	_CBASICDATAACCESS_H

#include "tinyXML/tinyxml.h"
#include "../include/cppmysql.h"

#include <string>
using std::string;


struct T_PassResultInfo {
    char szAeraID[32];
    char szChlNo[32];
    char szIEType[4];
    char szSeqNo[24];
    char szCheckResult[24];
    char szSealID[128];
    char szSealKey[128];
    char szOpenTimes[32];
    char szFormID[32];
    char szOPHint[64];
    char szVEName[64];
    char szGPSID[64];
    char szOriginCustoms[128];
    char szDestCustoms[128];
};

struct structGatherInfo
{
	std::string AREA_ID;
	std::string CHNL_NO;
	std::string SEQ_NO;
	std::string IE_FLAG;
        std::string CHNL_TYPE;
        std::string DR_IC_NO;
        std::string IC_DR_CUSTOMS_NO;
        std::string IC_CO_CUSTOMS_NO;
        std::string IC_BILL_NO;
        std::string IC_GROSS_WT;
        std::string IC_VE_CUSTOMS_NO;
        std::string IC_VE_NAME;
        std::string IC_CONTA_ID;
        std::string IC_ESEAL_ID;
        std::string IC_REG_DATETIME;
        std::string IC_PER_DAY_DUE;
        std::string IC_FORM_TYPE;
        std::string GROSS_WT;
        std::string VE_NAME;
        std::string CAR_EC_NO;
        std::string CAR_EC_NO2;
        std::string VE_CUSTOMS_NO;
        std::string VE_WT;
        std::string CONTA_NUM;
        std::string CONTA_RECO;
        std::string CONTA_ID_F;
        std::string CONTA_ID_B;
        std::string ESEAL_ID;
        std::string SEAL_KEY;        
        
        
	std::string GATHER_TIME;
	std::string ECAR_NO;
	std::string ETAG_ID;
	std::string PCAR_NO;
	std::string PCAR_NO_PICNAME;
	std::string PCAR_PICNAME;
};


struct structVEHICLES_INFO
{
	std::string AREA_ID;
	std::string CHNL_NO;
	std::string SEQ_NO;
	std::string I_E_TYPE;
	std::string CAR_EC_NO;
	std::string DR_IC_NO;
	std::string VE_NAME;
	std::string CONTA_ID_F;
	std::string CONTA_ID_B;
	std::string GROSS_WT;
        std::string NOTE;
};

class CBasicDataAccess{
public:
    CBasicDataAccess();
    virtual ~CBasicDataAccess();

    int RecordPassVehicleInfo(structGatherInfo & gatherInfo, CppMySQL3DB* pDataabse, char* szXML,int nIsRegather=0);
    int RecordPassResult(CppMySQL3DB* pDataabse, char* szXML,T_PassResultInfo* pPassResult);
    int RecordManPass(CppMySQL3DB* pDatabase,char* szPassSequence,char* szMemo );
    int RecordPassResult(CppMySQL3DB* pDataabse,char* szSequenceNo,int nResult, const char* szHint, const char *szEventID, char *szXmlInfo);
    //--------------------------
    int RecordPassVehicleInfoProxy(structGatherInfo & gatherInfo,char* szXML);
private:
    int handleICTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, CppMySQL3DB* pDatabase, TiXmlElement *item);
    int handleWeightTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, CppMySQL3DB* pDatabase, TiXmlElement *item);
    int handleCarTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, CppMySQL3DB* pDatabase, TiXmlElement *item);
    int handleTrailerTag(char* area_id, char* chnl_no, char* ie_type, char* seq_no, CppMySQL3DB* pDatabase, TiXmlElement *item);
    int handleContaTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, CppMySQL3DB* pDatabase, TiXmlElement *item);
    int handleSealTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, CppMySQL3DB* pDatabase, TiXmlElement *item);
    int handleAll(char* area_id, char* chnl_no, char* ie_type, char* seq_no, CppMySQL3DB* pDatabase, char* szXML,int nIsRegather=0);
    int handlePCarTag(structGatherInfo & gatherInfo, CppMySQL3DB* pDatabase, TiXmlElement *item);
    int handleResultAll(char* area_id, char* chnl_no, char* ie_type, char* seq_no, CppMySQL3DB* pDatabase, char* szXML);
    int handlePassResult(T_PassResultInfo& resultInfo, CppMySQL3DB* pDatabase);
    
    
    //---------------------------------------------------------------------
    int handleICTagProxy(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item);
    int handleWeightTagProxy(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item);
    int handleCarTagProxy(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item);
    int handleTrailerTagProxy(char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item);
    int handleContaTagProxy(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item);
    int handleSealTagProxy(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item);
    int handlePCarTagProxy(structGatherInfo & gatherInfo, TiXmlElement *item);
    int queryCarTag(structGatherInfo & gatherInfo, CppMySQL3DB* pDatabase, structVEHICLES_INFO &vehicleInfo);
    
};

#endif	/* _CBASICDATAACCESS_H */

