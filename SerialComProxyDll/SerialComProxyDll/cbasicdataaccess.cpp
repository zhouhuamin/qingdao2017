/* 
 * File:   cbasicdataaccess.cpp
 * Author: root
 * 
 * Created on 2014å¹?1??2?? ä¸??9:59
 */

#include "cbasicdataaccess.h"
#include <time.h>
#include <sys/time.h>

CBasicDataAccess::CBasicDataAccess()
{
}

CBasicDataAccess::~CBasicDataAccess()
{
}

int CBasicDataAccess::RecordPassVehicleInfo(structGatherInfo & gatherInfo, char* szXML,int nIsRegather)
{
    TiXmlDocument xml;
    xml.Parse(szXML + 38);

    TiXmlElement *root = xml.RootElement();
    if (root == NULL)
    {
        return -1;
    }

    char* pAreaID = (char*) root->Attribute("AREA_ID");
    char* pChannelNO = (char*) root->Attribute("CHNL_NO");
    char* pSquenceNo = (char*) root->Attribute("SEQ_NO");
    char* pIEType = (char*) root->Attribute("I_E_TYPE");
    char* pCHNL_TYPE = (char*) root->Attribute("CHNL_TYPE");

    if (!pAreaID || !pChannelNO || !pSquenceNo || !pIEType)
    {
        return -1;
    }
    
    gatherInfo.AREA_ID	= pAreaID;
    gatherInfo.CHNL_NO	= pChannelNO;
    gatherInfo.IE_FLAG	= pIEType;
    gatherInfo.SEQ_NO	= pSquenceNo;


    TiXmlElement *item_ic_tag = root->FirstChildElement("IC");

    if (item_ic_tag)
    {
        handleICTag(gatherInfo, pAreaID, pChannelNO, pIEType, pSquenceNo, item_ic_tag);
    }



    TiXmlElement *item_weight_tag = root->FirstChildElement("WEIGHT");

    if (item_weight_tag)
    {
        handleWeightTag(gatherInfo, pAreaID, pChannelNO, pIEType, pSquenceNo, item_weight_tag);
    }


    TiXmlElement *item_car_tag = root->FirstChildElement("CAR");

    if (item_car_tag)
    {
        handleCarTag(gatherInfo, pAreaID, pChannelNO, pIEType, pSquenceNo, item_car_tag);
    }


    TiXmlElement *item_trailer_tag = root->FirstChildElement("TRAILER");

    if (item_trailer_tag)
    {
        handleTrailerTag(pAreaID, pChannelNO, pIEType, pSquenceNo, item_trailer_tag);
    }

    TiXmlElement *item_conta_tag = root->FirstChildElement("CONTA");

    if (item_conta_tag)
    {

        handleContaTag(gatherInfo, pAreaID, pChannelNO, pIEType, pSquenceNo, item_conta_tag);
    }


    TiXmlElement *item_seal_tag = root->FirstChildElement("SEAL");
    if (item_seal_tag)
    {
        handleSealTag(gatherInfo, pAreaID, pChannelNO, pIEType, pSquenceNo, item_seal_tag);
    }
    
    TiXmlElement *item_pcar_tag = root->FirstChildElement("OPTCAR");

    if (item_pcar_tag)
    {
        handlePCarTag(gatherInfo, item_pcar_tag);
    }

    return 0;
}

