// main.cpp : Defines the entry point for the console application.
//	map local port to remote host port

#include	<pthread.h>
#include        <sys/ioctl.h>
#include	<sys/socket.h>
#include	<sys/types.h>
#include <sys/wait.h>
#include <linux/sockios.h>  
#include <linux/ethtool.h>
#include        <netinet/tcp.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include 	<arpa/inet.h>

#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>

#include        <unistd.h>
#include        <strings.h>
#include        <net/if.h>
#include	"sys/types.h"
#include	"string.h"
#include 	<sys/types.h>
#include 	<fcntl.h>
#include	"stdio.h"
#include	"stdlib.h"
#include <syslog.h>
#include <algorithm>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <boost/shared_array.hpp>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/std/utility.hpp>
#include <boost/shared_array.hpp>
#include <boost/timer.hpp>


#include "tinyxml/tinyxml.h"
#include "jzLocker.h"

using namespace std;
using namespace boost;

extern "C"
{
    typedef void*(*THREADFUNC)(void*);
}

typedef unsigned char BYTE;

#if	defined(_WIN32)||defined(_WIN64)	
CRITICAL_SECTION MyMutex;
#else
pthread_mutex_t MyMutex;
#endif
int GlobalStopFlag = 0;
#define	MAX_SOCKET	0x2000
//	max client per server
#define MAX_HOST_DATA 4096
int GlobalRemotePort = -1;

int     m_nETHStatus = 0;
locker	m_ETHLocker;

char GlobalRemoteHost[256];

struct JZMessage
{
    int nLen;
    int nType;
    int nCount;
    set<int> chnlSet;

    JZMessage() : nLen(0), nType(0), nCount(0), chnlSet()
    {
        ;
    }
};

typedef struct _SERVER_PARAM
{
    int nSocket;
    sockaddr_in ServerSockaddr;
    int nEndFlag; //	判断accept 线程是否结束
    int pClientSocket[MAX_SOCKET]; //	存放客户端连接的套接字
} SERVER_PARAM;

typedef struct _CLIENT_PARAM
{
    int nSocket;
    unsigned int nIndexOff;
    int nEndFlag; //	判断处理客户端连接的线程是否结束
    SERVER_PARAM * pServerParam;
    char szClientName[256];
} CLIENT_PARAM;

struct structManagerInfo
{
    int nStatus;
    std::string strSeqNo;
    std::string strRemoteIPHandle;
    std::string strRemotePortHandle;
    std::string strRemoteSynPortHandle;
    std::string strRemoteRole;
    structManagerInfo() : nStatus(0),strSeqNo(""),strRemoteIPHandle(""),strRemotePortHandle(""),strRemoteSynPortHandle(""),strRemoteRole("")
    {
        ;
    }
};

struct structProxyInfo
{
    std::string strProxyIPHandle;
    std::string strProxyPortHandle;  
    structProxyInfo():strProxyIPHandle(""),strProxyPortHandle("")
    {
        ;
    }
};


struct structConfigInfo
{
    locker      m_lock;
    std::string strRebootFlag;
    std::string strNetCardName;
    std::string strLocalIPHandle;
    std::vector<structProxyInfo>    proxyVect;
    std::vector<structManagerInfo>  managerVect;
    std::set<int>                   chnlSet;
    std::set<int>                   manChnlSet;

    structConfigInfo() : strRebootFlag("0"), strNetCardName(""), strLocalIPHandle(""), proxyVect(), managerVect(),chnlSet(),manChnlSet()
    {
        ;
    }
};

struct structChannelInfo
{
    int nChannelNo;
    std::string strAreaNo;
    std::string strChnlNo;
    std::string strIEType;

    structChannelInfo() : strAreaNo(""), strChnlNo(""), strIEType("")
    {
        ;
    }
};

struct structMonitorMap
{
    int nUpPort;
    int nDownPort;

    structMonitorMap() : nUpPort(0), nDownPort(0)
    {
        ;
    }
};

struct structChannelVector
{
    std::vector<structChannelInfo> chnnlVect;

    structChannelVector() : chnnlVect()
    {
        ;
    }
};


struct structHostInfo
{
    std::string strHostName;
    std::set<int>   chnlSet;
    structHostInfo():strHostName(""),chnlSet()
    {
        ;
    }    
};

struct structSocketInfo
{
    int             nSocket;
    time_t          nSendTime;
    time_t          nRecvTime;
    pthread_t       threadId;
    
    structSocketInfo() : nSocket(0), nSendTime(0), nRecvTime(0), threadId(0)
    {
        ;
    }
};


struct structWorkerChannelInfo
{
    int nSocket;
    time_t nTime;
    std::set<int> chnlSet;

    structWorkerChannelInfo() : nSocket(0), nTime(0), chnlSet()
    {
        ;
    }
};

struct structCenterProxyServerInfo
{
    std::string strServerIP;
    int nListenPort;

    structCenterProxyServerInfo() : strServerIP(""), nListenPort(0)
    {
        ;
    }
};


//======================================平台代理交互报文结构定义=========
#define SYS_NET_MSGHEAD                          "0XJZTECH"
#define SYS_NET_MSGTAIL                           "NJTECHJZ"

#define SYS_MSG_PUBLISH_CHANNELSERVER       0X1001
#define MAX_MSG_BODYLEN  20*1024

struct NET_PACKET_HEAD
{
    int msg_type;
    int packet_len;
    char version_no[16];
    int proxy_count;
    int net_proxy[10];
};

struct NET_PACKET_MSG
{
    NET_PACKET_HEAD msg_head;
    char msg_body[MAX_MSG_BODYLEN];
};

struct T_ChnanelServer
{
    char szAreaID[16];
    char szChannelNo[16];
    char szIEType[8];
    char szCenterControlIP[32];
    int nCenterControlPort;
    int nCenterControlAckPort;
};

struct T_ChnanelServerInfo
{
    int channel_no;
    T_ChnanelServer channelInfo[];
};

//======================================平台代理交互报文结构定义结束=========

structConfigInfo g_configInfo;
structMonitorMap g_monitorInfo;
structChannelVector g_chnlVect;

locker g_sockChannelLock;
locker g_socketInfoLock;
std::map<int, structWorkerChannelInfo> g_sockChannelMap;

std::map<int, structCenterProxyServerInfo> g_centerProxyServerMap;

std::string g_strConfigFileName = "";
std::string g_strLastMasterIP   = "";


structSocketInfo                g_socketInfo;
std::set<int>                   g_chnlSet;
locker g_chnlSetLock;

int get_gw_ip(const char *eth, char *ipaddr)
{
    int sock_fd;
    struct  sockaddr_in my_addr;
    struct ifreq ifr;

    /**//* Get socket file descriptor */
    if ((sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
       syslog(LOG_DEBUG, "socket");
       return -1;
    }

    /**//* Get IP Address */
    int nLen = strlen(eth);
    strncpy(ifr.ifr_name, eth, nLen);
    ifr.ifr_name[nLen]='\0';

    if (ioctl(sock_fd, SIOCGIFADDR, &ifr) < 0)
    {
       syslog(LOG_DEBUG, ":No Such Device %s/n",eth);
       return -1;
    }

    memcpy(&my_addr, &ifr.ifr_addr, sizeof(my_addr));
    strcpy(ipaddr, inet_ntoa(my_addr.sin_addr));
    close(sock_fd);
    return 0;
}


int GetProcessPath(char* cpPath)
{
    int iCount;
    iCount = readlink("/proc/self/exe", cpPath, 256);

    if (iCount < 0 || iCount >= 256)
    {
        syslog(LOG_DEBUG, "********get process absolute path failed,errno:%d !\n", errno);
        return -1;
    }

    cpPath[iCount] = '\0';
    return 0;
}


int set_keep_live(int nSocket, int keep_alive_times, int keep_alive_interval)
{

#ifdef WIN32                                //WIndows下
	TCP_KEEPALIVE inKeepAlive = {0}; //输入参数
	unsigned long ulInLen = sizeof (TCP_KEEPALIVE);

	TCP_KEEPALIVE outKeepAlive = {0}; //输出参数
	unsigned long ulOutLen = sizeof (TCP_KEEPALIVE);

	unsigned long ulBytesReturn = 0;

	//设置socket的keep alive为5秒，并且发送次数为3次
	inKeepAlive.on_off = keep_alive_times;
	inKeepAlive.keep_alive_interval = keep_alive_interval * 1000; //两次KeepAlive探测间的时间间隔
	inKeepAlive.keep_alive_time = keep_alive_interval * 1000; //开始首次KeepAlive探测前的TCP空闭时间


	outKeepAlive.on_off = keep_alive_times;
	outKeepAlive.keep_alive_interval = keep_alive_interval * 1000; //两次KeepAlive探测间的时间间隔
	outKeepAlive.keep_alive_time = keep_alive_interval * 1000; //开始首次KeepAlive探测前的TCP空闭时间


	if (WSAIoctl((unsigned int) nSocket, SIO_KEEPALIVE_VALS,
		(LPVOID) & inKeepAlive, ulInLen,
		(LPVOID) & outKeepAlive, ulOutLen,
		&ulBytesReturn, NULL, NULL) == SOCKET_ERROR)
	{
		//ACE_DEBUG((LM_INFO,
		//	ACE_TEXT("(%P|%t) WSAIoctl failed. error code(%d)!\n"), WSAGetLastError()));
	}

#else                                        //linux下
	int keepAlive = 1; //设定KeepAlive
	int keepIdle = keep_alive_interval; //开始首次KeepAlive探测前的TCP空闭时间
	int keepInterval = keep_alive_interval; //两次KeepAlive探测间的时间间隔
	int keepCount = keep_alive_times; //判定断开前的KeepAlive探测次数
	if (setsockopt(nSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*) & keepAlive, sizeof (keepAlive)) == -1)
	{
		syslog(LOG_DEBUG, "setsockopt SO_KEEPALIVE error!\n");
	}
	if (setsockopt(nSocket, SOL_TCP, TCP_KEEPIDLE, (const char *) & keepIdle, sizeof (keepIdle)) == -1)
	{
		syslog(LOG_DEBUG, "setsockopt TCP_KEEPIDLE error!\n");
	}
	if (setsockopt(nSocket, SOL_TCP, TCP_KEEPINTVL, (const char *) & keepInterval, sizeof (keepInterval)) == -1)
	{
		syslog(LOG_DEBUG, "setsockopt TCP_KEEPINTVL error!\n");
	}
	if (setsockopt(nSocket, SOL_TCP, TCP_KEEPCNT, (const char *) & keepCount, sizeof (keepCount)) == -1)
	{
		syslog(LOG_DEBUG, "setsockopt TCP_KEEPCNT error!\n");
	}

#endif

	return 0;

}


