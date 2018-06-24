/* 
 * File:   Encrypter.cpp
 * Author: root
 * 
 * Created on 2017年1月13日, 下午7:43
 */

#include "Encrypter.h"

#include <cstdlib>
#include <string.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <cassert> 


using namespace std;

Encrypter::Encrypter()
{
}

Encrypter::Encrypter(const Encrypter& orig)
{
}

Encrypter::~Encrypter()
{
}


//加密  
int Encrypter::EncodeRSAKeyFile(const std::string& strPemFileName, const std::string &strData, char *pOutData, int &nOutLen)  
{  
    if (strPemFileName.empty() || strData.empty())  
    {   
        return -1;  
    }  
    FILE* hPriKeyFile = fopen(strPemFileName.c_str(), "rb");  
    if( hPriKeyFile == NULL )  
    {  
        return -1;   
    }  
    std::string strRet;  
    RSA* pRSAPriKey = RSA_new();  
            
            
            
    if(PEM_read_RSAPrivateKey(hPriKeyFile, &pRSAPriKey, 0, 0) == NULL)  
    {  
        fclose(hPriKeyFile);   
        return -1;  
    }  
  
    int nLen = RSA_size(pRSAPriKey);  
    char* pEncode = new char[nLen + 1];  
    int ret = RSA_private_encrypt(strData.size(), (const unsigned char*)strData.c_str(), (unsigned char*)pEncode, pRSAPriKey, RSA_PKCS1_PADDING);  
    if (ret >= 0)  
    {  
        nOutLen = ret;
        memcpy(pOutData, pEncode, ret); 
    }  
    delete[] pEncode;  
    RSA_free(pRSAPriKey);  
    fclose(hPriKeyFile);  
    CRYPTO_cleanup_all_ex_data();   
    return 0;  
}

  
//解密  
std::string Encrypter::DecodeRSAKeyFile(const std::string& strPemFileName, char *pInData, int nOutLen)  
{  
    if (strPemFileName.empty())  
    {  
        //assert(false);  
        return "";  
    }  
    FILE* hPubKeyFile = fopen(strPemFileName.c_str(),"rb");  
    if( hPubKeyFile == NULL )  
    {  
        //assert(false);  
        return "";  
    }  
    std::string strRet;  
    RSA* pRSAPublicKey = RSA_new();  
    if(PEM_read_RSA_PUBKEY(hPubKeyFile, &pRSAPublicKey, 0, 0) == NULL)  
    {  
        fclose(hPubKeyFile);
        //assert(false);  
        return "";  
    }  
    int nLen = RSA_size(pRSAPublicKey);  
    char* pDecode = new char[nLen+1];  
  
    int ret = RSA_public_decrypt(nOutLen, (const unsigned char*)pInData, (unsigned char*)pDecode, pRSAPublicKey, RSA_PKCS1_PADDING);  
    if(ret >= 0)  
    {  
        strRet = std::string((char*)pDecode, ret);  
    }  
    delete [] pDecode;  
    RSA_free(pRSAPublicKey);  
    fclose(hPubKeyFile);  
    CRYPTO_cleanup_all_ex_data();   
    return strRet;  
} 

std::string Encrypter::GetMessageHash(const std::string &strXml)
{
    SHA_CTX stx;
    
    int len = strXml.size();
    char buffer[1024] = {0};
    memcpy(buffer, strXml.c_str(), len);
    
    unsigned char outmd[20] = {0};

    SHA1_Init(&stx);
    {
        SHA1_Update(&stx,buffer,len);
        memset(buffer,0,sizeof(buffer));
    }   

    SHA1_Final(outmd, &stx);
    
    std::string strData = "";
    char szXX[2 + 1] = {0};
    for(int i=0; i<20; i<i++)
    {
        sprintf(szXX, "%02X", outmd[i]);
        if (i != 19)
        {
            strData += szXX;
            strData += "-";
        }
        else
        {
            strData += szXX;
        }
    }  
    return strData;
}