int CBasicDataAccess::RecordPassResult(char* szXML, T_PassResultInfo* pPassResult)
{
    TiXmlDocument xml;
    xml.Parse(szXML + 38);

    TiXmlElement *root = xml.RootElement();
    if (root == NULL)
    {
        return -1;
    }

    char* pAreaID = (char*) root->Attribute("AREA_ID");
    char* pChannelNO = (char*) root->Attribute("CHNL_NO");
    char* pSquenceNo = (char*) root->Attribute("SEQ_NO");
    char* pIEType = (char*) root->Attribute("I_E_TYPE");

    if (!pAreaID || !pChannelNO || !pSquenceNo || !pIEType)
    {
        return -1;
    }

//    if (szXML)
//    {
//        handleResultAll(pAreaID, pChannelNO, pIEType, pSquenceNo, pDataabse, szXML + 38);
//    }


    char check_result[32] = {0};

    TiXmlElement *item_result = root->FirstChildElement("CHECK_RESULT");
    if (item_result)
    {
        char* pText = (char*) item_result->GetText();
        if (pText)
        {
            strcpy(check_result, pText);
        }
        else
        {
            strcpy(check_result, "");
        }
    }

    char op_hint[1024] = {0};
    TiXmlElement *item_hint = root->FirstChildElement("OP_HINT");
    if (item_hint)
    {
        char* pText = (char*) item_hint->GetText();
        if (pText)
        {
            strcpy(op_hint, pText);
        }
        else
        {
            strcpy(op_hint, "");
        }
    }


    char form_id[64] = {0};
    TiXmlElement *item_formid = root->FirstChildElement("FORM_ID");
    if (item_formid)
    {
        char* pText = (char*) item_formid->GetText();
        if (pText)
        {
            strcpy(form_id, pText);
        }
        else
        {
            strcpy(form_id, "");
        }
    }

    char seal_id[128] = {0};
    char seal_key[64] = {0};
    char open_times[32] = {0};

    TiXmlElement *item_seal_tag = root->FirstChildElement("SEAL");
    if (item_seal_tag)
    {
        TiXmlElement* item_eseal_id = item_seal_tag->FirstChildElement("ESEAL_ID");
        if (item_eseal_id)
        {
            char* pText = (char*) item_eseal_id->GetText();
            if (pText)
            {
                strcpy(seal_id, pText);
            }
            else
            {
                strcpy(seal_id, "");
            }

        }

        TiXmlElement* item_key = item_seal_tag->FirstChildElement("SEAL_KEY");
        if (item_key)
        {
            char* pText = (char*) item_key->GetText();
            if (pText)
            {
                strcpy(seal_key, pText);
            }
            else
            {
                strcpy(seal_key, "");
            }

        }


        TiXmlElement* item_times = item_seal_tag->FirstChildElement("OPEN_TIMES");
        if (item_times)
        {
            char* pText = (char*) item_times->GetText();
            if (pText)
            {
                strcpy(open_times, pText);
            }
            else
            {
                strcpy(open_times, "");
            }

        }
    }




    char szGPSVEName[128] = {0};
    char szGPSID[64] = {0};
    char szOrigCustoms[128] = {0};
    char szDestCustoms[128] = {0};

    TiXmlElement *item_gps_tag = root->FirstChildElement("GPS");
    if (item_gps_tag)
    {
        TiXmlElement* item_ve_name = item_gps_tag->FirstChildElement("VE_NAME");
        if (item_ve_name)
        {
            char* pText = (char*) item_ve_name->GetText();
            if (pText)
            {
                strcpy(szGPSVEName, pText);
            }
            else
            {
                strcpy(szGPSVEName, "");
            }

        }

        TiXmlElement* item_gpsid = item_gps_tag->FirstChildElement("GPS_ID");
        if (item_gpsid)
        {
            char* pText = (char*) item_gpsid->GetText();
            if (pText)
            {
                strcpy(szGPSID, pText);
            }
            else
            {
                strcpy(szGPSID, "");
            }

        }


        TiXmlElement* item_orig = item_gps_tag->FirstChildElement("ORIGIN_CUSTOMS");
        if (item_orig)
        {
            char* pText = (char*) item_orig->GetText();
            if (pText)
            {
                strcpy(szOrigCustoms, pText);
            }
            else
            {
                strcpy(szOrigCustoms, "");
            }

        }


        TiXmlElement* item_dest = item_gps_tag->FirstChildElement("DEST_CUSTOMS");
        if (item_dest)
        {
            char* pText = (char*) item_dest->GetText();
            if (pText)
            {
                strcpy(szDestCustoms, pText);
            }
            else
            {
                strcpy(szDestCustoms, "");
            }

        }


    }


    memset(pPassResult, 0, sizeof (T_PassResultInfo));

    strcpy(pPassResult->szAeraID, pAreaID);
    strcpy(pPassResult->szChlNo, pChannelNO);
    strcpy(pPassResult->szSeqNo, pSquenceNo);
    strcpy(pPassResult->szIEType, pIEType);

    strcpy(pPassResult->szCheckResult, check_result);
    strcpy(pPassResult->szOPHint, op_hint);
    strcpy(pPassResult->szFormID, form_id);
    strcpy(pPassResult->szSealID, seal_id);
    strcpy(pPassResult->szSealKey, seal_key);
    strcpy(pPassResult->szOpenTimes, open_times);

    strcpy(pPassResult->szVEName, szGPSVEName);
    strcpy(pPassResult->szGPSID, szGPSID);
    strcpy(pPassResult->szOriginCustoms, szOrigCustoms);
    strcpy(pPassResult->szDestCustoms, szDestCustoms);

    return handlePassResult(*pPassResult);

}