void ReadConfigInfo(struct structConfigInfo &configInfo)
{
    char exeFullPath[256] = {0};
    char EXE_FULL_PATH[256] = {0};

    GetProcessPath(exeFullPath); //得到程序模块名称，全路径

    string strFullExeName(exeFullPath);
    int nLast = strFullExeName.find_last_of("/");
    strFullExeName = strFullExeName.substr(0, nLast + 1);
    strcpy(EXE_FULL_PATH, strFullExeName.c_str());

    char cpPath[256]    = {0};
    char temp[256]      = {0};
    sprintf(cpPath, "%sJZWorkerConfig.xml", strFullExeName.c_str());

    g_strConfigFileName = cpPath;
    TiXmlDocument doc(cpPath);
    doc.LoadFile();

    TiXmlHandle docHandle(&doc);
    TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("ConfigInfo");
    TiXmlHandle RebootFlagHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("RebootFlag", 0).FirstChild();
    //TiXmlHandle TryTimesHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("TryTimes", 0).FirstChild();
    //TiXmlHandle LocalIPHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("LocalIP", 0).FirstChild();
    TiXmlHandle NetCardNameHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("NetCardName", 0).FirstChild();
    
//    TiXmlHandle RemoteIP1Handle     = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemoteIP1", 0).FirstChild();
//    TiXmlHandle RemotePort1Handle   = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemotePort1", 0).FirstChild();
//    TiXmlHandle RemoteSynPort1Handle   = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemoteSynPort1", 0).FirstChild();
//    TiXmlHandle Remote1RoleHandle       = docHandle.FirstChildElement("ConfigInfo").ChildElement("Remote1Role", 0).FirstChild();
//    
//    
//    
//    
//    TiXmlHandle RemoteIP2Handle      = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemoteIP2", 0).FirstChild();
//    TiXmlHandle RemotePort2Handle    = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemotePort2", 0).FirstChild();  
//    TiXmlHandle RemoteSynPort2Handle = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemoteSynPort2", 0).FirstChild();
//    TiXmlHandle Remote2RoleHandle       = docHandle.FirstChildElement("ConfigInfo").ChildElement("Remote2Role", 0).FirstChild();
//    
//    
//    
//    TiXmlHandle RemoteIP3Handle     = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemoteIP3", 0).FirstChild();
//    TiXmlHandle RemotePort3Handle   = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemotePort3", 0).FirstChild(); 
//    TiXmlHandle RemoteSynPort3Handle = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemoteSynPort3", 0).FirstChild();
//    TiXmlHandle Remote3RoleHandle       = docHandle.FirstChildElement("ConfigInfo").ChildElement("Remote3Role", 0).FirstChild();
//    
//    
//    TiXmlHandle RemoteIP4Handle     = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemoteIP4", 0).FirstChild();
//    TiXmlHandle RemotePort4Handle   = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemotePort4", 0).FirstChild();
//    TiXmlHandle RemoteSynPort4Handle = docHandle.FirstChildElement("ConfigInfo").ChildElement("RemoteSynPort4", 0).FirstChild();    
//    TiXmlHandle Remote4RoleHandle       = docHandle.FirstChildElement("ConfigInfo").ChildElement("Remote4Role", 0).FirstChild();
    
    
    TiXmlHandle ProxyIP1Handle = docHandle.FirstChildElement("ConfigInfo").ChildElement("ProxyIP1", 0).FirstChild();
    TiXmlHandle ProxyPort1Handle = docHandle.FirstChildElement("ConfigInfo").ChildElement("ProxyPort1", 0).FirstChild();
    
    TiXmlHandle ProxyIP2Handle = docHandle.FirstChildElement("ConfigInfo").ChildElement("ProxyIP2", 0).FirstChild();
    TiXmlHandle ProxyPort2Handle = docHandle.FirstChildElement("ConfigInfo").ChildElement("ProxyPort2", 0).FirstChild();

    std::string strTryTimes = "0";
    string strLocalIPHandle = "";
    //string strRemoteIPHandle = "";
    //string strRemotePortHandle = "";
    //string strProxyIP2      = "";
    //string strProxyPort2    = "";
    //structManagerInfo   tmpManger;
    structProxyInfo     tmpProxy1;
    structProxyInfo     tmpProxy2;
     
    
    if (RebootFlagHandle.Node() != NULL)
        configInfo.strRebootFlag      = RebootFlagHandle.Text()->Value();  

//    if (TryTimesHandle.Node() != NULL)
//        configInfo.strTryTimes      = TryTimesHandle.Text()->Value();    
    
//    if (LocalIPHandle.Node() != NULL)
//        configInfo.strLocalIPHandle = strLocalIPHandle = LocalIPHandle.Text()->Value();
    
    if (NetCardNameHandle.Node() != NULL)
    {
        configInfo.strNetCardName = NetCardNameHandle.Text()->Value();    
        char szLocalIP[50] = {0};
        std::string strDeviceName = configInfo.strNetCardName;
        get_gw_ip(strDeviceName.c_str(), szLocalIP);
        if (szLocalIP[0] == '\0')
            return;
        configInfo.strLocalIPHandle = szLocalIP;
    }

    
//    tmpManger.nStatus   = 1;
//    if (RemoteIP1Handle.Node() != NULL)
//        tmpManger.strRemoteIPHandle = RemoteIP1Handle.Text()->Value();
//    if (RemotePort1Handle.Node() != NULL)
//        tmpManger.strRemotePortHandle = RemotePort1Handle.Text()->Value();
//    if (RemoteSynPort1Handle.Node() != NULL)
//        tmpManger.strRemoteSynPortHandle = RemoteSynPort1Handle.Text()->Value();
//    if (Remote1RoleHandle.Node() != NULL)
//        tmpManger.strRemoteRole = Remote1RoleHandle.Text()->Value();    
//    
//    
//    
//    configInfo.managerVect.push_back(tmpManger);
//    
//    
//    tmpManger.nStatus   = 0;
//    if (RemoteIP2Handle.Node() != NULL)
//        tmpManger.strRemoteIPHandle = RemoteIP2Handle.Text()->Value();
//    if (RemotePort2Handle.Node() != NULL)
//        tmpManger.strRemotePortHandle = RemotePort2Handle.Text()->Value();
//    if (RemoteSynPort2Handle.Node() != NULL)
//        tmpManger.strRemoteSynPortHandle = RemoteSynPort2Handle.Text()->Value();    
//    if (Remote2RoleHandle.Node() != NULL)
//        tmpManger.strRemoteRole = Remote2RoleHandle.Text()->Value();   
//    configInfo.managerVect.push_back(tmpManger);    
//  
//    tmpManger.nStatus   = 0;
//    if (RemoteIP3Handle.Node() != NULL)
//        tmpManger.strRemoteIPHandle = RemoteIP3Handle.Text()->Value();
//    if (RemotePort3Handle.Node() != NULL)
//        tmpManger.strRemotePortHandle = RemotePort3Handle.Text()->Value();
//    if (RemoteSynPort3Handle.Node() != NULL)
//        tmpManger.strRemoteSynPortHandle = RemoteSynPort3Handle.Text()->Value();    
//    if (Remote3RoleHandle.Node() != NULL)
//        tmpManger.strRemoteRole = Remote3RoleHandle.Text()->Value();   
//    configInfo.managerVect.push_back(tmpManger);    
//    
//    tmpManger.nStatus   = 0;
//    if (RemoteIP4Handle.Node() != NULL)
//        tmpManger.strRemoteIPHandle = RemoteIP4Handle.Text()->Value();
//    if (RemotePort4Handle.Node() != NULL)
//        tmpManger.strRemotePortHandle = RemotePort4Handle.Text()->Value();
//    if (RemoteSynPort4Handle.Node() != NULL)
//        tmpManger.strRemoteSynPortHandle = RemoteSynPort4Handle.Text()->Value();   
//    if (Remote4RoleHandle.Node() != NULL)
//        tmpManger.strRemoteRole = Remote4RoleHandle.Text()->Value();   
//    configInfo.managerVect.push_back(tmpManger);    
    
    
    
    
    
    
    if (ProxyIP1Handle.Node() != NULL)
        tmpProxy1.strProxyIPHandle = ProxyIP1Handle.Text()->Value();
    if (ProxyPort1Handle.Node() != NULL)
        tmpProxy1.strProxyPortHandle = ProxyPort1Handle.Text()->Value();
    configInfo.proxyVect.push_back(tmpProxy1);

    if (ProxyIP2Handle.Node() != NULL)
        tmpProxy2.strProxyIPHandle   = ProxyIP2Handle.Text()->Value();
    if (ProxyPort2Handle.Node() != NULL)
        tmpProxy2.strProxyPortHandle = ProxyPort2Handle.Text()->Value();    
    configInfo.proxyVect.push_back(tmpProxy2);
    
    
    structCenterProxyServerInfo tmpServerInfo1;
    tmpServerInfo1.nListenPort   = atoi(tmpProxy1.strProxyPortHandle.c_str());
    tmpServerInfo1.strServerIP   = tmpProxy1.strProxyIPHandle;    
    g_centerProxyServerMap.insert(make_pair(0, tmpServerInfo1));
    
    
    structCenterProxyServerInfo tmpServerInfo2;
    tmpServerInfo2.nListenPort   = atoi(tmpProxy2.strProxyPortHandle.c_str());
    tmpServerInfo2.strServerIP   = tmpProxy2.strProxyIPHandle;   
    if (tmpServerInfo2.nListenPort <= 0 || tmpServerInfo2.strServerIP.empty())
    {
        ;
    }
    else
    {
        g_centerProxyServerMap.insert(make_pair(1, tmpServerInfo2));
    }
    
    syslog(LOG_DEBUG, "%s", strLocalIPHandle.c_str());

    TiXmlElement *xmlRootElement = NULL;
    TiXmlElement *xmlSubElement = NULL;
    TiXmlElement* xmlListElement = NULL;
    TiXmlNode *pNode = NULL;
    TiXmlNode *pNodeTmp = NULL;
    TiXmlNode *pConfigInfoNode = NULL;

    pConfigInfoNode = doc.FirstChild("ConfigInfo");
    if (pConfigInfoNode == NULL)
    {
        syslog(LOG_DEBUG, "pConfigNode:%p\n", pConfigInfoNode);
        return;
    }
    
    // xxxxx
    // 2016-6-15 add man
    xmlRootElement = pConfigInfoNode->ToElement();
    if (xmlRootElement != NULL && (pNode = xmlRootElement->FirstChild("ServiceList")) != NULL)
    {
            xmlListElement = pNode->ToElement();

            for (pNode = xmlListElement->FirstChild("Service"); pNode != NULL; pNode = pNode->NextSibling("Service"))
            {
                    string SeqNo        = "";
                    string IP		= "";
                    string Port         = "";
                    string Port2	= "";
                    string Role		= "";
                    

                    xmlSubElement = pNode->ToElement() ;
                    
                    if (xmlSubElement != NULL)
                    {
                        if (xmlSubElement->Attribute("SeqNo") != NULL)
                        {
                            SeqNo = xmlSubElement->Attribute("SeqNo");
                        }
                    }
                    
                    if (xmlSubElement != NULL)
                    {
                        if (xmlSubElement->Attribute("IP") != NULL)
                        {
                            IP = xmlSubElement->Attribute("IP");
                        }
                    }
                    
                    if (xmlSubElement != NULL)
                    {
                        if (xmlSubElement->Attribute("Port") != NULL)
                        {
                            Port = xmlSubElement->Attribute("Port");
                        }
                    }                    
                    
                    if (xmlSubElement != NULL)
                    {
                        if (xmlSubElement->Attribute("Port2") != NULL)
                        {
                            Port2 = xmlSubElement->Attribute("Port2");
                        }
                    }   
                    
                    
                    if (xmlSubElement != NULL)
                    {
                        if (xmlSubElement->Attribute("Role") != NULL)
                        {
                            Role = xmlSubElement->Attribute("Role");
                        }
                    }                      
                    
                    
//                    if (xmlSubElement != NULL)
//                            pNodeTmp = xmlSubElement->FirstChildElement("IP");
//
//                    if (pNodeTmp != NULL && pNodeTmp->ToElement() != NULL && pNodeTmp->ToElement()->GetText() != 0)
//                            IP = pNodeTmp->ToElement()->GetText();
//
//                    if (xmlSubElement != NULL)
//                            pNodeTmp = xmlSubElement->FirstChildElement("Port");
//                    if (pNodeTmp != NULL && pNodeTmp->ToElement() != NULL && pNodeTmp->ToElement()->GetText() != 0)
//                            Port = pNodeTmp->ToElement()->GetText();
//
//                    if (xmlSubElement != NULL)
//                            pNodeTmp = xmlSubElement->FirstChildElement("Port2");
//                    if (pNodeTmp != NULL && pNodeTmp->ToElement() != NULL && pNodeTmp->ToElement()->GetText() != 0)
//                            Port2 = pNodeTmp->ToElement()->GetText();
//
//                    if (xmlSubElement != NULL)
//                            pNodeTmp = xmlSubElement->FirstChildElement("Role");
//                    if (pNodeTmp != NULL && pNodeTmp->ToElement() != NULL && pNodeTmp->ToElement()->GetText() != 0)
//                            Role = pNodeTmp->ToElement()->GetText();

                    structManagerInfo   tmpManger;
                    
                    tmpManger.nStatus                   = 0;
                    tmpManger.strSeqNo                  = SeqNo;
                    tmpManger.strRemoteIPHandle         = IP;
                    tmpManger.strRemotePortHandle       = Port;
                    tmpManger.strRemoteSynPortHandle    = Port2;    
                    tmpManger.strRemoteRole             = Role;   
                    configInfo.managerVect.push_back(tmpManger);  
            }
    }   
    
    // xxxxlejlejre
    
    
    

    xmlRootElement = pConfigInfoNode->ToElement();
    if (xmlRootElement)
    {
        TiXmlNode *pConfigNode = NULL;
        TiXmlNode *pChnlNoNode = NULL;
        pConfigNode = xmlRootElement->FirstChild("ChannelConfig");
        if (pConfigNode != NULL)
        {
            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
            {
                string strChnlNo = "";
                xmlSubElement = pChnlNoNode->ToElement();

                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
                {
                    strChnlNo = pChnlNoNode->ToElement()->GetText();
                    int nChnlNo = atoi(strChnlNo.c_str());
                    configInfo.chnlSet.insert(nChnlNo);
                    syslog(LOG_DEBUG, "ChannelNo:%s", strChnlNo.c_str());
                }

                syslog(LOG_DEBUG, "\n");
            }
        }
    }
    
    
    // 2016-6-15 add man
    xmlRootElement = pConfigInfoNode->ToElement();
    if (xmlRootElement)
    {
        TiXmlNode *pConfigNode = NULL;
        TiXmlNode *pChnlNoNode = NULL;
        pConfigNode = xmlRootElement->FirstChild("ManChannelConfig");
        if (pConfigNode != NULL)
        {
            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
            {
                string strChnlNo = "";
                xmlSubElement = pChnlNoNode->ToElement();

                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
                {
                    strChnlNo = pChnlNoNode->ToElement()->GetText();
                    int nChnlNo = atoi(strChnlNo.c_str());
                    configInfo.manChnlSet.insert(nChnlNo);
                    syslog(LOG_DEBUG, "ManChannelNo:%s", strChnlNo.c_str());
                }

                syslog(LOG_DEBUG, "\n");
            }
        }
    }    
    
    
    // end
    
    if (g_configInfo.strRebootFlag == "0")
    {
        g_chnlSetLock.lock();
        g_chnlSet = configInfo.chnlSet; 
        g_chnlSetLock.unlock();
    }
}

