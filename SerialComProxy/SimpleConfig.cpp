#include "SimpleConfig.h"
#include <errno.h>
#include "MyLog.h"
#include <string>
#include <syslog.h>
using namespace std;

char CSimpleConfig::EXE_FULL_PATH[256];
std::string CSimpleConfig::CUSTOMS_PLATFORM_LISTEN_PORT;
std::string CSimpleConfig::SERIAL_SERVER_IP;
std::string CSimpleConfig::GATHER_PORT;
std::string CSimpleConfig::PASS_PORT;
std::string CSimpleConfig::CENTER_CONTROLER_SERIAL;
std::string CSimpleConfig::CENTER_LISTEN_PORT;
std::string CSimpleConfig::DATABASE_SERVER_IP;
std::string CSimpleConfig::DATABASE_SERVER_PORT;
std::string CSimpleConfig::DATABASE_USER;
std::string CSimpleConfig::DATABASE_PASSWORD;
std::string CSimpleConfig::PIC_PATH;
std::string CSimpleConfig::MAX_SIZE;
std::string CSimpleConfig::CENTER_SERVER_IP;
std::string CSimpleConfig::CENTER_SERVER_PORT; 


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
        syslog(LOG_DEBUG, "********get process absolute path failed,errno:%d !\n", errno);
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

    sprintf(cpPath, "%sSerialComProxyConfig.xml", strFullExeName.c_str());

    //简单配置部分，一层，用辅助类直接读
    TiXmlDocument doc(cpPath);
    doc.LoadFile();

    TiXmlNode* node = 0;
    TiXmlElement* areaInfoElement = 0;
    TiXmlElement* itemElement = 0;

    TiXmlHandle docHandle( &doc );
    TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("ConfigInfo");
    TiXmlHandle CUSTOMS_PLATFORM_LISTEN_PORTHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("CUSTOMS_PLATFORM_LISTEN_PORT", 0).FirstChild();
    TiXmlHandle SERIAL_SERVER_IPHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("SERIAL_SERVER_IP", 0).FirstChild();
    TiXmlHandle GATHER_PORTHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("GATHER_PORT", 0).FirstChild();
    TiXmlHandle PASS_PORTHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("PASS_PORT", 0).FirstChild();
    TiXmlHandle CENTER_CONTROLER_SERIALHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("CENTER_CONTROLER_SERIAL", 0).FirstChild();
    TiXmlHandle CENTER_LISTEN_PORTHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("CENTER_LISTEN_PORT", 0).FirstChild();
    TiXmlHandle DATABASE_SERVER_IPHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("DATABASE_SERVER_IP", 0).FirstChild();
    TiXmlHandle DATABASE_SERVER_PORTHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("DATABASE_SERVER_PORT", 0).FirstChild();
    TiXmlHandle DATABASE_USERHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("DATABASE_USER", 0).FirstChild();
    TiXmlHandle DATABASE_PASSWORDHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("DATABASE_PASSWORD", 0).FirstChild();
    TiXmlHandle PIC_PATHHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("PIC_PATH", 0).FirstChild();
    TiXmlHandle MAX_SIZEHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("MAX_SIZE", 0).FirstChild();

    TiXmlHandle CENTER_SERVER_IPHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("CENTER_SERVER_IP", 0).FirstChild();
    TiXmlHandle CENTER_SERVER_PORTHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("CENTER_SERVER_PORT", 0).FirstChild();

    if (CUSTOMS_PLATFORM_LISTEN_PORTHandle.Node() != NULL)
            CSimpleConfig::CUSTOMS_PLATFORM_LISTEN_PORT = CUSTOMS_PLATFORM_LISTEN_PORTHandle.Text()->Value();
    if (SERIAL_SERVER_IPHandle.Node() != NULL)
            CSimpleConfig::SERIAL_SERVER_IP = SERIAL_SERVER_IPHandle.Text()->Value();
    if (GATHER_PORTHandle.Node() != NULL)
            CSimpleConfig::GATHER_PORT = GATHER_PORTHandle.Text()->Value();
    if (PASS_PORTHandle.Node() != NULL)
            CSimpleConfig::PASS_PORT = PASS_PORTHandle.Text()->Value();
    if (CENTER_CONTROLER_SERIALHandle.Node() != NULL)
            CSimpleConfig::CENTER_CONTROLER_SERIAL = CENTER_CONTROLER_SERIALHandle.Text()->Value();
    if (CENTER_LISTEN_PORTHandle.Node() != NULL)
            CSimpleConfig::CENTER_LISTEN_PORT = CENTER_LISTEN_PORTHandle.Text()->Value();
    
    if (DATABASE_SERVER_IPHandle.Node() != NULL)
            CSimpleConfig::DATABASE_SERVER_IP = DATABASE_SERVER_IPHandle.Text()->Value();
    if (DATABASE_SERVER_PORTHandle.Node() != NULL)
            CSimpleConfig::DATABASE_SERVER_PORT = DATABASE_SERVER_PORTHandle.Text()->Value();
    if (DATABASE_USERHandle.Node() != NULL)
            CSimpleConfig::DATABASE_USER = DATABASE_USERHandle.Text()->Value();
    if (DATABASE_PASSWORDHandle.Node() != NULL)
            CSimpleConfig::DATABASE_PASSWORD = DATABASE_PASSWORDHandle.Text()->Value();
    if (CUSTOMS_PLATFORM_LISTEN_PORTHandle.Node() != NULL)
            CSimpleConfig::CUSTOMS_PLATFORM_LISTEN_PORT = CUSTOMS_PLATFORM_LISTEN_PORTHandle.Text()->Value();
    if (PIC_PATHHandle.Node() != NULL)
            CSimpleConfig::PIC_PATH = PIC_PATHHandle.Text()->Value();   
    
    if (MAX_SIZEHandle.Node() != NULL)
            CSimpleConfig::MAX_SIZE = MAX_SIZEHandle.Text()->Value();        
    if (CENTER_SERVER_IPHandle.Node() != NULL)
            CSimpleConfig::CENTER_SERVER_IP = CENTER_SERVER_IPHandle.Text()->Value();    
    if (CENTER_SERVER_PORTHandle.Node() != NULL)
            CSimpleConfig::CENTER_SERVER_PORT = CENTER_SERVER_PORTHandle.Text()->Value();      
}