int CBasicDataAccess::handleICTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item)
{
    if (!item)
    {
        return -1;
    }

    char ic_no[32] = {0};
    TiXmlElement* item_ic_no = item->FirstChildElement("DR_IC_NO");
    if (item_ic_no)
    {
        char* pText = (char*) item_ic_no->GetText();
        if (pText)
        {
            strcpy(ic_no, pText);
            gatherInfo.DR_IC_NO = pText;
        }
        else
        {
            strcpy(ic_no, "");
        }

    }


    char ic_driver[128] = {0};
    TiXmlElement* item_driver_no = item->FirstChildElement("IC_DR_CUSTOMS_NO");
    if (item_driver_no)
    {
        char* pText = (char*) item_driver_no->GetText();
        if (pText)
        {
            strcpy(ic_driver, pText);
            gatherInfo.IC_DR_CUSTOMS_NO = pText;
        }
        else
        {
            strcpy(ic_driver, "");
        }

    }


    char ic_company[128] = {0};
    TiXmlElement* item_company_no = item->FirstChildElement("IC_CO_CUSTOMS_NO");
    if (item_company_no)
    {
        char* pText = (char*) item_company_no->GetText();
        if (pText)
        {
            strcpy(ic_company, pText);
            gatherInfo.IC_CO_CUSTOMS_NO = pText;
        }
        else
        {
            strcpy(ic_company, "");
        }

    }


    char ic_bill[64] = {0};
    TiXmlElement* item_bill_no = item->FirstChildElement("IC_BILL_NO");
    if (item_bill_no)
    {
        char* pText = (char*) item_bill_no->GetText();
        if (pText)
        {
            strcpy(ic_bill, pText);
            gatherInfo.IC_BILL_NO = pText;
        }
        else
        {
            strcpy(ic_bill, "");
        }

    }


    char ic_grossweight[64] = {0};
    TiXmlElement* item_gross_weight = item->FirstChildElement("IC_GROSS_WT");
    if (item_gross_weight)
    {
        char* pText = (char*) item_gross_weight->GetText();
        if (pText)
        {
            strcpy(ic_grossweight, pText);
            gatherInfo.IC_GROSS_WT = pText;
        }
        else
        {
            strcpy(ic_grossweight, "");
        }

    }


    char ic_ve_no[64] = {0};
    TiXmlElement* item_ve_customs_no = item->FirstChildElement("IC_VE_CUSTOMS_NO");
    if (item_ve_customs_no)
    {
        char* pText = (char*) item_ve_customs_no->GetText();
        if (pText)
        {
            strcpy(ic_ve_no, pText);
            gatherInfo.IC_VE_CUSTOMS_NO = pText;
        }
        else
        {
            strcpy(ic_ve_no, "");
        }

    }


    char ic_ve_name[64] = {0};
    TiXmlElement* item_ve_name = item->FirstChildElement("IC_VE_NAME");
    if (item_ve_name)
    {
        char* pText = (char*) item_ve_name->GetText();
        if (pText)
        {
            strcpy(ic_ve_name, pText);
            gatherInfo.IC_VE_NAME = pText;
        }
        else
        {
            strcpy(ic_ve_name, "");
        }

    }


    char ic_conta_id[64] = {0};
    TiXmlElement* item_conta_id = item->FirstChildElement("IC_CONTA_ID");
    if (item_conta_id)
    {
        char* pText = (char*) item_conta_id->GetText();
        if (pText)
        {
            strcpy(ic_conta_id, pText);
            gatherInfo.IC_CONTA_ID = pText;
        }
        else
        {
            strcpy(ic_conta_id, "");
        }

    }


    char ic_eseal_id[64] = {0};
    TiXmlElement* item_eseal_id = item->FirstChildElement("IC_ESEAL_ID");
    if (item_eseal_id)
    {
        char* pText = (char*) item_eseal_id->GetText();
        if (pText)
        {
            strcpy(ic_eseal_id, pText);
            gatherInfo.IC_ESEAL_ID = pText;
        }
        else
        {
            strcpy(ic_eseal_id, "");
        }

    }


    char ic_buss_type[64] = {0};
    TiXmlElement* item_buss_type = item->FirstChildElement("IC_BUSS_TYPE");
    if (item_buss_type)
    {
        char* pText = (char*) item_buss_type->GetText();
        if (pText)
        {
            strcpy(ic_buss_type, pText);           
        }
        else
        {
            strcpy(ic_buss_type, "");
        }

    }


    char ic_ex_data[512] = {0};
    TiXmlElement* item_ex_data = item->FirstChildElement("IC_EX_DATA");
    if (item_ex_data)
    {
        char* pText = (char*) item_ex_data->GetText();
        if (pText)
        {
            strcpy(ic_ex_data, pText);
        }
        else
        {
            strcpy(ic_ex_data, "");
        }

    }
    return 0;
}