void ReadChnlConfigInfo(int nChnlNo, const std::string &strFileName)
{
    char cpPath[256] = {0};
    sprintf(cpPath, "%s", strFileName.c_str());

    TiXmlDocument doc(cpPath);
    doc.LoadFile();

    TiXmlHandle docHandle(&doc);
    TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("Config");
    TiXmlHandle GATHER_AREA_IDHandle = docHandle.FirstChildElement("Config").ChildElement("GATHER_AREA_ID", 0).FirstChild();
    TiXmlHandle GATHER_CHANNE_NOHandle = docHandle.FirstChildElement("Config").ChildElement("GATHER_CHANNE_NO", 0).FirstChild();
    TiXmlHandle GATHER_I_E_TYPEHandle = docHandle.FirstChildElement("Config").ChildElement("GATHER_I_E_TYPE", 0).FirstChild();

    structChannelInfo tmpChnlInfo;
    tmpChnlInfo.nChannelNo = nChnlNo;

    if (GATHER_AREA_IDHandle.Node() != NULL)
        tmpChnlInfo.strAreaNo = GATHER_AREA_IDHandle.Text()->Value();

    if (GATHER_CHANNE_NOHandle.Node() != NULL)
        tmpChnlInfo.strChnlNo = GATHER_CHANNE_NOHandle.Text()->Value();
    
    if (GATHER_I_E_TYPEHandle.Node() != NULL)
        tmpChnlInfo.strIEType = GATHER_I_E_TYPEHandle.Text()->Value();

    g_chnlVect.chnnlVect.push_back(tmpChnlInfo);
    syslog(LOG_DEBUG, "%s,%s", tmpChnlInfo.strAreaNo.c_str(), tmpChnlInfo.strChnlNo.c_str());
}

void ReadMonitorConfigInfo(const std::string &strFileName)
{
    char cpPath[256] = {0};
    sprintf(cpPath, "%s", strFileName.c_str());

    TiXmlDocument doc(cpPath);
    doc.LoadFile();

    TiXmlHandle docHandle(&doc);
    TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("Config");
    TiXmlHandle CENTER_LISTEN_PORTHandle = docHandle.FirstChildElement("Config").ChildElement("CENTER_LISTEN_PORT", 0).FirstChild();
    TiXmlHandle CUSTOMS_PLATFORM_LISTEN_PORTHandle = docHandle.FirstChildElement("Config").ChildElement("CUSTOMS_PLATFORM_LISTEN_PORT", 0).FirstChild();

    std::string strUpPort = "";
    std::string strDownPort = "";

    if (CENTER_LISTEN_PORTHandle.Node() != NULL)
    {
        strUpPort = CENTER_LISTEN_PORTHandle.Text()->Value();
        g_monitorInfo.nUpPort = atoi(strUpPort.c_str());
    }

    if (CUSTOMS_PLATFORM_LISTEN_PORTHandle.Node() != NULL)
    {
        strDownPort = CUSTOMS_PLATFORM_LISTEN_PORTHandle.Text()->Value();
        g_monitorInfo.nDownPort = atoi(strDownPort.c_str());
    }


    syslog(LOG_DEBUG, "%s,%s", strUpPort.c_str(), strDownPort.c_str());
}

void WriteConfigFile(const set<int> &paramSet)
{
    // 2016-8-30
    if (paramSet.size() <= 0)
        return;
    
    
    using boost::property_tree::ptree;
    ptree pt;


    std::string strRebootFlag = "1";
    //pt.add_child("ConfigInfo", pattr1);
    pt.put("ConfigInfo.RebootFlag",     strRebootFlag);
    //pt.put("ConfigInfo.TryTimes",       g_configInfo.strTryTimes);
    //pt.put("ConfigInfo.LocalIP",        g_configInfo.strLocalIPHandle);
    pt.put("ConfigInfo.NetCardName",        g_configInfo.strNetCardName);
    
    for (int i = 0; i < g_configInfo.managerVect.size(); ++i)
    {
        ptree pattr1;
        pattr1.add<std::string>("<xmlattr>.SeqNo", g_configInfo.managerVect[i].strSeqNo);  
        
        pattr1.add<std::string>("<xmlattr>.IP", g_configInfo.managerVect[i].strRemoteIPHandle);  

        pattr1.add<std::string>("<xmlattr>.Port", g_configInfo.managerVect[i].strRemotePortHandle);  
        
        pattr1.add<std::string>("<xmlattr>.Port2", g_configInfo.managerVect[i].strRemoteSynPortHandle);          
        
        pattr1.add<std::string>("<xmlattr>.Role", g_configInfo.managerVect[i].strRemoteRole);  
        pt.add_child("ConfigInfo.ServiceList.Service", pattr1);           
//        pt.add("ConfigInfo.ServiceList.Service.IP",     g_configInfo.managerVect[i].strRemoteIPHandle);
//        pt.add("ConfigInfo.ServiceList.Service.Port",   g_configInfo.managerVect[i].strRemotePortHandle);
//        pt.add("ConfigInfo.ServiceList.Service.Port2",  g_configInfo.managerVect[i].strRemoteSynPortHandle);
//        pt.add("ConfigInfo.ServiceList.Service.Role",   g_configInfo.managerVect[i].strRemoteRole);       
    }
    
    
//    pt.put("ConfigInfo.RemoteIP1",   g_configInfo.managerVect[0].strRemoteIPHandle);
//    pt.put("ConfigInfo.RemotePort1", g_configInfo.managerVect[0].strRemotePortHandle);
//    pt.put("ConfigInfo.RemoteSynPort1", g_configInfo.managerVect[0].strRemoteSynPortHandle);
//    pt.put("ConfigInfo.Remote1Role", g_configInfo.managerVect[0].strRemoteRole);
//    
//    pt.put("ConfigInfo.RemoteIP2",   g_configInfo.managerVect[1].strRemoteIPHandle);
//    pt.put("ConfigInfo.RemotePort2", g_configInfo.managerVect[1].strRemotePortHandle);  
//    pt.put("ConfigInfo.RemoteSynPort2", g_configInfo.managerVect[1].strRemoteSynPortHandle);
//    pt.put("ConfigInfo.Remote2Role", g_configInfo.managerVect[1].strRemoteRole);
//    
//    pt.put("ConfigInfo.RemoteIP3",   g_configInfo.managerVect[2].strRemoteIPHandle);
//    pt.put("ConfigInfo.RemotePort3", g_configInfo.managerVect[2].strRemotePortHandle);    
//    pt.put("ConfigInfo.RemoteSynPort3", g_configInfo.managerVect[2].strRemoteSynPortHandle);
//    pt.put("ConfigInfo.Remote3Role", g_configInfo.managerVect[2].strRemoteRole);
//    
//    pt.put("ConfigInfo.RemoteIP4",   g_configInfo.managerVect[3].strRemoteIPHandle);
//    pt.put("ConfigInfo.RemotePort4", g_configInfo.managerVect[3].strRemotePortHandle);   
//    pt.put("ConfigInfo.RemoteSynPort4", g_configInfo.managerVect[3].strRemoteSynPortHandle);
//    pt.put("ConfigInfo.Remote4Role", g_configInfo.managerVect[3].strRemoteRole);
    
    pt.put("ConfigInfo.ProxyIP1",   g_centerProxyServerMap[0].strServerIP);
    pt.put("ConfigInfo.ProxyPort1", g_centerProxyServerMap[0].nListenPort);
    pt.put("ConfigInfo.ProxyIP2",   g_centerProxyServerMap[1].strServerIP);
    pt.put("ConfigInfo.ProxyPort2", g_centerProxyServerMap[0].nListenPort);
    
    set<int>::iterator it;
    for (it = paramSet.begin(); it != paramSet.end(); ++it)
    {
        pt.add("ConfigInfo.ChannelConfig.ChannelNo", *it);
    }
    
    for (it = g_configInfo.manChnlSet.begin(); it != g_configInfo.manChnlSet.end(); ++it)
    {
        pt.add("ConfigInfo.ManChannelConfig.ChannelNo", *it);
    }


    // 格式化输出，指定编码（默认utf-8）
    boost::property_tree::xml_writer_settings<char> settings('\t', 1, "GB2312");

    std::string filename = g_strConfigFileName;
    write_xml(filename, pt, std::locale(), settings);    
}


void UninitWinSock()
{
#if	defined(_WIN32)||defined(_WIN64)	
    WSACleanup();
#endif
}

void MyInitLock()
{
#if	defined(_WIN32)||defined(_WIN64)
    InitializeCriticalSection(&MyMutex);
#else
    pthread_mutex_init(&MyMutex, NULL);
#endif
}

void MyUninitLock()
{
#if	defined(_WIN32)||defined(_WIN64)
    DeleteCriticalSection(&MyMutex);
#else
    pthread_mutex_destroy(&MyMutex);
#endif
}

void MyLock()
{
#if	defined(_WIN32)||defined(_WIN64)
    EnterCriticalSection(&MyMutex);
#else
    pthread_mutex_lock(&MyMutex);
#endif
}

void MyUnlock()
{
#if	defined(_WIN32)||defined(_WIN64)
    LeaveCriticalSection(&MyMutex);
#else
    pthread_mutex_unlock(&MyMutex);
#endif
}


int ReceiveTimer(int fd)
{
    int iRet = 0;
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tv.tv_sec = 5;
    tv.tv_usec = 500 * 1000;

    iRet = select(fd + 1, &rset, NULL, NULL, &tv);
    return iRet;
}




void PrintUsage(const char * szAppName)
{
    syslog(LOG_DEBUG, "Usage : %s <LocalPort> <RemoteHostName> <RemotePort> \n", szAppName);
}

int CreateSocket()
{
    int nSocket;
    nSocket = (int) socket(PF_INET, SOCK_STREAM, 0);
    return nSocket;
}

int CheckSocketResult(int nResult)
{
    //	check result;
#if !defined(_WIN32)&&!defined(_WIN64)
    if (nResult == -1)
        return 0;
    else
        return 1;
#else
    if (nResult == SOCKET_ERROR)
        return 0;
    else
        return 1;
#endif
}

int CheckSocketValid(int nSocket)
{
    //	check socket valid

#if !defined(_WIN32)&&!defined(_WIN64)
    if (nSocket == -1)
        return 0;
    else
        return 1;
#else
    if (((SOCKET) nSocket) == INVALID_SOCKET)
        return 0;
    else
        return 1;
#endif
}

int BindPort(int nSocket, int nPort)
{
    int rc;
    int optval = 1;
#if	defined(_WIN32)||defined(_WIN64)
    rc = setsockopt((SOCKET) nSocket, SOL_SOCKET, SO_REUSEADDR,
            (const char *) &optval, sizeof (int));
#else
    rc = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR,
            (const char *) &optval, sizeof (int));
#endif
    if (!CheckSocketResult(rc))
        return 0;
    sockaddr_in name;
    memset(&name, 0, sizeof (sockaddr_in));
    name.sin_family = AF_INET;
    name.sin_port = htons((unsigned short) nPort);
    name.sin_addr.s_addr = INADDR_ANY;
#if	defined(_WIN32)||defined(_WIN64)
    rc = ::bind((SOCKET) nSocket, (sockaddr *) & name, sizeof (sockaddr_in));
#else
    rc = ::bind(nSocket, (sockaddr *) & name, sizeof (sockaddr_in));
#endif
    if (!CheckSocketResult(rc))
        return 0;
    return 1;
}

int ConnectSocket(int nSocket, const char * szHost, int nPort)
{
    hostent *pHost = NULL;
    hostent localHost;
    char pHostData[MAX_HOST_DATA];
    int h_errorno = 0;
    int h_rc = gethostbyname_r(szHost, &localHost, pHostData, MAX_HOST_DATA, &pHost, &h_errorno);
    if ((pHost == 0) || (h_rc != 0))
    {
        return 0;
    }

    struct in_addr in;
    memcpy(&in.s_addr, pHost->h_addr_list[0], sizeof (in.s_addr));
    sockaddr_in name;
    memset(&name, 0, sizeof (sockaddr_in));
    name.sin_family = AF_INET;
    name.sin_port = htons((unsigned short) nPort);
    name.sin_addr.s_addr = in.s_addr;
#if defined(_WIN32)||defined(_WIN64)
    int rc = connect((SOCKET) nSocket, (sockaddr *) & name, sizeof (sockaddr_in));
#else
    int rc = connect(nSocket, (sockaddr *) & name, sizeof (sockaddr_in));
#endif
    if (rc >= 0)
        return 1;
    return 0;
}

