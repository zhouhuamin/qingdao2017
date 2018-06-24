#ifndef _SIMPLE_CONFIG_H_
#define	_SIMPLE_CONFIG_H_

#pragma warning(disable : 4786)	
#include <string>
#include <vector>
#include <map>
using namespace std;

#include "tinyXML/xml_config.h"

struct T_PlatformInfo
{
    char platform_name[32];
    char platform_ip[32];
    int  platform_port;
};

class CSimpleConfig
{
public:
    CSimpleConfig();

    virtual ~CSimpleConfig();

    void get_config();
    static int GetProcessPath(char* cpPath);

    static char EXE_FULL_PATH[256];
    
    static std::string CUSTOMS_PLATFORM_LISTEN_PORT;
    static std::string SERIAL_SERVER_IP;
    static std::string GATHER_PORT;
    static std::string PASS_PORT;
    static std::string CENTER_CONTROLER_SERIAL;
    static std::string CENTER_LISTEN_PORT;
    static std::string DATABASE_SERVER_IP;
    static std::string DATABASE_SERVER_PORT;
    static std::string DATABASE_USER;
    static std::string DATABASE_PASSWORD;
    static std::string PIC_PATH;
    static std::string MAX_SIZE;
    static std::string CENTER_SERVER_IP;
    static std::string CENTER_SERVER_PORT;  
};

#endif