int CBasicDataAccess::handleWeightTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item)
{
    if (!item)
    {
        return -1;
    }

    char weight[32] = {0};
    TiXmlElement* item_weight = item->FirstChildElement("GROSS_WT");
    if (item_weight)
    {
        char* pText = (char*) item_weight->GetText();
        if (pText)
        {
            strcpy(weight, pText);
            gatherInfo.GROSS_WT = pText;
        }
        else
        {
            strcpy(weight, "");
        }
    }
    return 0;
}

int CBasicDataAccess::handleCarTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item)
{
    if (!item)
    {
        return -1;
    }

    char ve_name[128] = {0};
    TiXmlElement* item_ve_name = item->FirstChildElement("VE_NAME");
    if (item_ve_name)
    {
        char* pText = (char*) item_ve_name->GetText();
        if (pText)
        {
            strcpy(ve_name, pText);
            gatherInfo.VE_NAME = pText;
        }
        else
        {
            strcpy(ve_name, "");
        }

    }


    char car_ec_no[64] = {0};
    TiXmlElement* item_ec_no = item->FirstChildElement("CAR_EC_NO");
    if (item_ec_no)
    {
        char* pText = (char*) item_ec_no->GetText();
        if (pText)
        {
            strcpy(car_ec_no, pText);
            gatherInfo.CAR_EC_NO = pText;
        }
        else
        {
            strcpy(car_ec_no, "");
        }

    }


    char car_ec_no2[64] = {0};
    TiXmlElement* item_ec_no2 = item->FirstChildElement("CAR_EC_NO2");
    if (item_ec_no2)
    {
        char* pText = (char*) item_ec_no2->GetText();
        if (pText)
        {
            strcpy(car_ec_no2, pText);
            gatherInfo.CAR_EC_NO2 = pText;
        }
        else
        {
            strcpy(car_ec_no2, "");
        }

    }



    char ve_customs_no[128] = {0};
    TiXmlElement* item_customs_no = item->FirstChildElement("VE_CUSTOMS_NO");
    if (item_customs_no)
    {
        char* pText = (char*) item_customs_no->GetText();
        if (pText)
        {
            strcpy(ve_customs_no, pText);
            gatherInfo.VE_CUSTOMS_NO = pText;
        }
        else
        {
            strcpy(ve_customs_no, "");
        }

    }


    char ve_weight[64] = {0};
    TiXmlElement* item_ve_weight = item->FirstChildElement("VE_WEIGHT");
    if (item_ve_weight)
    {
        char* pText = (char*) item_ve_weight->GetText();
        if (pText)
        {
            strcpy(ve_weight, pText);
            gatherInfo.VE_WT = pText;
        }
        else
        {
            strcpy(ve_weight, "");
        }

    }
    return 0;
}