int CloseSocket(int nSocket)
{
    int rc = 0;
    if (!CheckSocketValid(nSocket))
    {
        return rc;
    }
#if	defined(_WIN32)||defined(_WIN64)
    shutdown((SOCKET) nSocket, SD_BOTH);
    closesocket((SOCKET) nSocket);
#else
    shutdown(nSocket, SHUT_RDWR);
    close(nSocket);
#endif
    rc = 1;
    return rc;
}

int ListenSocket(int nSocket, int nMaxQueue)
{
    int rc = 0;
#if	defined(_WIN32)||defined(_WIN64)
    rc = listen((SOCKET) nSocket, nMaxQueue);
#else
    rc = listen(nSocket, nMaxQueue);
#endif
    return CheckSocketResult(rc);
}

void SetSocketNotBlock(int nSocket)
{
    //	改变文件句柄为非阻塞模式
#if	defined(_WIN32)||defined(_WIN64)
    ULONG optval2 = 1;
    ioctlsocket((SOCKET) nSocket, FIONBIO, &optval2);
#else
    long fileattr;
    fileattr = fcntl(nSocket, F_GETFL);
    fcntl(nSocket, F_SETFL, fileattr | O_NDELAY);
#endif
}

void SysSleep(long nTime)
//	延时nTime毫秒，毫秒是千分之一秒
{
#if defined(_WIN32 )||defined(_WIN64)
    //	windows 代码
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
        if (GetMessage(&msg, NULL, 0, 0) != -1)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    Sleep(nTime);
#else
    //	unix/linux代码
    timespec localTimeSpec;
    timespec localLeftSpec;
    localTimeSpec.tv_sec = nTime / 1000;
    localTimeSpec.tv_nsec = (nTime % 1000)*1000000;
    nanosleep(&localTimeSpec, &localLeftSpec);
#endif
}

int CheckSocketError(int nResult)
{
    //	检查非阻塞套接字错误
    if (nResult > 0)
        return 0;
    if (nResult == 0)
        return 1;

#if !defined(_WIN32)&&!defined(_WIN64)
    if (errno == EAGAIN)
        return 0;
    return 1;
#else
    if (WSAGetLastError() == WSAEWOULDBLOCK)
        return 0;
    else
        return 1;
#endif
}

int SocketWrite(int nSocket, char * pBuffer, int nLen, int nTimeout)
{
    int nOffset = 0;
    int nWrite;
    int nLeft = nLen;
    int nLoop = 0;
    int nTotal = 0;
    int nNewTimeout = nTimeout * 10;
    while ((nLoop <= nNewTimeout)&&(nLeft > 0))
    {
        nWrite = send(nSocket, pBuffer + nOffset, nLeft, 0);
        if (nWrite == 0)
        {
            return -1;
        }
#if defined(_WIN32)||defined(_WIN64)
        if (nWrite == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                return -1;
            }
        }
#else
        if (nWrite == -1)
        {
            if (errno != EAGAIN)
            {
                return -1;
            }
        }
#endif
        if (nWrite < 0)
        {
            return nWrite;
        }
        nOffset += nWrite;
        nLeft -= nWrite;
        nTotal += nWrite;
        if (nLeft > 0)
        {
            //	延时100ms
            SysSleep(100);
        }
        nLoop++;
    }
    return nTotal;
}

int SocketRead(int nSocket, void * pBuffer, int nLen)
{
    if (nSocket == -1)
        return -1;
    int len = 0;
#if	defined(_WIN32)||defined(_WIN64)
    len = recv((SOCKET) nSocket, (char *) pBuffer, nLen, 0);
#else
    len = recv(nSocket, (char *) pBuffer, nLen, 0);
#endif

    if (len == 0)
    {
        return -1;
    }
    if (len == -1)
    {
//#if	defined(_WIN32)||defined(_WIn64)
//        int localError = WSAGetLastError();
//        if (localError == WSAEWOULDBLOCK)
//            return 0;
//        return -1;
//#else
//        if (errno == 0)
//            return -1;
//        if (errno == EAGAIN)
//            return 0;
//#endif		
//        return len;
        return -1;
    }
    if (len > 0)
        return len;
    else
        return -1;
}

void EndClient(void * pParam)
{
    CLIENT_PARAM * localParam = (CLIENT_PARAM *) pParam;
    MyLock();
    localParam->nEndFlag = 1;
    localParam->pServerParam->pClientSocket[localParam->nIndexOff] = -1;
    delete (CLIENT_PARAM *) localParam;
    MyUnlock();
#ifdef	_WIN32
    Sleep(50);
    _endthread();
    return;
#else
    SysSleep(50);
    pthread_exit(NULL);
    return;
#endif

}

int WhoIsMaster(std::string &strMasterIP)
{
    syslog(LOG_DEBUG, "Enter WhoIsMaster\n");
    int nLen = 0;
    int nLoopTotal = 0;
    int nLoopMax = 20 * 300; //	300 秒判断选循环
#define	nMaxLen 0x1000
    char pBuffer[nMaxLen + 1] = {0};
    char pNewBuffer[nMaxLen + 1] = {0};

    int nSocketErrorFlag = 0;

    int nNewLen = 0;
    
    int nNewSocket = -1;
    int nTimes  = 0;
//    g_socketInfoLock.lock();
//    nTimes  = atoi(g_configInfo.strTryTimes.c_str());
//    g_socketInfoLock.unlock();
    
    int nManagerSize2 = g_configInfo.managerVect.size();
    
    while (!GlobalStopFlag&&!nSocketErrorFlag)
    {
            while (1)
            {
                int nStatus = 0;
                m_ETHLocker.lock();
                nStatus = m_nETHStatus;
                m_ETHLocker.unlock();
                if (nStatus == 1)
                {
                    //nTimes = 0;
                    break;
                }
                else
                {
                    nTimes = 0;
                    sleep(3);
                }
            }
        
        
        nNewSocket = CreateSocket();
        if (nNewSocket == -1)
        {
            //	不能建立套接字，直接返回
            syslog(LOG_DEBUG, "Can't create socket\n");
            sleep(10);
            nNewSocket = CreateSocket();
            if (nNewSocket == -1)
            {
                continue;
            }
        }
          
        syslog(LOG_DEBUG, "nManagerSize2:%d,%d\n", nManagerSize2,nTimes);
        if (nManagerSize2 >= 1 && nTimes <= 1)
        {
            
                {
                    int nStatus = 0;
                    m_ETHLocker.lock();
                    nStatus = m_nETHStatus;
                    m_ETHLocker.unlock();
                    if (nStatus == 1)
                    {
                        
                    }
                    else
                    {
                        nTimes = 0;
                        if (nNewSocket != -1)
                        {
                            CloseSocket(nNewSocket);
                            nNewSocket = -1;
                        }
                        continue;
                    }
                }             
            
            
            
            //configInfo.strLocalIPHandle
            
            // if (nManagerSize2 >= 1 && g_strLastMasterIP == g_configInfo.managerVect[0].strRemoteIPHandle)
//            if (nManagerSize2 >= 1 && g_strLastMasterIP == g_configInfo.strLocalIPHandle)
//            {
//                ++nTimes;
//                CloseSocket(nNewSocket);
//                sleep(3);
//                continue;
//            }            
            
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
            WriteConfigFile(g_chnlSet);
            
            

            
            
            
            boost::timer tt;
//            g_configInfo.managerVect[0].nStatus = 1;
//            g_configInfo.managerVect[1].nStatus = 0;    
//            g_configInfo.managerVect[2].nStatus = 0;
//            g_configInfo.managerVect[3].nStatus = 0;
            int GlobalRemotePort = atoi(g_configInfo.managerVect[0].strRemoteSynPortHandle.c_str());
            if (ConnectSocket(nNewSocket, g_configInfo.managerVect[0].strRemoteIPHandle.c_str(), GlobalRemotePort) <= 0)
            {
                ++nTimes;
                //	不能建立连接，直接返回
                //CloseSocket(nSocket);
                syslog(LOG_DEBUG, "Can't connect host1:%lf,%s\n", tt.elapsed(), strMasterIP.c_str());
                CloseSocket(nNewSocket);
                sleep(3);
                continue;
                //EndClient(pParam);
            } 
            ++nTimes;
            syslog(LOG_DEBUG, "connect host:%s:%d\n", g_configInfo.managerVect[0].strRemoteIPHandle.c_str(), GlobalRemotePort);
        }
        else if (nManagerSize2 >= 2 && nTimes > 1 && nTimes <= 3)
        {
            
                {
                    int nStatus = 0;
                    m_ETHLocker.lock();
                    nStatus = m_nETHStatus;
                    m_ETHLocker.unlock();
                    if (nStatus == 1)
                    {
                        
                    }
                    else
                    {
                        nTimes = 0;
                        if (nNewSocket != -1)
                        {
                            CloseSocket(nNewSocket);
                            nNewSocket = -1;
                        }
                        continue;
                    }
                }              
            
            
            
            
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
            WriteConfigFile(g_chnlSet);
            
//            g_configInfo.managerVect[0].nStatus = 0;
//            g_configInfo.managerVect[1].nStatus = 1;    
//            g_configInfo.managerVect[2].nStatus = 0;
//            g_configInfo.managerVect[3].nStatus = 0;
            int GlobalRemotePort = atoi(g_configInfo.managerVect[1].strRemoteSynPortHandle.c_str());
            if (ConnectSocket(nNewSocket, g_configInfo.managerVect[1].strRemoteIPHandle.c_str(), GlobalRemotePort) <= 0)
            {
                ++nTimes;
                //	不能建立连接，直接返回
                //CloseSocket(nSocket);
                syslog(LOG_DEBUG, "Can't connect host2\n");
                CloseSocket(nNewSocket);
                sleep(3);
                continue;
                //EndClient(pParam);
            } 
            ++nTimes;
            syslog(LOG_DEBUG, "connect host:%s:%d\n", g_configInfo.managerVect[1].strRemoteIPHandle.c_str(), GlobalRemotePort);
        }
        else if (nManagerSize2 >= 3 && nTimes > 3 && nTimes <= 5)
        {
            
                {
                    int nStatus = 0;
                    m_ETHLocker.lock();
                    nStatus = m_nETHStatus;
                    m_ETHLocker.unlock();
                    if (nStatus == 1)
                    {
                        
                    }
                    else
                    {
                        nTimes = 0;
                        if (nNewSocket != -1)
                        {
                            CloseSocket(nNewSocket);
                            nNewSocket = -1;
                        }
                        continue;
                    }
                }  
            
            
            
            
            
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
            WriteConfigFile(g_chnlSet);
            
//            g_configInfo.managerVect[0].nStatus = 0;
//            g_configInfo.managerVect[1].nStatus = 0;    
//            g_configInfo.managerVect[2].nStatus = 1;
//            g_configInfo.managerVect[3].nStatus = 0;
            int GlobalRemotePort = atoi(g_configInfo.managerVect[2].strRemoteSynPortHandle.c_str());
            if (ConnectSocket(nNewSocket, g_configInfo.managerVect[2].strRemoteIPHandle.c_str(), GlobalRemotePort) <= 0)
            {
                ++nTimes;
                //	不能建立连接，直接返回
                //CloseSocket(nSocket);
                syslog(LOG_DEBUG, "Can't connect host3\n");
                CloseSocket(nNewSocket);
                sleep(3);
                continue;
                //EndClient(pParam);
            }   
            ++nTimes;
            syslog(LOG_DEBUG, "connect host:%s:%d\n", g_configInfo.managerVect[2].strRemoteIPHandle.c_str(), GlobalRemotePort);
        }   
        else if (nManagerSize2 >= 4 && nTimes > 5 && nTimes <= 7)
        {
            
            {
                int nStatus = 0;
                m_ETHLocker.lock();
                nStatus = m_nETHStatus;
                m_ETHLocker.unlock();
                if (nStatus == 1)
                {

                }
                else
                {
                    nTimes = 0;
                    if (nNewSocket != -1)
                    {
                        CloseSocket(nNewSocket);
                        nNewSocket = -1;
                    }
                    continue;
                }
            }              
            
            
            
            
            
            
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
            WriteConfigFile(g_chnlSet);
            
//            g_configInfo.managerVect[0].nStatus = 0;
//            g_configInfo.managerVect[1].nStatus = 0;    
//            g_configInfo.managerVect[2].nStatus = 0;
//            g_configInfo.managerVect[3].nStatus = 1;
            int GlobalRemotePort = atoi(g_configInfo.managerVect[3].strRemoteSynPortHandle.c_str());
            if (ConnectSocket(nNewSocket, g_configInfo.managerVect[3].strRemoteIPHandle.c_str(), GlobalRemotePort) <= 0)
            {
                ++nTimes;
                //	不能建立连接，直接返回
                //CloseSocket(nSocket);
                syslog(LOG_DEBUG, "Can't connect host4\n");
                CloseSocket(nNewSocket);
                sleep(3);
                continue;
                //EndClient(pParam);
            }
            ++nTimes;
            syslog(LOG_DEBUG, "connect host:%s:%d\n", g_configInfo.managerVect[3].strRemoteIPHandle.c_str(), GlobalRemotePort);            
        }              
        else
        {
            {
                int nStatus = 0;
                m_ETHLocker.lock();
                nStatus = m_nETHStatus;
                m_ETHLocker.unlock();
                if (nStatus == 1)
                {

                }
                else
                {
                    nTimes = 0;
                    if (nNewSocket != -1)
                    {
                        CloseSocket(nNewSocket);
                        nNewSocket = -1;
                    }
                    continue;
                }
            }             
            
            
            
            
            
            
            nTimes = 0;
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
            WriteConfigFile(g_chnlSet);
            
//            g_configInfo.managerVect[0].nStatus = 0;
//            g_configInfo.managerVect[1].nStatus = 0;    
//            g_configInfo.managerVect[2].nStatus = 0;
//            g_configInfo.managerVect[3].nStatus = 0;
            
            sleep(3);
            continue;
        }
        set_keep_live(nNewSocket, 3, 5);


        // ===========first start
        int index = 0;
        int nTotalLen = 0;

        memcpy(pNewBuffer + index, &nTotalLen, 4);
        index += 4;

        int nType = 8;
        memcpy(pNewBuffer + index, &nType, 4);
        index += 4;
        
        nNewLen = nTotalLen = index;
        memcpy(pNewBuffer, &nTotalLen, 4);
        
        nLen = SocketWrite(nNewSocket, pNewBuffer, nNewLen, 30);
        syslog(LOG_DEBUG, "write len:%d,%d\n", nLen, __LINE__);
        
        if (nLen < 0) //	断开
            continue;
        
        nLen = SocketRead(nNewSocket, pBuffer, nMaxLen);
        CloseSocket(nNewSocket);
        syslog(LOG_DEBUG, "read len:%d,%d\n", nLen, __LINE__);
        //	读取客户端数据
        if (nLen > 0)
        {
            pBuffer[nLen] = 0;

            // ===========start
            index = 0;
            nTotalLen = 0;
            memcpy(&nTotalLen, pBuffer + index, 4);
            index += 4;

            nType = 0;
            memcpy(&nType, pBuffer + index, 4);
            index += 4;

            int nTmp = 0;
            memcpy(&nTmp, pBuffer + index, 4);
            index += 4;               
            
            if (nTmp > 0)
            {
                nLoopTotal = 0;
                char szBuffer[50] = {0};
                memcpy(szBuffer, pBuffer + index, nTmp);
                index += 4;    
                
                strMasterIP = szBuffer;
                g_strLastMasterIP   = strMasterIP;
                break;
            }
            else
            {
                syslog(LOG_DEBUG, "nLoopTotal:%d\n", nLoopTotal);
                
                
                // avoid down back---->main
                {
                    int nStatus = 0;
                    m_ETHLocker.lock();
                    nStatus = m_nETHStatus;
                    m_ETHLocker.unlock();
                    if (nStatus == 1)
                    {
                        
                    }
                    else
                    {
                        nTimes = 0;
                        continue;
                    }
                }                
                
                
                
                
                
                
                
                // select master
                ++nLoopTotal;
                if (nLoopTotal > 10)
                {
                    nLoopTotal = 0;
                    int nManagerSize = g_configInfo.managerVect.size();
                    for (int i = 0; i < nManagerSize; ++i)
                    {
                        if (g_configInfo.managerVect[i].strRemoteIPHandle == g_strLastMasterIP)
                        {
                            if (i < nManagerSize - 1)
                            {
                                strMasterIP         = g_configInfo.managerVect[i + 1].strRemoteIPHandle;
                                g_strLastMasterIP   = strMasterIP;  
                                break;
                            }
                            else
                            {
                                strMasterIP         = g_configInfo.managerVect[0].strRemoteIPHandle;
                                g_strLastMasterIP   = strMasterIP;  
                                break;                                
                            }
                        }
                    }
                    
                    if (g_strLastMasterIP == "")
                    {
                                strMasterIP         = g_configInfo.managerVect[nManagerSize - 1].strRemoteIPHandle;
                                g_strLastMasterIP   = strMasterIP;                         
                    }
                    
                    syslog(LOG_DEBUG, "strMasterIP:%s\n", strMasterIP.c_str());
                    return 1;
                }
                // 
            }
        }

        sleep(3);
    }
    return 0;
}



