#include "SimpleConfig.h"
#include <errno.h>
#include "MyLog.h"
#include <string>
using namespace std;

int CSimpleConfig::CUSTOMS_PLATFORM_LISTEN_PORT;

int CSimpleConfig::CENTER_LISTEN_PORT;

char CSimpleConfig::DATABASE_SERVER_IP[32];
char CSimpleConfig::DATABASE_SERVER_PORT;
char CSimpleConfig::DATABASE_USER[32];
char CSimpleConfig::DATABASE_PASSWORD[32];

char CSimpleConfig::PIC_SAVE_PATH[256];
int CSimpleConfig::PIC_MAX_SIZE;
char CSimpleConfig::EXE_FULL_PATH[256];


char CSimpleConfig::EVENT_SERVER_IP[32];
int CSimpleConfig::EVENT_SERVER_PORT;

std::map<string, T_PlatformInfo*> CSimpleConfig::m_StaPlatformMap;

CSimpleConfig::CSimpleConfig()
{
}

CSimpleConfig::~CSimpleConfig()
{
}

int CSimpleConfig::GetProcessPath(char* cpPath)
{
    int iCount;

    iCount = readlink("/proc/self/exe", cpPath, 256);

    if(iCount < 0 || iCount >= 256)
    {
        CMyLog::m_pLog->_XGSysLog("********get process absolute path failed,errno:%d !\n", errno);
        return -1;
    }

    cpPath[iCount] = '\0';

    return 0;
}

void CSimpleConfig::get_config()
{
    char exeFullPath[256];

    GetProcessPath(exeFullPath); //得到程序模块名称，全路径

    string strFullExeName(exeFullPath);
    int nLast = strFullExeName.find_last_of("/");

    strFullExeName = strFullExeName.substr(0, nLast + 1);
    strcpy(EXE_FULL_PATH, strFullExeName.c_str());

    char cpPath[256] = {0};
    char temp[256] = {0};

    sprintf(cpPath, "%sCenterConfig.xml", strFullExeName.c_str());

    //简单配置部分，一层，用辅助类直接读

    XMLConfig xmlConfig(cpPath);


    string plat_form_listen_port = xmlConfig.GetValue("CUSTOMS_PLATFORM_LISTEN_PORT");
    CUSTOMS_PLATFORM_LISTEN_PORT = atoi(plat_form_listen_port.c_str());

    string center_port = xmlConfig.GetValue("CENTER_LISTEN_PORT");
    CENTER_LISTEN_PORT = atoi(center_port.c_str());


    strcpy(EVENT_SERVER_IP, xmlConfig.GetValue("EVENT_SERVER_IP").c_str());

    string event_port = xmlConfig.GetValue("EVENT_SERVER_PORT").c_str();
    EVENT_SERVER_PORT = atoi(event_port.c_str());


//    strcpy(DATABASE_SERVER_IP, xmlConfig.GetValue("DATABASE_SERVER_IP").c_str());
//
//    string database_port = xmlConfig.GetValue("DATABASE_SERVER_PORT");
//    DATABASE_SERVER_PORT = atoi(database_port.c_str());
//
//
//    strcpy(DATABASE_USER, xmlConfig.GetValue("DATABASE_USER").c_str());
//    strcpy(DATABASE_PASSWORD, xmlConfig.GetValue("DATABASE_PASSWORD").c_str());


    strcpy(PIC_SAVE_PATH, xmlConfig.GetValue("PIC_PATH").c_str());

    string max_size = xmlConfig.GetValue("MAX_SIZE");
    PIC_MAX_SIZE = atoi(max_size.c_str());


    //platform config part

    //这里采用TiXML来读节点数据
     TiXmlDocument doc;
    bool bopen = doc.LoadFile(cpPath);

    TiXmlElement* pRootElem = doc.RootElement();
    TiXmlElement* item_platformlist = pRootElem->FirstChildElement("PLATFORM_LIST");
    if(!item_platformlist)
    {
        return;
    }

    TiXmlElement* item_platform = item_platformlist->FirstChildElement("PLATFORM");
    if(!item_platform)
    {
        return;
    }


    T_PlatformInfo* pPlatform = new T_PlatformInfo;

    TiXmlElement* item_platform_name = item_platform->FirstChildElement("PLATFORM_NAME");
    if(!item_platform_name)
    {
        return;
    }

    char* pText = (char*) item_platform_name->GetText();

    if(pText)
    {
        strcpy(pPlatform->platform_name, pText);
    }


    TiXmlElement* item_platform_ip = item_platform->FirstChildElement("PLATFORM_IP");
    if(!item_platform_ip)
    {
        return;
    }

    pText = (char*) item_platform_ip->GetText();

    if(pText)
    {
        strcpy(pPlatform->platform_ip, pText);
    }

    TiXmlElement* item_platform_port = item_platform->FirstChildElement("PLATFORM_PORT");
    if(!item_platform_port)
    {
        return;
    }

    pText = (char*) item_platform_port->GetText();

    if(pText)
    {
        pPlatform->platform_port = atoi(pText);
    }


    m_StaPlatformMap[string(pPlatform->platform_name)] = pPlatform;


    printf("read platform name: %s,ip:%s,port:%d\n", pPlatform->platform_name, pPlatform->platform_ip, pPlatform->platform_port);


    while ((item_platform = item_platform->NextSiblingElement()) != NULL)
    {

        T_PlatformInfo* pPlatform = new T_PlatformInfo;

        TiXmlElement* item_platform_name = item_platform->FirstChildElement("PLATFORM_NAME");
        if(!item_platform_name)
        {
            return;
        }

        char* pText = (char*) item_platform_name->GetText();

        if(pText)
        {
            strcpy(pPlatform->platform_name, pText);
        }


      

        TiXmlElement* item_platform_ip = item_platform->FirstChildElement("PLATFORM_IP");
        if(!item_platform_ip)
        {
            return;
        }

        pText = (char*) item_platform_ip->GetText();

        if(pText)
        {
            strcpy(pPlatform->platform_ip, pText);
        }

        TiXmlElement* item_platform_port = item_platform->FirstChildElement("PLATFORM_PORT");
        if(!item_platform_port)
        {
            return;
        }

        pText = (char*) item_platform_port->GetText();

        if(pText)
        {
            pPlatform->platform_port = atoi(pText);
        }


        m_StaPlatformMap[string(pPlatform->platform_name)] = pPlatform;


        printf("read platform name: %s,ip:%s,port:%d\n", pPlatform->platform_name, pPlatform->platform_ip, pPlatform->platform_port);

    }

}