int CBasicDataAccess::handleTrailerTag(char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item)
{
    if (!item)
    {
        return -1;
    }

    char tr_ec_no[128] = {0};
    TiXmlElement* item_tr_ec_no = item->FirstChildElement("TR_EC_NO");
    if (item_tr_ec_no)
    {
        char* pText = (char*) item_tr_ec_no->GetText();
        if (pText)
        {
            strcpy(tr_ec_no, pText);
        }
        else
        {
            strcpy(tr_ec_no, "");
        }

    }


    char tr_name[64] = {0};
    TiXmlElement* item_tr_name = item->FirstChildElement("TR_NAME");
    if (item_tr_name)
    {
        char* pText = (char*) item_tr_name->GetText();
        if (pText)
        {
            strcpy(tr_name, pText);
        }
        else
        {
            strcpy(tr_name, "");
        }

    }


    char tr_wt[64] = {0};
    TiXmlElement* item_tr_wt = item->FirstChildElement("TR_WT");
    if (item_tr_wt)
    {
        char* pText = (char*) item_tr_wt->GetText();
        if (pText)
        {
            strcpy(tr_wt, pText);
        }
        else
        {
            strcpy(tr_wt, "");
        }

    }
    return 0;
}

int CBasicDataAccess::handleContaTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item)
{
    if (!item)
    {
        return -1;
    }

    char conta_num[128] = {0};
    int n_conta_num = 0;
    TiXmlElement* item_conta_num = item->FirstChildElement("CONTA_NUM");
    if (item_conta_num)
    {
        char* pText = (char*) item_conta_num->GetText();
        if (pText)
        {
            strcpy(conta_num, pText);
            n_conta_num = atoi(conta_num);
            gatherInfo.CONTA_NUM = pText;
        }
        else
        {
            strcpy(conta_num, "");
        }

    }


    char conta_reco[64] = {0};
    int n_conta_reco = 0;
    TiXmlElement* item_conta_reco = item->FirstChildElement("CONTA_RECO");
    if (item_conta_reco)
    {
        char* pText = (char*) item_conta_reco->GetText();
        if (pText)
        {
            strcpy(conta_reco, pText);
            n_conta_reco = atoi(conta_reco);
            gatherInfo.CONTA_RECO = pText;
        }
        else
        {
            strcpy(conta_reco, "");
        }

    }


    char conta_id_f[64] = {0};
    TiXmlElement* item_conta_id_f = item->FirstChildElement("CONTA_ID_F");
    if (item_conta_id_f)
    {
        char* pText = (char*) item_conta_id_f->GetText();
        if (pText)
        {
            strcpy(conta_id_f, pText);
            gatherInfo.CONTA_ID_F = pText;
        }
        else
        {
            strcpy(conta_id_f, "");
        }

    }


    char conta_id_b[64] = {0};
    TiXmlElement* item_conta_id_b = item->FirstChildElement("CONTA_ID_B");
    if (item_conta_id_b)
    {
        char* pText = (char*) item_conta_id_b->GetText();
        if (pText)
        {
            strcpy(conta_id_b, pText);
            gatherInfo.CONTA_ID_B = pText;
        }
        else
        {
            strcpy(conta_id_b, "");
        }

    }


    char conta_model_f[64] = {0};
    TiXmlElement* item_conta_model_f = item->FirstChildElement("CONTA_MODEL_F");
    if (item_conta_model_f)
    {
        char* pText = (char*) item_conta_model_f->GetText();
        if (pText)
        {
            strcpy(conta_model_f, pText);
        }
        else
        {
            strcpy(conta_model_f, "");
        }
    }

    char conta_model_b[64] = {0};
    TiXmlElement* item_conta_model_b = item->FirstChildElement("CONTA_MODEL_B");
    if (item_conta_model_b)
    {
        char* pText = (char*) item_conta_model_b->GetText();
        if (pText)
        {
            strcpy(conta_model_b, pText);
        }
        else
        {
            strcpy(conta_model_b, "");
        }

    }
    return 0;
}