void * ClientProc(void * pParam)
{
    syslog(LOG_DEBUG, "Enter ClientProc Func\n");
    //	客户端处理线程,把接收的数据发送给目标机器,并把目标机器返回的数据返回到客户端
    //    CLIENT_PARAM * localParam=(CLIENT_PARAM *) pParam;
    //	localParam->nEndFlag=0;
    //	int nSocket=localParam->nSocket;

    int nLen = 0;
    int nLoopTotal = 0;
    int nLoopMax = 20 * 30; //	30 秒判断选循环
#define	nMaxLen 0x1000
    char pBuffer[nMaxLen + 1] = {0};
    char pNewBuffer[nMaxLen + 1] = {0};

    int nSocketErrorFlag = 0;

    int nNewLen = 0;
    
    int nNewSocket = -1;
    int nTimes  = 0; // atoi(g_configInfo.strTryTimes.c_str());
    
    
    while (1)
    {
        int nStatus = 0;
        m_ETHLocker.lock();
        nStatus = m_nETHStatus;
        m_ETHLocker.unlock();
        if (nStatus == 1)
        {
            break;
        }
        else
        {
            sleep(3);
        }
    }
    
    
    
    std::string strMasterIP = "";
    WhoIsMaster(strMasterIP);
    
    
    
    while (!GlobalStopFlag&&!nSocketErrorFlag)
    {
        while (1)
        {
            int nStatus = 0;
            m_ETHLocker.lock();
            nStatus = m_nETHStatus;
            m_ETHLocker.unlock();
            if (nStatus == 1)
            {
                break;
            }
            else
            {
                sleep(3);
            }
        }
        
        
        if (strMasterIP.empty())
        { 
            WhoIsMaster(strMasterIP);
            sleep(3);
            continue;
        }
        
        
        nNewSocket = CreateSocket();
        if (nNewSocket == -1)
        {
            //	不能建立套接字，直接返回
            syslog(LOG_DEBUG, "Can't create socket\n");
            sleep(1);
            nNewSocket = CreateSocket();
            if (nNewSocket == -1)
            {
                continue;
            }
        }
        
        
        //SetSocketNotBlock(nNewSocket);
        //if (ConnectSocket(nNewSocket, GlobalRemoteHost, GlobalRemotePort) <= 0)
        
//        if (nTimes <= 3)
//        {
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
//            WriteConfigFile(g_chnlSet);
//            
//            g_configInfo.managerVect[0].nStatus = 1;
//            g_configInfo.managerVect[1].nStatus = 0;    
//            g_configInfo.managerVect[2].nStatus = 0;
//            g_configInfo.managerVect[3].nStatus = 0;
//            int GlobalRemotePort = atoi(g_configInfo.managerVect[0].strRemotePortHandle.c_str());
//            if (ConnectSocket(nNewSocket, g_configInfo.managerVect[0].strRemoteIPHandle.c_str(), GlobalRemotePort) <= 0)
//            {
//                ++nTimes;
//                //	不能建立连接，直接返回
//                //CloseSocket(nSocket);
//                syslog(LOG_DEBUG, "Can't connect host1\n");
//                CloseSocket(nNewSocket);
//                sleep(1);
//                continue;
//                //EndClient(pParam);
//            }            
//        }
//        else if (nTimes > 3 && nTimes <= 7)
//        {
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
//            WriteConfigFile(g_chnlSet);
//            
//            g_configInfo.managerVect[0].nStatus = 0;
//            g_configInfo.managerVect[1].nStatus = 1;    
//            g_configInfo.managerVect[2].nStatus = 0;
//            g_configInfo.managerVect[3].nStatus = 0;
//            int GlobalRemotePort = atoi(g_configInfo.managerVect[1].strRemotePortHandle.c_str());
//            if (ConnectSocket(nNewSocket, g_configInfo.managerVect[1].strRemoteIPHandle.c_str(), GlobalRemotePort) <= 0)
//            {
//                ++nTimes;
//                //	不能建立连接，直接返回
//                //CloseSocket(nSocket);
//                syslog(LOG_DEBUG, "Can't connect host2\n");
//                CloseSocket(nNewSocket);
//                sleep(1);
//                continue;
//                //EndClient(pParam);
//            }              
//        }
//        else if (nTimes > 7 && nTimes <= 11)
//        {
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
//            WriteConfigFile(g_chnlSet);
//            
//            g_configInfo.managerVect[0].nStatus = 0;
//            g_configInfo.managerVect[1].nStatus = 0;    
//            g_configInfo.managerVect[2].nStatus = 1;
//            g_configInfo.managerVect[3].nStatus = 0;
//            int GlobalRemotePort = atoi(g_configInfo.managerVect[2].strRemotePortHandle.c_str());
//            if (ConnectSocket(nNewSocket, g_configInfo.managerVect[2].strRemoteIPHandle.c_str(), GlobalRemotePort) <= 0)
//            {
//                ++nTimes;
//                //	不能建立连接，直接返回
//                //CloseSocket(nSocket);
//                syslog(LOG_DEBUG, "Can't connect host3\n");
//                CloseSocket(nNewSocket);
//                sleep(1);
//                continue;
//                //EndClient(pParam);
//            }              
//        }   
//        else if (nTimes > 11 && nTimes <= 15)
//        {
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
//            WriteConfigFile(g_chnlSet);
//            
//            g_configInfo.managerVect[0].nStatus = 0;
//            g_configInfo.managerVect[1].nStatus = 0;    
//            g_configInfo.managerVect[2].nStatus = 0;
//            g_configInfo.managerVect[3].nStatus = 1;
//            int GlobalRemotePort = atoi(g_configInfo.managerVect[3].strRemotePortHandle.c_str());
//            if (ConnectSocket(nNewSocket, g_configInfo.managerVect[3].strRemoteIPHandle.c_str(), GlobalRemotePort) <= 0)
//            {
//                ++nTimes;
//                //	不能建立连接，直接返回
//                //CloseSocket(nSocket);
//                syslog(LOG_DEBUG, "Can't connect host4\n");
//                CloseSocket(nNewSocket);
//                sleep(1);
//                continue;
//                //EndClient(pParam);
//            }              
//        }              
//        else
//        {
//            nTimes = 0;
//            char szTimes[10] = {0};
//            sprintf(szTimes, "%d", nTimes);
//            g_configInfo.strTryTimes = szTimes;
//            WriteConfigFile(g_chnlSet);
//            
//            g_configInfo.managerVect[0].nStatus = 0;
//            g_configInfo.managerVect[1].nStatus = 0;    
//            g_configInfo.managerVect[2].nStatus = 0;
//            g_configInfo.managerVect[3].nStatus = 0;
//            
//            sleep(1);
//            continue;
//        }
        
        
//========================  
        int GlobalRemotePort = atoi(g_configInfo.managerVect[0].strRemotePortHandle.c_str());
        if (ConnectSocket(nNewSocket, strMasterIP.c_str(), GlobalRemotePort) <= 0)
        {
            ++nTimes;
            //	不能建立连接，直接返回
            //CloseSocket(nSocket);
            syslog(LOG_DEBUG, "Can't connect host:%s\n", strMasterIP.c_str());
            CloseSocket(nNewSocket);
            strMasterIP = "";
            sleep(3);
            continue;
            //EndClient(pParam);
        }  
        syslog(LOG_DEBUG, "connect host:%s\n", strMasterIP.c_str());
        
        
        set_keep_live(nNewSocket, 3, 5);

        //SetSocketNotBlock(nNewSocket);

        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        
        setsockopt(nNewSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        // ===========first start
        int index = 0;
        int nTotalLen = 0;

        memcpy(pNewBuffer + index, &nTotalLen, 4);
        index += 4;

        int nType = 1;
        memcpy(pNewBuffer + index, &nType, 4);
        index += 4;

        
        int nCount = 0;
        
        // 重新登录
        g_configInfo.m_lock.lock();
        //set<int> chnlSetFirst(g_configInfo.chnlSet);
        
        set<int> chnlSetFirst(g_chnlSet);
//        if (g_configInfo.strRebootFlag == "0")
        {
            nCount = chnlSetFirst.size(); // g_configInfo.chnlSet.size();
            memcpy(pNewBuffer + index, &nCount, 4);
            index += 4;

            
            set<int>::iterator it;
            for (it = chnlSetFirst.begin(); it != chnlSetFirst.end(); ++it)
            {
                int chnlNo = *it;
                memcpy(pNewBuffer + index, &chnlNo, 4);
                index += 4;
            }            
        }
//        else
//        {
//            nCount = 0;
//            memcpy(pNewBuffer + index, &nCount, 4);
//            index += 4;            
//        }
        g_configInfo.m_lock.unlock();
        
        
        //=============================================
        {
            int nStatus = 0;
            m_ETHLocker.lock();
            nStatus = m_nETHStatus;
            m_ETHLocker.unlock();
            if (nStatus == 1)
            {
                ;
            }
            else
            {
                CloseSocket(nNewSocket);
                strMasterIP = "";
                sleep(3);
                continue;
            }
        }        
        //===========================================
        
        
        
        


        nNewLen = nTotalLen = index;
        memcpy(pNewBuffer, &nTotalLen, 4);
        // ===========first end            
        nLen = SocketWrite(nNewSocket, pNewBuffer, nNewLen, 30);
        syslog(LOG_DEBUG, "write len:%d\n", nLen);
        
        if (nLen < 0) //	断开
            continue;

        
        g_socketInfoLock.lock();
        g_socketInfo.nSendTime = nNewSocket;
        g_socketInfo.nSendTime = time(0);
        g_socketInfoLock.unlock();
        

        set<int> chnlSetLast;
        
        //if (g_configInfo.strRebootFlag == "0")
        {
            chnlSetLast = chnlSetFirst;
        }
        
        
        while (!GlobalStopFlag)
        {
            nLoopTotal++;
            
            // 2016-6-12
            //if (ReceiveTimer(nNewSocket) > 0)
            {
            
                nLen = SocketRead(nNewSocket, pBuffer, nMaxLen);
                SysSleep(1000);

                syslog(LOG_DEBUG, "read len:%d,%d\n", nLen, __LINE__);
                boost::timer tt;
                //	读取客户端数据
                if (nLen > 0)
                {
                    pBuffer[nLen] = 0;
                    nLoopTotal = 0;
                    
                    g_socketInfoLock.lock();
                    g_socketInfo.nSocket   = nNewSocket;
                    g_socketInfo.nRecvTime = time(0);
                    g_socketInfoLock.unlock();                    
                    
                    

                    // ===========start
                    index = 0;
                    nTotalLen = 0;
                    memcpy(&nTotalLen, pBuffer + index, 4);
                    index += 4;

                    nType = 0;
                    memcpy(&nType, pBuffer + index, 4);
                    index += 4;

                    nCount = 0;
                    memcpy(&nCount, pBuffer + index, 4);
                    index += 4;

                    set<int> chnlSet;
                    for (int i = 0; i < nCount; ++i)
                    {
                        int chnlNo = 0;
                        memcpy(&chnlNo, pBuffer + index, 4);
                        index += 4;
                        chnlSet.insert(chnlNo);
                    }
                    // ===========end

                    
                    set<int> diffSet;
                    set<int> chnlSetNew(chnlSetLast);
                    set_difference(chnlSet.begin(), chnlSet.end(), chnlSetLast.begin(), chnlSetLast.end(), inserter(diffSet, diffSet.begin()));

                    chnlSetNew = chnlSet;
                    chnlSetLast = chnlSet;
                    if (diffSet.empty())
                    {
                        ;
                    }
                    else
                    {
                        WriteConfigFile(chnlSetNew);
                        
                        g_chnlSetLock.lock();
                        g_chnlSet = chnlSetNew;
                        g_chnlSetLock.unlock();
                        // start channel
                        set<int>::iterator it;
                        for (it = diffSet.begin(); it != diffSet.end(); ++it)
                        {
                            int chnlNo = *it;
                            char sh_cmd[512] = {0};

                            // sprintf(sh_cmd, "bash /root/jiezisoft/channel%d/startchannel.sh", chnlNo);
                            sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/startchannel.sh &", chnlNo);
                            system(sh_cmd);
                            syslog(LOG_DEBUG, "%s\n", sh_cmd);
                        }
                    }
                    

//                    //=======将最新的通道和集中监控服务的对应关系发送给平台代理============
//                    {
//
//                        //                   T_ChnanelServerInfo;
//
//
//                        char chMsgAck[1024 * 20] = {0};
//
//                        int sendLen = 0;
//                        memcpy(chMsgAck, SYS_NET_MSGHEAD, 8); //锟斤拷头锟斤拷锟?
//                        sendLen += 8;
//
//                        NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chMsgAck + sendLen);
//                        pHead->msg_type = SYS_MSG_PUBLISH_CHANNELSERVER;
//                        pHead->packet_len = sizeof (T_ChnanelServerInfo);
//
//
//                        sendLen += sizeof (NET_PACKET_HEAD);
//
//                        T_ChnanelServerInfo* pChannelServerAck = (T_ChnanelServerInfo*) (chMsgAck + sendLen);
//
//                        //       T_ChannelInfo
//                        //       ServerChannelInfoMap[string(pChannelInfo->szNodeIP)] = pChannelInfo;
//                        pChannelServerAck->channel_no = 0;
//
//                        int index = 0;
//                        set<int>::iterator it;
//                        for (it = chnlSetNew.begin(); it != chnlSetNew.end(); ++it)
//                        {
//                            int chnlNo = *it;
//                            for (int i = 0; i < g_chnlVect.chnnlVect.size(); ++i)
//                            {
//                                if (chnlNo == g_chnlVect.chnnlVect[i].nChannelNo)
//                                {
//                                    //send
//                                    int nUpPort = g_monitorInfo.nUpPort;
//                                    int nDowPort = g_monitorInfo.nDownPort;
//                                    std::string strAreaNo = g_chnlVect.chnnlVect[i].strAreaNo;
//                                    std::string strChnlNo = g_chnlVect.chnnlVect[i].strChnlNo;
//
//                                    std::string strIEType           = g_chnlVect.chnnlVect[i].strIEType;
//                                    std::string strCenterServerIP   = g_configInfo.strLocalIPHandle;
//                                    int nCenterServerPort           = g_monitorInfo.nUpPort;
//                                    int nCenterServerPortAck        = g_monitorInfo.nDownPort;
//
//
//                                    strcpy(pChannelServerAck->channelInfo[index].szAreaID, strAreaNo.c_str());
//                                    strcpy(pChannelServerAck->channelInfo[index].szChannelNo, strChnlNo.c_str());
//                                    strcpy(pChannelServerAck->channelInfo[index].szIEType, strIEType.c_str());
//
//                                    strcpy(pChannelServerAck->channelInfo[index].szCenterControlIP, strCenterServerIP.c_str());
//                                    pChannelServerAck->channelInfo[index].nCenterControlPort = nCenterServerPort;
//                                    pChannelServerAck->channelInfo[index].nCenterControlAckPort = nCenterServerPortAck;
//
//                                    index++;
//
//                                    pChannelServerAck->channel_no++;
//                                }
//                            }
//                        }
//
//
//
//                        sendLen += sizeof (T_ChnanelServerInfo) + pChannelServerAck->channel_no * sizeof (T_ChnanelServer);
//                        pHead->packet_len = sizeof (T_ChnanelServerInfo) + pChannelServerAck->channel_no * sizeof (T_ChnanelServer);
//                        memcpy(chMsgAck + sendLen, SYS_NET_MSGTAIL, 8); //锟斤拷头锟斤拷锟?
//                        sendLen += 8;
//
//
//                        // time add
//                        
//                        std::map<int, structCenterProxyServerInfo>::iterator iter;
//                        for (iter = g_centerProxyServerMap.begin(); index != 0 && iter != g_centerProxyServerMap.end(); iter++)
//                        {
//                            structCenterProxyServerInfo proxyServer = iter->second;
//
//                            int nSendSocket = CreateSocket();
//                            if (nSendSocket == -1)
//                            {
//                                //	不能建立套接字，直接返回
//                                syslog(LOG_DEBUG, "Can't create socket\n");
//                                sleep(1);
//                                nSendSocket = CreateSocket();
//                                if (nSendSocket == -1)
//                                {
//                                    continue;
//                                }
//                            }
//                            //SetSocketNotBlock(nNewSocket);
//                            //if (ConnectSocket(nNewSocket, GlobalRemoteHost, GlobalRemotePort) <= 0)
//                            if (ConnectSocket(nSendSocket, proxyServer.strServerIP.c_str(), proxyServer.nListenPort) <= 0)
//                            {
//                                //	不能建立连接，直接返回
//                                //CloseSocket(nSocket);
//                                syslog(LOG_DEBUG, "Can't connect proxy server\n");
//                                CloseSocket(nSendSocket);
//                                sleep(1);
//                                continue;
//                                //EndClient(pParam);
//                            }
//
//
//                            int nSendLen = SocketWrite(nSendSocket, chMsgAck, sendLen, 30);
//
//                            if(nSendLen!=sendLen)
//                            {
//                                syslog(LOG_DEBUG, "send to proxy server fail......\n");
//                            }
//
//                            CloseSocket(nSendSocket);
//
//                        }
//
//                        // time end
//                        
//
//                    }
//
//
//                    //==============================================平台代理处理结束============

                    //SysSleep(6000);
                    // ===========rebuild start
                    index = 0;
                    nTotalLen = 0;

                    memcpy(pNewBuffer + index, &nTotalLen, 4);
                    index += 4;

                    nType = 1;
                    memcpy(pNewBuffer + index, &nType, 4);
                    index += 4;

                    nCount = chnlSetNew.size();
                    memcpy(pNewBuffer + index, &nCount, 4);
                    index += 4;


                    set<int>::iterator it;
                    for (it = chnlSetNew.begin(); it != chnlSetNew.end(); ++it)
                    {
                        int chnlNo = *it;
                        memcpy(pNewBuffer + index, &chnlNo, 4);
                        index += 4;
                    }
                    nTotalLen = nNewLen = index;
                    memcpy(pNewBuffer, &nTotalLen, 4);
                    // rebuild ===========end                        
                    syslog(LOG_DEBUG, "cost time:%lf......\n", tt.elapsed());

                    nLen = SocketWrite(nNewSocket, pNewBuffer, nNewLen, 30);
                    syslog(LOG_DEBUG, "write len:%d,%d\n", nLen, __LINE__);
                    if (nLen < 0) //	断开
                        break;
                    
                    if (nLen > 0)
                    {
                        g_strLastMasterIP = strMasterIP;
                    }
                }
                if (nLen < 0)
                {
                    SysSleep(50);
                    //	读断开
                    break;
                }                
            }
            
            
            //=============================================
            {
                int nStatus = 0;
                m_ETHLocker.lock();
                nStatus = m_nETHStatus;
                m_ETHLocker.unlock();
                if (nStatus == 1)
                {
                    ;
                }
                else
                {
                    CloseSocket(nNewSocket);
                    strMasterIP = "";
                    sleep(3);
                    break;
                }
            }        
            //===========================================            


            if ((nSocketErrorFlag == 0)&&(nLoopTotal > 0))
            {
                SysSleep(50);
                if (nLoopTotal >= nLoopMax)
                {
                    nLoopTotal = 0;
                }
            }
        }
    }
    CloseSocket(nNewSocket);
    //EndClient(pParam);
    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}

void * AcceptProc(void * pParam)
{
    return NULL;
}

void * VoteProc(void * pParam)
{
    while (!GlobalStopFlag)
    {
        std::map<int, structWorkerChannelInfo>::iterator mapIter;
        set<int> chnlSetNew;
        structWorkerChannelInfo tmpWorker;

        g_sockChannelLock.lock();
        for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();)
        {
            int nLastTime = mapIter->second.nTime;
            int nNowTime = time(0);
            int nDiffTime = nNowTime - nLastTime;
            if (nDiffTime > 10 * 60)
            {
                tmpWorker = mapIter->second;
                g_sockChannelMap.erase(mapIter);
                break;
            }
            else
            {
                mapIter++;
            }
        }
        g_sockChannelLock.unlock();
        sleep(1);

        g_sockChannelLock.lock();
        set<int> chnlSetLast(tmpWorker.chnlSet);
        set<int>::iterator it;
        for (it = chnlSetLast.begin(), mapIter = g_sockChannelMap.begin(); it != chnlSetLast.end(); ++it)
        {
            int chnlNo = *it;
            //for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();mapIter++)
            if (mapIter != g_sockChannelMap.end())
            {
                mapIter->second.chnlSet.insert(chnlNo);
                mapIter++;
            }
            else
            {
                mapIter = g_sockChannelMap.begin();
                mapIter->second.chnlSet.insert(chnlNo);
                mapIter++;
            }
        }

        g_sockChannelLock.unlock();

        sleep(3);
    }


    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}

