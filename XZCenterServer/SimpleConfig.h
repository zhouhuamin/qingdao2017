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

 
    static int CUSTOMS_PLATFORM_LISTEN_PORT;

    static int CENTER_LISTEN_PORT;

    static char EVENT_SERVER_IP[32];
    static int  EVENT_SERVER_PORT;
   
    

    static char DATABASE_SERVER_IP[32];
    static char DATABASE_SERVER_PORT;
    static char DATABASE_USER[32];
    static char DATABASE_PASSWORD[32];

    static char PIC_SAVE_PATH[256];
    static int  PIC_MAX_SIZE;

    static char EXE_FULL_PATH[256];
    
    static std::map<string,T_PlatformInfo*>  m_StaPlatformMap;
};

#endif