int CBasicDataAccess::handleSealTag(structGatherInfo & gatherInfo, char* area_id, char* chnl_no, char* ie_type, char* seq_no, TiXmlElement *item)
{
    if (!item)
    {
        return -1;
    }

    char seal_id[128] = {0};
    TiXmlElement* item_eseal_id = item->FirstChildElement("ESEAL_ID");
    if (item_eseal_id)
    {
        char* pText = (char*) item_eseal_id->GetText();
        if (pText)
        {
            strcpy(seal_id, pText);
            gatherInfo.ESEAL_ID = pText;
        }
        else
        {
            strcpy(seal_id, "");
        }

    }


    char ic_no[64] = {0};
    TiXmlElement* item_ic_no = item->FirstChildElement("ESEAL_IC_NO");
    if (item_ic_no)
    {
        char* pText = (char*) item_ic_no->GetText();
        if (pText)
        {
            strcpy(ic_no, pText);
        }
        else
        {
            strcpy(ic_no, "");
        }

    }

    return 0;
}

int CBasicDataAccess::handleAll(char* area_id, char* chnl_no, char* ie_type, char* seq_no, char* szXML,int nIsRegather)
{
    return 0;
}

int CBasicDataAccess::handleResultAll(char* area_id, char* chnl_no, char* ie_type, char* seq_no, char* szXML)
{
    return 0;
}

int CBasicDataAccess::handlePassResult(T_PassResultInfo& resultInfo)
{
    return 0;
}

int CBasicDataAccess::RecordManPass(char* szPassSequence, char* szMemo)
{
    return 0;
}


int CBasicDataAccess::RecordPassResult(char* szSequenceNo,int nResult, const char* szHint, const char *szEventID, char *szXmlInfo)
{
    return 0;
}


int CBasicDataAccess::handlePCarTag(structGatherInfo & gatherInfo, TiXmlElement *item)
{
    if (item != NULL)
    {
        TiXmlElement* item_PCAR_NO = item->FirstChildElement("PCAR_NO");
        if (item_PCAR_NO)
        {
                char* pText = (char*) item_PCAR_NO->GetText();
                if (pText)
                {
                        gatherInfo.PCAR_NO = pText;
                }
                else
                {
                        gatherInfo.PCAR_NO	= "";
                }

        }

        TiXmlElement* item_PCAR_NO_PICNAME = item->FirstChildElement("PCAR_NO_PICNAME");
        if (item_PCAR_NO_PICNAME)
        {
                char* pText = (char*) item_PCAR_NO_PICNAME->GetText();
                if (pText)
                {
                        gatherInfo.PCAR_NO_PICNAME	= pText;
                }
                else
                {
                        gatherInfo.PCAR_NO_PICNAME	= "";
                }

        }

        TiXmlElement* item_PCAR_PICNAME = item->FirstChildElement("PCAR_PICNAME");
        if (item_PCAR_PICNAME)
        {
                char* pText = (char*) item_PCAR_PICNAME->GetText();
                if (pText)
                {
                        gatherInfo.PCAR_PICNAME	= pText;
                }
                else
                {
                        gatherInfo.PCAR_PICNAME	= "";
                }

        }
    }
    return 0;
}