// 2016-6-14

void * CancelRecvThread(void * pParam)
{
    int nDelayFlag = 0;
    int nSendTime = 0;
    int nRecvTime = 0;
    pthread_t localThreadId;
    while (!GlobalStopFlag)
    {
        
        while (1)
        {
            int nStatus = 0;
            m_ETHLocker.lock();
            nStatus = m_nETHStatus;
            m_ETHLocker.unlock();
            if (nStatus == 1)
            {
                break;
            }
            else
            {
                sleep(3);
            }
        }        
        
        //syslog(LOG_DEBUG, "CancelRecvThread process!\n");
        int nSocket = g_socketInfo.nSocket;
        if (nSocket > 0)
        {
            int nNowTime    = time(0);
            g_socketInfoLock.lock();
            nSendTime = g_socketInfo.nSendTime;
            nRecvTime = g_socketInfo.nRecvTime;
            g_socketInfoLock.unlock();  
            
            //syslog(LOG_DEBUG, "nNowTime nSendTime nRecvTime:%d,%d,%d\n", nNowTime, nSendTime, nRecvTime);
            if (abs(nNowTime - nSendTime) > 60 && abs(nNowTime - nRecvTime) > 60)
            {
                if (nDelayFlag == 1)
                {
                    sleep(60);
                    nDelayFlag = 0;
                    continue;
                }
                
                syslog(LOG_DEBUG, "nRecvTime time less nSendTime\n");
                CloseSocket(nSocket);
                
                g_socketInfoLock.lock();
                g_socketInfo.nRecvTime = g_socketInfo.nSendTime = nNowTime;
                g_socketInfoLock.unlock(); 
                
                
                // cancel
                if (g_socketInfo.threadId != 0)
                {
                    syslog(LOG_DEBUG, "begin cancel process\n");
                    pthread_cancel(g_socketInfo.threadId);
                    syslog(LOG_DEBUG, "begin cancel process finish\n");
                    
//                    g_socketInfoLock.lock();
//                    g_configInfo.strTryTimes = 4;
//                    g_socketInfoLock.unlock();
                    
                    sleep(1);
                    //==========================
                    
                    
                    syslog(LOG_DEBUG, "begin create thread \n");
                    int nThreadErr = 0;

                    nThreadErr = pthread_create(&localThreadId, NULL, (THREADFUNC) ClientProc, NULL);
                    if (nThreadErr == 0)
                    {
                        pthread_detach(localThreadId);
                    }
                    //	释放线程私有数据,不必等待pthread_join();

                    if (nThreadErr)
                    {
                        //	建立线程失败
                        MyUninitLock();
                        UninitWinSock();
                        pthread_exit(NULL);
                        return NULL;
                    }

                    
                    g_socketInfo.threadId   = localThreadId;
                    nDelayFlag = 1;
                    
                    syslog(LOG_DEBUG, "end create thread \n");
                }
//                //sleep(60);
//                for (int i = 0;  i < 1200; ++i)
//                {
//                    SysSleep(50);
//                }
            }  
        }

        sleep(3);
    }


    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}




#if !defined(_WIN32)&&!defined(_WIN64)

extern "C" void sig_term(int signo)
{
    if (signo == SIGTERM)
    {
        GlobalStopFlag = 1;
        signal(SIGTERM, sig_term);
        signal(SIGHUP, SIG_IGN);
        signal(SIGKILL, sig_term);
        signal(SIGINT, sig_term);
        signal(SIGPIPE, SIG_IGN);
        signal(SIGALRM, SIG_IGN);
    }
}

long daemon_init()
{
    pid_t pid;
    signal(SIGALRM, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if ((pid = fork()) < 0)
        return 0;
    else if (pid != 0)
        exit(0);
    setsid();

    signal(SIGTERM, sig_term);
    signal(SIGINT, sig_term);
    signal(SIGHUP, SIG_IGN);
    signal(SIGKILL, sig_term);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    return 1;
}
#else

BOOL WINAPI ControlHandler(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
        case CTRL_BREAK_EVENT: // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT: // SERVICE_CONTROL_STOP in debug mode
        case CTRL_CLOSE_EVENT: //	close console	
            GlobalStopFlag = 1;
            return TRUE;
            break;
    }
    return FALSE;
}

#endif



void * RequestHostInfoProc(void * pParam)
{
	syslog(LOG_DEBUG, "Enter RequestHostInfoProc Func\n");
	//	客户端处理线程,把接收的数据发送给目标机器,并把目标机器返回的数据返回到客户端

	int nLen = 0;
	int nLoopTotal = 0;
	int nLoopMax = 20 * 300; //	300 秒判断选循环
#define	nMaxLen 0x1000
	char pBuffer[nMaxLen + 1] = {0};
	char pNewBuffer[nMaxLen + 1] = {0};

	int nSocketErrorFlag = 0;

	int nNewLen = 0;

	int nNewSocket = -1;
	int nNewSocket2 = -1;
	int nTimes  = 0; 

	std::string strMasterIP = "";
	// WhoIsMaster(strMasterIP);



	while (!GlobalStopFlag&&!nSocketErrorFlag)
	{
                while (1)
                {
                    int nStatus = 0;
                    m_ETHLocker.lock();
                    nStatus = m_nETHStatus;
                    m_ETHLocker.unlock();
                    if (nStatus == 1)
                    {
                        break;
                    }
                    else
                    {
                        sleep(3);
                    }
                }
            
            
		strMasterIP = g_strLastMasterIP;
		if (strMasterIP.empty())
		{
			// WhoIsMaster(strMasterIP);
                        syslog(LOG_DEBUG, "g_strLastMasterIP is null\n");
			sleep(1);
			continue;
		}


		nNewSocket = CreateSocket();
		if (nNewSocket == -1)
		{
			//	不能建立套接字，直接返回
			syslog(LOG_DEBUG, "Can't create socket\n");
			sleep(1);
			nNewSocket = CreateSocket();
			if (nNewSocket == -1)
			{
				continue;
			}
		}

		//========================  
		int GlobalRemotePort = atoi(g_configInfo.managerVect[0].strRemoteSynPortHandle.c_str());
		if (ConnectSocket(nNewSocket, strMasterIP.c_str(), GlobalRemotePort) <= 0)
		{
			++nTimes;
			//	不能建立连接，直接返回
			//CloseSocket(nSocket);
			//syslog(LOG_DEBUG, "Can't connect host:%s\n", strMasterIP.c_str());
			CloseSocket(nNewSocket);
			strMasterIP = "";
			sleep(1);
			continue;
			//EndClient(pParam);
		}  
		syslog(LOG_DEBUG, "connect host:%s\n", strMasterIP.c_str());


		set_keep_live(nNewSocket, 3, 5);

		//SetSocketNotBlock(nNewSocket);

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		setsockopt(nNewSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		// ===========first start
		int index = 0;
		int nTotalLen = 0;

		memcpy(pNewBuffer + index, &nTotalLen, 4);
		index += 4;

		int nType = 26;
		memcpy(pNewBuffer + index, &nType, 4);
		index += 4;

		// 重新登录
		nNewLen = nTotalLen = index;
		memcpy(pNewBuffer, &nTotalLen, 4);
		// ===========first end            
		nLen = SocketWrite(nNewSocket, pNewBuffer, nNewLen, 30);
		syslog(LOG_DEBUG, "write len:%d\n", nLen);

		if (nLen < 0) //	断开
		{
			CloseSocket(nNewSocket);
			sleep(1);
			continue;
		}

		// 2016-6-12
		{
			nLen = SocketRead(nNewSocket, pBuffer, nMaxLen);
			CloseSocket(nNewSocket);
			syslog(LOG_DEBUG, "read len:%d,%d\n", nLen, __LINE__);
			boost::timer tt;
			//	读取客户端数据
			if (nLen > 0)
			{
				pBuffer[nLen] = 0;
				nLoopTotal = 0;
                                
//0000000000000000000000000000000000000000000

				int iXmlLen = 0;
				if (nLen > 12)
				{
					memcpy(&iXmlLen, pBuffer + 8, sizeof(iXmlLen));
				}
				else
				{
					sleep(1);
					continue;
				}

				if (iXmlLen <= 0)
				{
					sleep(1);
					continue;
				}

				char *pXmlBuffer = new char[iXmlLen + 1];
				if (pXmlBuffer == NULL)
				{
					sleep(1);
					continue;
				}
				memcpy(pXmlBuffer, pBuffer + 12, iXmlLen);
				pXmlBuffer[iXmlLen] = '\0';
				std::string strXmlData(pXmlBuffer);
				delete []pXmlBuffer;

				TiXmlDocument doc;
				doc.Parse(strXmlData.c_str());

				TiXmlNode* node = 0;
				TiXmlElement* itemElement = 0;

				TiXmlHandle docHandle( &doc );
				TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("HostRunInfo"); 

				TiXmlElement *xmlRootElement = NULL;
				TiXmlElement *xmlSubElement = NULL;
				TiXmlNode *pNode = NULL;
				TiXmlNode *pNodeTmp = NULL;
				TiXmlNode *pConfigInfoNode = NULL;
				TiXmlElement* xmlListElement = NULL;

				pConfigInfoNode = doc.FirstChild("HostRunInfo");
				if (pConfigInfoNode == NULL)
				{
					syslog(LOG_DEBUG, "pConfigNode:%p\n", pConfigInfoNode);
					sleep(1);
					continue;
				}

				xmlRootElement = pConfigInfoNode->ToElement();
				if (xmlRootElement != NULL && (pNode = xmlRootElement->FirstChild("HostList")) != NULL)
				{
					xmlListElement = pNode->ToElement();

					for (pNode = xmlListElement->FirstChild("HostInfo"); pNode != NULL; pNode = pNode->NextSibling("HostInfo"))
					{
						structHostInfo      tmpHostInfo;
						string HostName        = "";
						TiXmlNode *pChnlNoNode = NULL;
						std::vector<int> outputVect1;


						xmlSubElement = pNode->ToElement() ;
						if (xmlSubElement != NULL)
							pNodeTmp = xmlSubElement->FirstChildElement("HostName");

						if (pNodeTmp != NULL && pNodeTmp->ToElement() != NULL && pNodeTmp->ToElement()->GetText() != 0)
							HostName = pNodeTmp->ToElement()->GetText();
						tmpHostInfo.strHostName = HostName;

						for (pChnlNoNode = pNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
						{
							string strChnlNo = "";
							xmlSubElement = pChnlNoNode->ToElement();

							if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
							{
								strChnlNo = pChnlNoNode->ToElement()->GetText();
								int nChnlNo = atoi(strChnlNo.c_str());
								tmpHostInfo.chnlSet.insert(nChnlNo);
								syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo.strHostName.c_str(), strChnlNo.c_str());
							}

							syslog(LOG_DEBUG, "\n");
						} 
						outputVect1.resize(tmpHostInfo.chnlSet.size());
						std::copy(tmpHostInfo.chnlSet.begin(), tmpHostInfo.chnlSet.end(), outputVect1.begin());


						//=====================================================
						g_configInfo.m_lock.lock();
						if (g_configInfo.strLocalIPHandle == tmpHostInfo.strHostName)
						{
                                                    g_configInfo.chnlSet.clear();
                                                        
                                                    for (int j = 0; j < outputVect1.size(); ++j)
                                                    {
                                                        g_configInfo.chnlSet.insert(outputVect1[j]);
                                                    }   
						}
						g_configInfo.m_lock.unlock();
						//==================================================
					}
				}    

//000000000000000000000000000000000000000000000000                                

				// ===========start
                                if (g_configInfo.strLocalIPHandle != strMasterIP)
				{
					nNewSocket2 = CreateSocket();
					if (nNewSocket2 == -1)
					{
						//	不能建立套接字，直接返回
						syslog(LOG_DEBUG, "Can't create socket\n");
						sleep(1);
						nNewSocket2 = CreateSocket();
						if (nNewSocket2 == -1)
						{
							sleep(1);
							continue;
						}
					}

					//========================  
					std::string strLocalIP = "127.0.0.1";
					int GlobalRemotePort = atoi(g_configInfo.managerVect[0].strRemoteSynPortHandle.c_str());
					if (ConnectSocket(nNewSocket2, strLocalIP.c_str(), GlobalRemotePort) <= 0)
					{
						++nTimes;
						//	不能建立连接，直接返回
						CloseSocket(nNewSocket2);
						sleep(1);
						continue;
						//EndClient(pParam);
					}  


					set_keep_live(nNewSocket2, 3, 5);

					//SetSocketNotBlock(nNewSocket2);

					struct timeval tv;
					tv.tv_sec = 5;
					tv.tv_usec = 0;

					setsockopt(nNewSocket2, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

					int nType = 28;
					memcpy(pBuffer + 4, &nType, 4);
					//index += 4;

					//// 重新登录
					nNewLen = nLen;
					//memcpy(pNewBuffer, &nTotalLen, 4);
					// ===========first end            
					nLen = SocketWrite(nNewSocket2, pBuffer, nNewLen, 30);
					CloseSocket(nNewSocket2);
					syslog(LOG_DEBUG, "write len:%d\n", nLen);
                                        
				}
			}             
		}
		sleep(3);
	}

	SysSleep(50);
	pthread_exit(NULL);
	return NULL;
}


// if_name like "ath0", "eth0". Notice: call this function
// need root privilege.
// return value:
// -1 -- error , details can check errno
// 1 -- interface link up
// 0 -- interface link down.

int get_netlink_status(const char *if_name)
{
	int skfd;
	struct ifreq ifr;
	struct ethtool_value edata;
	edata.cmd = ETHTOOL_GLINK;
	edata.data = 0;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_data = (char *) &edata;
	if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) == 0)
		return -1;

	if(ioctl( skfd, SIOCETHTOOL, &ifr ) == -1)
	{
		close(skfd);
		return -1;
	}
	close(skfd);
	return edata.data;
}

void * ProcessNICProc(void * pParam)
{
	while (1)
	{
            int i = 0;
            //i = GetNetStat();g_configInfo.strNetCardName
            //i = get_netlink_status("eth0");
            i = get_netlink_status(g_configInfo.strNetCardName.c_str());
            
            if (i == 1)
            {
                    m_ETHLocker.lock();
                    m_nETHStatus = 1;
                    m_ETHLocker.unlock();
                    // online
                    //syslog(LOG_DEBUG, "%s running\n", g_configInfo.strNetCardName.c_str());
            }
            else
            {
                g_strLastMasterIP = "";
                // offline
                g_chnlSetLock.lock();
                if (!g_chnlSet.empty())
                    g_chnlSet.clear();
                g_chnlSetLock.unlock();
                WriteConfigFile(g_chnlSet);
                
                m_ETHLocker.lock();
                m_nETHStatus = 0;
                m_ETHLocker.unlock();
                syslog(LOG_DEBUG, "%s not running\n", g_configInfo.strNetCardName.c_str());

                // 杀掉机器N上的通道进程
                {
                        // 杀掉机器N上的主通道进程
                        syslog(LOG_DEBUG, "kill  process on machine\n");
                        char sh_cmd[512] = {0};
                        sprintf(sh_cmd, "bash /root/jiezisoft/hckillserver.sh");
                        system(sh_cmd);
                        //exit(0);
                }
                //sleep(5);
            }

            sleep(15);
	} 
        SysSleep(50);
	pthread_exit(NULL);
	return NULL;
}


void * SendToPlatformProc(void * pParam)
{
    while (1)
    {
            //=======将最新的通道和集中监控服务的对应关系发送给平台代理============
            {
                set<int> chnlSetNew(g_chnlSet);
                if (chnlSetNew.empty())
                {
                    sleep(5);
                    continue;
                }


                //                   T_ChnanelServerInfo;
                char chMsgAck[1024 * 20] = {0};

                int sendLen = 0;
                memcpy(chMsgAck, SYS_NET_MSGHEAD, 8); //锟斤拷头锟斤拷锟?
                sendLen += 8;

                NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chMsgAck + sendLen);
                pHead->msg_type = SYS_MSG_PUBLISH_CHANNELSERVER;
                pHead->packet_len = sizeof (T_ChnanelServerInfo);


                sendLen += sizeof (NET_PACKET_HEAD);

                T_ChnanelServerInfo* pChannelServerAck = (T_ChnanelServerInfo*) (chMsgAck + sendLen);

                //       T_ChannelInfo
                //       ServerChannelInfoMap[string(pChannelInfo->szNodeIP)] = pChannelInfo;
                pChannelServerAck->channel_no = 0;

                int index = 0;
                set<int>::iterator it;
                for (it = chnlSetNew.begin(); it != chnlSetNew.end(); ++it)
                {
                    int chnlNo = *it;
                    for (int i = 0; i < g_chnlVect.chnnlVect.size(); ++i)
                    {
                        if (chnlNo == g_chnlVect.chnnlVect[i].nChannelNo)
                        {
                            //send
                            int nUpPort = g_monitorInfo.nUpPort;
                            int nDowPort = g_monitorInfo.nDownPort;
                            std::string strAreaNo = g_chnlVect.chnnlVect[i].strAreaNo;
                            std::string strChnlNo = g_chnlVect.chnnlVect[i].strChnlNo;

                            std::string strIEType           = g_chnlVect.chnnlVect[i].strIEType;
                            std::string strCenterServerIP   = g_configInfo.strLocalIPHandle;
                            int nCenterServerPort           = g_monitorInfo.nUpPort;
                            int nCenterServerPortAck        = g_monitorInfo.nDownPort;


                            strcpy(pChannelServerAck->channelInfo[index].szAreaID, strAreaNo.c_str());
                            strcpy(pChannelServerAck->channelInfo[index].szChannelNo, strChnlNo.c_str());
                            strcpy(pChannelServerAck->channelInfo[index].szIEType, strIEType.c_str());

                            strcpy(pChannelServerAck->channelInfo[index].szCenterControlIP, strCenterServerIP.c_str());
                            pChannelServerAck->channelInfo[index].nCenterControlPort = nCenterServerPort;
                            pChannelServerAck->channelInfo[index].nCenterControlAckPort = nCenterServerPortAck;

                            index++;

                            pChannelServerAck->channel_no++;
                        }
                    }
                }



                sendLen += sizeof (T_ChnanelServerInfo) + pChannelServerAck->channel_no * sizeof (T_ChnanelServer);
                pHead->packet_len = sizeof (T_ChnanelServerInfo) + pChannelServerAck->channel_no * sizeof (T_ChnanelServer);
                memcpy(chMsgAck + sendLen, SYS_NET_MSGTAIL, 8); //锟斤拷头锟斤拷锟?
                sendLen += 8;


                // time add

                std::map<int, structCenterProxyServerInfo>::iterator iter;
                for (iter = g_centerProxyServerMap.begin(); index != 0 && iter != g_centerProxyServerMap.end(); iter++)
                {
                    structCenterProxyServerInfo proxyServer = iter->second;

                    int nSendSocket = CreateSocket();
                    if (nSendSocket == -1)
                    {
                        //	不能建立套接字，直接返回
                        syslog(LOG_DEBUG, "Can't create socket\n");
                        sleep(1);
                        nSendSocket = CreateSocket();
                        if (nSendSocket == -1)
                        {
                            continue;
                        }
                    }
                    //SetSocketNotBlock(nNewSocket);
                    //if (ConnectSocket(nNewSocket, GlobalRemoteHost, GlobalRemotePort) <= 0)
                    if (ConnectSocket(nSendSocket, proxyServer.strServerIP.c_str(), proxyServer.nListenPort) <= 0)
                    {
                        //	不能建立连接，直接返回
                        //CloseSocket(nSocket);
                        //syslog(LOG_DEBUG, "Can't connect proxy server\n");
                        CloseSocket(nSendSocket);
                        sleep(1);
                        continue;
                        //EndClient(pParam);
                    }


                    int nSendLen = SocketWrite(nSendSocket, chMsgAck, sendLen, 30);

                    if(nSendLen!=sendLen)
                    {
                        syslog(LOG_DEBUG, "send to proxy server fail......\n");
                    }

                    CloseSocket(nSendSocket);

                }

                // time end
            }


            //==============================================平台代理处理结束============   
            sleep(5);
    }
    
    
    SysSleep(50);
    pthread_exit(NULL);
    return NULL;    
}

int rsmain()
//int main(int argc, char * argv[])
{
    openlog("JZWorker", LOG_PID, LOG_LOCAL7);
    
//    char szLocalIP[50] = {0};
//    std::string strDeviceName = "eth0";
//    get_gw_ip(strDeviceName.c_str(), szLocalIP);
//    
//    if (szLocalIP[0] == '\0')
//        return -1;

    //std::string strFileName = "/root/jiezisoft/channel1/CenterConfig.xml";
    std::string strFileName = "/root/jiezisoft/A0/C0/CenterConfig.xml";
    ReadConfigInfo(g_configInfo);
    ReadMonitorConfigInfo(strFileName);
    
    WriteConfigFile(g_chnlSet);
    //return 0;

    for (int i = 1; i <= 30; ++i)
    {
        char szFileName[512] = {0};
        //sprintf(szFileName, "/root/jiezisoft/channel%d/ChannelConfig.xml", i);
        sprintf(szFileName, "/root/jiezisoft/A1/C%d/ChannelConfig.xml", i);
        
        
        strFileName = szFileName;
        if (access(strFileName.c_str(), F_OK) == 0)
        {
            ReadChnlConfigInfo(i, strFileName);
        }
    }

    //WriteConfigFile();
    //if(argc<4)
    //{
    //	PrintUsage(argv[0]);
    //	return -1;
    //}
    
    
//    int nRemotePort = -1;
//    nRemotePort = atoi(g_configInfo.strRemotePortHandle.c_str());
//    //nRemotePort		= atoi(g_configInfo.strKaKouPortHandle.c_str());
//    if (nRemotePort <= 0)// || (nRemotePort<=0))
//    {
//        syslog(LOG_DEBUG, "invalid local port or remote port \n");
//        return -1;
//    }

    // char * szRemoteHost = "192.168.1.130"; // argv[2];
    //GlobalRemotePort = nRemotePort;
    //strcpy(GlobalRemoteHost, g_configInfo.strKaKouIPHandle.c_str());
    //#if !defined(_WIN32)&&!defined(_WIN64)
    ////	become a daemon program
    //	daemon_init();
    //#else
    //	InitWinSock();
    //    SetConsoleCtrlHandler( ControlHandler, TRUE );
    //#endif

    pthread_t localThreadId;

    int nThreadErr = 0;

    nThreadErr = pthread_create(&localThreadId, NULL, (THREADFUNC) ClientProc, NULL);
    if (nThreadErr == 0)
    {
        pthread_detach(localThreadId);
    }
    //	释放线程私有数据,不必等待pthread_join();

    if (nThreadErr)
    {
        //	建立线程失败
        MyUninitLock();
        UninitWinSock();
        return -1;
    }
    
    g_socketInfo.threadId   = localThreadId;

    
            //if (g_configInfo.strHostRole == "mainhost")
        {
            pthread_t ThreadId2;

            int nThreadErr2=0;

            nThreadErr2=pthread_create(&ThreadId2,NULL, (THREADFUNC)CancelRecvThread,NULL);
            if(nThreadErr2==0)
                    pthread_detach(ThreadId2);
    //	释放线程私有数据,不必等待pthread_join();

            if(nThreadErr2)
            {
    //	建立线程失败
                    MyUninitLock();
                    UninitWinSock();
                    return -1;
            }              
        }
    
//========================================================
		{
			pthread_t ThreadId3;

			int nThreadErr3=0;

			nThreadErr3 = pthread_create(&ThreadId3,NULL, (THREADFUNC)RequestHostInfoProc,NULL);
			if (nThreadErr3 == 0)
				pthread_detach(ThreadId3);
			//	释放线程私有数据,不必等待pthread_join();

			if (nThreadErr3)
			{
				//	建立线程失败
				MyUninitLock();
				UninitWinSock();
				return -1;
			}              
		}  
        
//================================================================
		{
			pthread_t ThreadId4;

			int nThreadErr4=0;

			nThreadErr4 = pthread_create(&ThreadId4,NULL, (THREADFUNC)ProcessNICProc,NULL);
			if (nThreadErr4 == 0)
				pthread_detach(ThreadId4);
			//	释放线程私有数据,不必等待pthread_join();

			if (nThreadErr4)
			{
				//	建立线程失败
				MyUninitLock();
				UninitWinSock();
				return -1;
			}              
		}    
                
                
//================================================================
		{
			pthread_t ThreadId5;

			int nThreadErr5=0;

			nThreadErr5 = pthread_create(&ThreadId5,NULL, (THREADFUNC)SendToPlatformProc,NULL);
			if (nThreadErr5 == 0)
				pthread_detach(ThreadId5);
			//	释放线程私有数据,不必等待pthread_join();

			if (nThreadErr5)
			{
				//	建立线程失败
				MyUninitLock();
				UninitWinSock();
				return -1;
			}              
		}                  
                
                
                
    
    
    while ((GlobalStopFlag == 0))
    {
        SysSleep(500);
    }
    MyUninitLock();
    UninitWinSock();
    return 0;
}


int Daemon()
{
    pid_t pid;
    pid = fork();
    if (pid < 0)
    {
        return -1;
    }
    else if (pid != 0)
    {
        exit(0);
    }
    setsid();
    return 0;
}

#define SOFTWARE_VERSION  "version 1.0.0.0"         //软件版本号

int main(int argc, char *argv[])
{
    int iRet;
    int iStatus;
    pid_t pid;
    //显示版本号
    if (argc == 2)
    {
        //如果是查看版本号
        if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "-V") || !strcmp(argv[1], "-Version") || !strcmp(argv[1], "-version"))
        {
            printf("%s  %s\n", argv[0], SOFTWARE_VERSION);
            return 0;
        }
    }

    Daemon();

createchildprocess:
    //开始创建子进程
    //syslog(LOG_DEBUG, "begin to create	the child process of %s\n", argv[0]);

    int itest = 0;
    switch (fork())//switch(fork())
    {
        case -1 : //创建子进程失败
            //syslog(LOG_DEBUG, "cs创建子进程失败\n");
            return -1;
        case 0://子进程
            //syslog(LOG_DEBUG, "cs创建子进程成功\n");
            rsmain();
            return -1;
        default://父进程
            pid = ::wait(&iStatus);
            //syslog(LOG_DEBUG, "子进程退出，5秒后重新启动......\n");
            sleep(3);
            goto createchildprocess; //重新启动进程
            break;
    }
    return 0;
}


