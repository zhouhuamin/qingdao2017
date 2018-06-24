// main.cpp : Defines the entry point for the console application.
//	map local port to remote host port

#include	<pthread.h>
#include        <sys/ioctl.h>
#include	<sys/socket.h>
#include	<sys/types.h>
#include        <netinet/tcp.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include 	<arpa/inet.h>
#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>

#include <sys/stat.h>
#include <unistd.h>
#include <linux/sockios.h>  
#include <linux/ethtool.h>

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
        
        pthread_mutex_t MyMutex2;
        
        
int GlobalStopFlag=0;
#define	MAX_SOCKET	0x2000
//	max client per server
#define MAX_HOST_DATA 4096
int GlobalRemotePort=-1;
char GlobalRemoteHost[256];

sem	g_sem;


struct JZMessage
{
    int         nLen;
    int         nType;
    int         nCount;
    set<int>    chnlSet;
    JZMessage():nLen(0),nType(0),nCount(0),chnlSet()
    {
        ;
    }
};

typedef struct _SERVER_PARAM
{
	int nSocket;
	sockaddr_in ServerSockaddr;
	int nEndFlag;	//	判断accept 线程是否结束
	int pClientSocket[MAX_SOCKET];	//	存放客户端连接的套接字
}	SERVER_PARAM;

typedef	struct _CLIENT_PARAM
{
	int nSocket;
	unsigned int nIndexOff;
	int nEndFlag;	//	判断处理客户端连接的线程是否结束
	SERVER_PARAM * pServerParam;
	char szClientName[256];
}	CLIENT_PARAM;

struct structHostInfo
{
    std::string strHostName;
    std::set<int>   chnlSet;
    structHostInfo():strHostName(""),chnlSet()
    {
        ;
    }    
};

struct structConfigInfo
{
    //std::string strKaKouIPHandle;
    //std::string strKaKouPortHandle;
    //std::string strWcfURLHandle;
    locker      m_lock;
    std::string strMaster;
    std::string strHostRole;
    std::string strLocalPortHandle;
    std::string strLocalPortHandle2;
    std::string strLocalIP;
    std::string strNetCardName;
    std::string strHeartbeatTime;
    
    std::vector<structHostInfo>   hostInfoVect;
};


struct structWorkerChannelInfo
{
    int             nSocket;
    time_t          nTime;
    std::set<int>   chnlSet;
    structWorkerChannelInfo():nSocket(0),nTime(0),chnlSet()
    {
        ;
    }
};

struct structThreadInfo
{
    time_t          nSendTime;
    time_t          nRecvTime;
    pthread_t       threadId;
    
    structThreadInfo() : nSendTime(0), nRecvTime(0), threadId(0)
    {
        ;
    }
};


structConfigInfo g_configInfo;

locker                                      g_sockChannelLock;
std::map<std::string, structWorkerChannelInfo>      g_sockChannelMap;
std::string g_strConfigFileName = "";

std::map<pthread_t, structThreadInfo> 	g_threadMap;
locker					g_threadMapLock;

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


void ReadConfigInfo(struct structConfigInfo &configInfo)
{
    char exeFullPath[256]	= {0};
    char EXE_FULL_PATH[256]	= {0};

    GetProcessPath(exeFullPath); //得到程序模块名称，全路径

    string strFullExeName(exeFullPath);
    int nLast = strFullExeName.find_last_of("/");
    strFullExeName = strFullExeName.substr(0, nLast + 1);
    strcpy(EXE_FULL_PATH, strFullExeName.c_str());

    char cpPath[256] = {0};
    char temp[256] = {0};
    sprintf(cpPath, "%sJZManagerConfig.xml", strFullExeName.c_str());
    g_strConfigFileName = cpPath;

    TiXmlDocument doc(cpPath);
    doc.LoadFile();

    TiXmlNode* node = 0;
    TiXmlElement* areaInfoElement = 0;
    TiXmlElement* itemElement = 0;

    TiXmlHandle docHandle( &doc );
    TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("ConfigInfo");
    TiXmlHandle HostRoleHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("HostRole", 0).FirstChild();
    //TiXmlHandle KaKouPortHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("KaKouPort", 0).FirstChild();
    //TiXmlHandle WcfURLHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("WcfURL", 0).FirstChild();
    TiXmlHandle LocalPortHandle     = docHandle.FirstChildElement("ConfigInfo").ChildElement("LocalPort", 0).FirstChild();
    TiXmlHandle LocalPortHandle2    = docHandle.FirstChildElement("ConfigInfo").ChildElement("LocalPort2", 0).FirstChild();
    
    //TiXmlHandle LocalIPHandle    = docHandle.FirstChildElement("ConfigInfo").ChildElement("LocalIP", 0).FirstChild();
    TiXmlHandle NetCardNameHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("NetCardName", 0).FirstChild();
    TiXmlHandle HeartbeatTimeHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("HeartbeatTime", 0).FirstChild();

    string strLocalPortHandle	= "";
    if (HostRoleHandle.Node() != NULL)
            configInfo.strHostRole = HostRoleHandle.Text()->Value();
    
    g_configInfo.m_lock.lock();
    if (g_configInfo.strHostRole == "mainhost")
    {
        configInfo.strMaster = "yes";
    }
    else if (g_configInfo.strHostRole == "backhost")
    {
        configInfo.strMaster = "no";
    }
    else
    {
        configInfo.strMaster = "no";
    }
    g_configInfo.m_lock.unlock();

    if (LocalPortHandle.Node() != NULL)
            configInfo.strLocalPortHandle   = strLocalPortHandle = LocalPortHandle.Text()->Value();
    if (LocalPortHandle2.Node() != NULL)
            configInfo.strLocalPortHandle2  = LocalPortHandle2.Text()->Value();        
    if (NetCardNameHandle.Node() != NULL)
    {
        configInfo.strNetCardName           = NetCardNameHandle.Text()->Value(); 
        char szLocalIP[50] = {0};
        std::string strDeviceName = configInfo.strNetCardName;
        get_gw_ip(strDeviceName.c_str(), szLocalIP);
        if (szLocalIP[0] == '\0')
            return;
        configInfo.strLocalIP = szLocalIP;
    }
    if (HeartbeatTimeHandle.Node() != NULL)
            configInfo.strHeartbeatTime  = HeartbeatTimeHandle.Text()->Value();      
    
    
        
    TiXmlElement *xmlRootElement = NULL;
    TiXmlElement *xmlSubElement = NULL;
    TiXmlNode *pNode = NULL;
    TiXmlNode *pNodeTmp = NULL;
    TiXmlNode *pConfigInfoNode = NULL;
    TiXmlElement* xmlListElement = NULL;

    pConfigInfoNode = doc.FirstChild("ConfigInfo");
    if (pConfigInfoNode == NULL)
    {
        syslog(LOG_DEBUG, "pConfigNode:%p,%d\n", pConfigInfoNode,__LINE__);
        return;
    }

    //xmlRootElement = pConfigInfoNode->ToElement();
    
    xmlRootElement = pConfigInfoNode->ToElement();
    if (xmlRootElement != NULL && (pNode = xmlRootElement->FirstChild("HostList")) != NULL)
    {
        xmlListElement = pNode->ToElement();

        for (pNode = xmlListElement->FirstChild("HostInfo"); pNode != NULL; pNode = pNode->NextSibling("HostInfo"))
        {
            structHostInfo      tmpHostInfo;
            string HostName        = "";
            TiXmlNode *pChnlNoNode = NULL;

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
            g_configInfo.m_lock.lock();
            g_configInfo.hostInfoVect.push_back(tmpHostInfo);
            g_configInfo.m_lock.unlock();
        }
    }     
    
    
    
    
    
    
    
//    
//    if (xmlRootElement)
//    {
//        TiXmlNode *pConfigNode      = NULL;
//        TiXmlNode *pChnlNoNode      = NULL;
//        TiXmlNode *pHostNameNode    = NULL;
//        structHostInfo      tmpHostInfo1;
//        pConfigNode = xmlRootElement->FirstChild("HostInfo1");
//        if (pConfigNode != NULL)
//        {
//            pHostNameNode = pConfigNode->FirstChild("HostName");
//            if (pHostNameNode != NULL)
//            {
//                xmlSubElement = pHostNameNode->ToElement();
//                if (pHostNameNode != NULL && pHostNameNode->ToElement() != NULL && pHostNameNode->ToElement()->GetText() != 0)
//                {
//                    tmpHostInfo1.strHostName = pHostNameNode->ToElement()->GetText();
//                }
//            }
//            
//            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
//            {
//                string strChnlNo = "";
//                xmlSubElement = pChnlNoNode->ToElement();
//
//                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
//                {
//                    strChnlNo = pChnlNoNode->ToElement()->GetText();
//                    int nChnlNo = atoi(strChnlNo.c_str());
//                    tmpHostInfo1.chnlSet.insert(nChnlNo);
//                    syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo1.strHostName.c_str(), strChnlNo.c_str());
//                }
//
//                syslog(LOG_DEBUG, "\n");
//            }
//            g_configInfo.hostInfoVect.push_back(tmpHostInfo1);
//        }
//        
//        structHostInfo      tmpHostInfo2;
//        pConfigNode = xmlRootElement->FirstChild("HostInfo2");
//        if (pConfigNode != NULL)
//        {
//            pHostNameNode = pConfigNode->FirstChild("HostName");
//            if (pHostNameNode != NULL)
//            {
//                xmlSubElement = pHostNameNode->ToElement();
//                if (pHostNameNode != NULL && pHostNameNode->ToElement() != NULL && pHostNameNode->ToElement()->GetText() != 0)
//                {
//                    tmpHostInfo2.strHostName = pHostNameNode->ToElement()->GetText();
//                }
//            }
//            
//            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
//            {
//                string strChnlNo = "";
//                xmlSubElement = pChnlNoNode->ToElement();
//
//                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
//                {
//                    strChnlNo = pChnlNoNode->ToElement()->GetText();
//                    int nChnlNo = atoi(strChnlNo.c_str());
//                    tmpHostInfo2.chnlSet.insert(nChnlNo);
//                    syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo2.strHostName.c_str(), strChnlNo.c_str());
//                }
//
//                syslog(LOG_DEBUG, "\n");
//            }
//            g_configInfo.hostInfoVect.push_back(tmpHostInfo2);
//        }        
//        
//        
//        structHostInfo      tmpHostInfo3;
//        pConfigNode = xmlRootElement->FirstChild("HostInfo3");
//        if (pConfigNode != NULL)
//        {
//            pHostNameNode = pConfigNode->FirstChild("HostName");
//            if (pHostNameNode != NULL)
//            {
//                xmlSubElement = pHostNameNode->ToElement();
//                if (pHostNameNode != NULL && pHostNameNode->ToElement() != NULL && pHostNameNode->ToElement()->GetText() != 0)
//                {
//                    tmpHostInfo3.strHostName = pHostNameNode->ToElement()->GetText();
//                }
//            }
//            
//            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
//            {
//                string strChnlNo = "";
//                xmlSubElement = pChnlNoNode->ToElement();
//
//                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
//                {
//                    strChnlNo = pChnlNoNode->ToElement()->GetText();
//                    int nChnlNo = atoi(strChnlNo.c_str());
//                    tmpHostInfo3.chnlSet.insert(nChnlNo);
//                    syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo3.strHostName.c_str(), strChnlNo.c_str());
//                }
//
//                syslog(LOG_DEBUG, "\n");
//            }
//            g_configInfo.hostInfoVect.push_back(tmpHostInfo3);
//        }        
//        
//        structHostInfo      tmpHostInfo4;
//        pConfigNode = xmlRootElement->FirstChild("HostInfo4");
//        if (pConfigNode != NULL)
//        {
//            pHostNameNode = pConfigNode->FirstChild("HostName");
//            if (pHostNameNode != NULL)
//            {
//                xmlSubElement = pHostNameNode->ToElement();
//                if (pHostNameNode != NULL && pHostNameNode->ToElement() != NULL && pHostNameNode->ToElement()->GetText() != 0)
//                {
//                    tmpHostInfo4.strHostName = pHostNameNode->ToElement()->GetText();
//                }
//            }
//            
//            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
//            {
//                string strChnlNo = "";
//                xmlSubElement = pChnlNoNode->ToElement();
//
//                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
//                {
//                    strChnlNo = pChnlNoNode->ToElement()->GetText();
//                    int nChnlNo = atoi(strChnlNo.c_str());
//                    tmpHostInfo4.chnlSet.insert(nChnlNo);
//                    syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo4.strHostName.c_str(), strChnlNo.c_str());
//                }
//
//                syslog(LOG_DEBUG, "\n");
//            }
//            g_configInfo.hostInfoVect.push_back(tmpHostInfo4);
//        }            
//    }    
//













    
        
    syslog(LOG_DEBUG, "%s, %s", configInfo.strHostRole.c_str(), configInfo.strLocalPortHandle.c_str());
}



void ReadConfigInfo2(struct structConfigInfo &configInfo)
{
    char exeFullPath[256]	= {0};
    char EXE_FULL_PATH[256]	= {0};

    GetProcessPath(exeFullPath); //得到程序模块名称，全路径

    string strFullExeName(exeFullPath);
    int nLast = strFullExeName.find_last_of("/");
    strFullExeName = strFullExeName.substr(0, nLast + 1);
    strcpy(EXE_FULL_PATH, strFullExeName.c_str());

    char cpPath[256] = {0};
    char temp[256] = {0};
    sprintf(cpPath, "%sJZManagerConfig.xml", strFullExeName.c_str());
    g_strConfigFileName = cpPath;

    TiXmlDocument doc(cpPath);
    doc.LoadFile();

    TiXmlNode* node = 0;
    TiXmlElement* areaInfoElement = 0;
    TiXmlElement* itemElement = 0;

    TiXmlHandle docHandle( &doc );
    TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("ConfigInfo"); 

    TiXmlElement *xmlRootElement = NULL;
    TiXmlElement *xmlSubElement = NULL;
    TiXmlNode *pNode = NULL;
    TiXmlNode *pNodeTmp = NULL;
    TiXmlNode *pConfigInfoNode = NULL;
    TiXmlElement* xmlListElement = NULL;

    pConfigInfoNode = doc.FirstChild("ConfigInfo");
    if (pConfigInfoNode == NULL)
    {
        syslog(LOG_DEBUG, "pConfigNode:%p,%d\n", pConfigInfoNode, __LINE__);
        return;
    }

    //xmlRootElement = pConfigInfoNode->ToElement();
     g_configInfo.m_lock.lock();
     g_configInfo.hostInfoVect.clear();
     g_configInfo.m_lock.unlock();
    
    xmlRootElement = pConfigInfoNode->ToElement();
    if (xmlRootElement != NULL && (pNode = xmlRootElement->FirstChild("HostList")) != NULL)
    {
        xmlListElement = pNode->ToElement();

        for (pNode = xmlListElement->FirstChild("HostInfo"); pNode != NULL; pNode = pNode->NextSibling("HostInfo"))
        {
            structHostInfo      tmpHostInfo;
            string HostName        = "";
            TiXmlNode *pChnlNoNode = NULL;

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
            g_configInfo.m_lock.lock();
            g_configInfo.hostInfoVect.push_back(tmpHostInfo);
            g_configInfo.m_lock.unlock();
        }
    }     
    
    
    
    
    
    
    
//    
//    if (xmlRootElement)
//    {
//        TiXmlNode *pConfigNode      = NULL;
//        TiXmlNode *pChnlNoNode      = NULL;
//        TiXmlNode *pHostNameNode    = NULL;
//        structHostInfo      tmpHostInfo1;
//        pConfigNode = xmlRootElement->FirstChild("HostInfo1");
//        if (pConfigNode != NULL)
//        {
//            pHostNameNode = pConfigNode->FirstChild("HostName");
//            if (pHostNameNode != NULL)
//            {
//                xmlSubElement = pHostNameNode->ToElement();
//                if (pHostNameNode != NULL && pHostNameNode->ToElement() != NULL && pHostNameNode->ToElement()->GetText() != 0)
//                {
//                    tmpHostInfo1.strHostName = pHostNameNode->ToElement()->GetText();
//                }
//            }
//            
//            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
//            {
//                string strChnlNo = "";
//                xmlSubElement = pChnlNoNode->ToElement();
//
//                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
//                {
//                    strChnlNo = pChnlNoNode->ToElement()->GetText();
//                    int nChnlNo = atoi(strChnlNo.c_str());
//                    tmpHostInfo1.chnlSet.insert(nChnlNo);
//                    syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo1.strHostName.c_str(), strChnlNo.c_str());
//                }
//
//                syslog(LOG_DEBUG, "\n");
//            }
//            g_configInfo.hostInfoVect.push_back(tmpHostInfo1);
//        }
//        
//        structHostInfo      tmpHostInfo2;
//        pConfigNode = xmlRootElement->FirstChild("HostInfo2");
//        if (pConfigNode != NULL)
//        {
//            pHostNameNode = pConfigNode->FirstChild("HostName");
//            if (pHostNameNode != NULL)
//            {
//                xmlSubElement = pHostNameNode->ToElement();
//                if (pHostNameNode != NULL && pHostNameNode->ToElement() != NULL && pHostNameNode->ToElement()->GetText() != 0)
//                {
//                    tmpHostInfo2.strHostName = pHostNameNode->ToElement()->GetText();
//                }
//            }
//            
//            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
//            {
//                string strChnlNo = "";
//                xmlSubElement = pChnlNoNode->ToElement();
//
//                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
//                {
//                    strChnlNo = pChnlNoNode->ToElement()->GetText();
//                    int nChnlNo = atoi(strChnlNo.c_str());
//                    tmpHostInfo2.chnlSet.insert(nChnlNo);
//                    syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo2.strHostName.c_str(), strChnlNo.c_str());
//                }
//
//                syslog(LOG_DEBUG, "\n");
//            }
//            g_configInfo.hostInfoVect.push_back(tmpHostInfo2);
//        }        
//        
//        
//        structHostInfo      tmpHostInfo3;
//        pConfigNode = xmlRootElement->FirstChild("HostInfo3");
//        if (pConfigNode != NULL)
//        {
//            pHostNameNode = pConfigNode->FirstChild("HostName");
//            if (pHostNameNode != NULL)
//            {
//                xmlSubElement = pHostNameNode->ToElement();
//                if (pHostNameNode != NULL && pHostNameNode->ToElement() != NULL && pHostNameNode->ToElement()->GetText() != 0)
//                {
//                    tmpHostInfo3.strHostName = pHostNameNode->ToElement()->GetText();
//                }
//            }
//            
//            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
//            {
//                string strChnlNo = "";
//                xmlSubElement = pChnlNoNode->ToElement();
//
//                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
//                {
//                    strChnlNo = pChnlNoNode->ToElement()->GetText();
//                    int nChnlNo = atoi(strChnlNo.c_str());
//                    tmpHostInfo3.chnlSet.insert(nChnlNo);
//                    syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo3.strHostName.c_str(), strChnlNo.c_str());
//                }
//
//                syslog(LOG_DEBUG, "\n");
//            }
//            g_configInfo.hostInfoVect.push_back(tmpHostInfo3);
//        }        
//        
//        structHostInfo      tmpHostInfo4;
//        pConfigNode = xmlRootElement->FirstChild("HostInfo4");
//        if (pConfigNode != NULL)
//        {
//            pHostNameNode = pConfigNode->FirstChild("HostName");
//            if (pHostNameNode != NULL)
//            {
//                xmlSubElement = pHostNameNode->ToElement();
//                if (pHostNameNode != NULL && pHostNameNode->ToElement() != NULL && pHostNameNode->ToElement()->GetText() != 0)
//                {
//                    tmpHostInfo4.strHostName = pHostNameNode->ToElement()->GetText();
//                }
//            }
//            
//            for (pChnlNoNode = pConfigNode->FirstChild("ChannelNo"); pChnlNoNode != NULL; pChnlNoNode = pChnlNoNode->NextSibling("ChannelNo"))
//            {
//                string strChnlNo = "";
//                xmlSubElement = pChnlNoNode->ToElement();
//
//                if (pChnlNoNode != NULL && pChnlNoNode->ToElement() != NULL && pChnlNoNode->ToElement()->GetText() != 0)
//                {
//                    strChnlNo = pChnlNoNode->ToElement()->GetText();
//                    int nChnlNo = atoi(strChnlNo.c_str());
//                    tmpHostInfo4.chnlSet.insert(nChnlNo);
//                    syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo4.strHostName.c_str(), strChnlNo.c_str());
//                }
//
//                syslog(LOG_DEBUG, "\n");
//            }
//            g_configInfo.hostInfoVect.push_back(tmpHostInfo4);
//        }            
//    }    
//
 
    syslog(LOG_DEBUG, "%s, %s", configInfo.strHostRole.c_str(), configInfo.strLocalPortHandle.c_str());
}

void ReadConfigInfo3(struct structConfigInfo &configInfo)
{
	char exeFullPath[256]	= {0};
	char EXE_FULL_PATH[256]	= {0};

	GetProcessPath(exeFullPath); //得到程序模块名称，全路径

	string strFullExeName(exeFullPath);
	int nLast = strFullExeName.find_last_of("/");
	strFullExeName = strFullExeName.substr(0, nLast + 1);
	strcpy(EXE_FULL_PATH, strFullExeName.c_str());

	char cpPath[256] = {0};
	char temp[256] = {0};
	sprintf(cpPath, "%sHostInfo.xml", strFullExeName.c_str());
	//g_strConfigFileName = cpPath;

	TiXmlDocument doc(cpPath);
	doc.LoadFile();

	TiXmlNode* node = 0;
	TiXmlElement* areaInfoElement = 0;
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
		syslog(LOG_DEBUG, "pConfigNode:%p,%d\n", pConfigInfoNode, __LINE__);
		return;
	}

	//xmlRootElement = pConfigInfoNode->ToElement();
//	g_configInfo.m_lock.lock();
//	g_configInfo.hostInfoVect.clear();
//	g_configInfo.m_lock.unlock();

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
                        int nVectSize = g_configInfo.hostInfoVect.size();
			//g_configInfo.hostInfoVect.push_back(tmpHostInfo);
			for (int i = 0; i != nVectSize; ++i)
			{
				if (g_configInfo.hostInfoVect[i].strHostName == tmpHostInfo.strHostName)
				{
					g_configInfo.hostInfoVect[i].chnlSet.clear();
                                        break;
				}
			}
                        
			//g_configInfo.hostInfoVect.push_back(tmpHostInfo);
			for (int i = 0; i != nVectSize; ++i)
			{
                            if (g_configInfo.hostInfoVect[i].strHostName == tmpHostInfo.strHostName)
                            {
				for (int j = 0; j < outputVect1.size(); ++j)
				{
                                    g_configInfo.hostInfoVect[i].chnlSet.insert(outputVect1[j]);
				} 
                                break;
                            }
			}                      
                        
			g_configInfo.m_lock.unlock();
//==================================================




		}
	}     

	syslog(LOG_DEBUG, "%s, %s", configInfo.strHostRole.c_str(), configInfo.strLocalPortHandle.c_str());
}




void ReadConfigInfo4(struct structConfigInfo &configInfo)
{
	char exeFullPath[256]	= {0};
	char EXE_FULL_PATH[256]	= {0};

	GetProcessPath(exeFullPath); //得到程序模块名称，全路径

	string strFullExeName(exeFullPath);
	int nLast = strFullExeName.find_last_of("/");
	strFullExeName = strFullExeName.substr(0, nLast + 1);
	strcpy(EXE_FULL_PATH, strFullExeName.c_str());

	char cpPath[256] = {0};
	char temp[256] = {0};
	sprintf(cpPath, "%sHostInfo.xml", strFullExeName.c_str());
	//g_strConfigFileName = cpPath;

	TiXmlDocument doc(cpPath);
	doc.LoadFile();

	TiXmlNode* node = 0;
	TiXmlElement* areaInfoElement = 0;
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
		syslog(LOG_DEBUG, "pConfigNode:%p,%d\n", pConfigInfoNode, __LINE__);
		return;
	}

	//xmlRootElement = pConfigInfoNode->ToElement();
//	g_configInfo.m_lock.lock();
//	g_configInfo.hostInfoVect.clear();
//	g_configInfo.m_lock.unlock();

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
                        std::map<std::string, structWorkerChannelInfo>::iterator mapIter;
                        structWorkerChannelInfo tmpWorker;

                        g_sockChannelLock.lock();
                        mapIter = g_sockChannelMap.find(HostName);
                        if (mapIter != g_sockChannelMap.end())
                        {
                            ;
                        }
                        else
                        {
                            tmpWorker.nSocket   = -1;
                            tmpWorker.nTime     = time(0);
                            tmpWorker.chnlSet   = tmpHostInfo.chnlSet;
                            g_sockChannelMap.insert(make_pair(HostName, tmpWorker));
                            
                            syslog(LOG_DEBUG, "insert impossible\n");
                        }                        
                        g_sockChannelLock.unlock();
//==================================================
		}
	}     

	syslog(LOG_DEBUG, "%s, %s", configInfo.strHostRole.c_str(), configInfo.strLocalPortHandle.c_str());
}





















void WriteConfigFile()
{
//    using boost::property_tree::ptree;
//    ptree pt;
//
//    std::vector<int> outputVect1(g_configInfo.hostInfoVect[0].chnlSet.size());
//    std::copy(g_configInfo.hostInfoVect[0].chnlSet.begin(), g_configInfo.hostInfoVect[0].chnlSet.end(), outputVect1.begin());
// 
//    std::vector<int> outputVect2(g_configInfo.hostInfoVect[1].chnlSet.size());
//    std::copy(g_configInfo.hostInfoVect[1].chnlSet.begin(), g_configInfo.hostInfoVect[1].chnlSet.end(), outputVect2.begin());    
//    
//    std::vector<int> outputVect3(g_configInfo.hostInfoVect[2].chnlSet.size());
//    std::copy(g_configInfo.hostInfoVect[2].chnlSet.begin(), g_configInfo.hostInfoVect[2].chnlSet.end(), outputVect3.begin()); 
//
//    std::vector<int> outputVect4(g_configInfo.hostInfoVect[3].chnlSet.size());
//    std::copy(g_configInfo.hostInfoVect[3].chnlSet.begin(), g_configInfo.hostInfoVect[3].chnlSet.end(), outputVect4.begin()); 
//
//    //pt.add_child("ConfigInfo", pattr1);
//    std::string strHostRole = "backhost";
//    pt.put("ConfigInfo.HostRole",     strHostRole);    
//    pt.put("ConfigInfo.LocalPort",    g_configInfo.strLocalPortHandle);
//    pt.put("ConfigInfo.LocalPort2",   g_configInfo.strLocalPortHandle2);
//    pt.put("ConfigInfo.LocalIP",      g_configInfo.strLocalIP);
//    
//    pt.add("ConfigInfo.HostInfo1.HostName",    g_configInfo.hostInfoVect[0].strHostName);
//    pt.add("ConfigInfo.HostInfo1.ChannelNo",   outputVect1[0]);
//    pt.add("ConfigInfo.HostInfo1.ChannelNo",   outputVect1[1]);
//    
//    pt.add("ConfigInfo.HostInfo2.HostName",    g_configInfo.hostInfoVect[1].strHostName);
//    pt.add("ConfigInfo.HostInfo2.ChannelNo",   outputVect2[0]);
//    pt.add("ConfigInfo.HostInfo2.ChannelNo",   outputVect2[1]);
//    
//    pt.add("ConfigInfo.HostInfo3.HostName",    g_configInfo.hostInfoVect[2].strHostName);
//    pt.add("ConfigInfo.HostInfo3.ChannelNo",   outputVect3[0]);
//    pt.add("ConfigInfo.HostInfo3.ChannelNo",   outputVect3[1]);    
//
//    pt.add("ConfigInfo.HostInfo4.HostName",    g_configInfo.hostInfoVect[3].strHostName);
//    pt.add("ConfigInfo.HostInfo4.ChannelNo",   outputVect4[0]);
//    pt.add("ConfigInfo.HostInfo4.ChannelNo",   outputVect4[1]); 
//    
//    // 格式化输出，指定编码（默认utf-8）
//    boost::property_tree::xml_writer_settings<char> settings('\t', 1, "GB2312");
//
//    std::string filename = g_strConfigFileName;
//    write_xml(filename, pt, std::locale(), settings);  
    
    
    
    
    //============================================================================
    std::string strHostRole = "backhost";
    char szFileData[2048] = {0};
    sprintf(szFileData, "%s", "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n");
    sprintf(szFileData, "%s<ConfigInfo>\r\n\t<HostRole>%s</HostRole>\r\n",        szFileData,    strHostRole.c_str());
    sprintf(szFileData, "%s\t<LocalPort>%s</LocalPort>\r\n",      szFileData,    g_configInfo.strLocalPortHandle.c_str());
    sprintf(szFileData, "%s\t<LocalPort2>%s</LocalPort2>\r\n",    szFileData,    g_configInfo.strLocalPortHandle2.c_str());
    sprintf(szFileData, "%s\t<NetCardName>%s</NetCardName>\r\n",          szFileData,    g_configInfo.strNetCardName.c_str());
    sprintf(szFileData, "%s\t<HeartbeatTime>%s</HeartbeatTime>\r\n",          szFileData,    g_configInfo.strHeartbeatTime.c_str());
    sprintf(szFileData, "%s\t<HostList>\r\n",       szFileData);
    
    g_configInfo.m_lock.lock();
    for (int i = 0; i < g_configInfo.hostInfoVect.size(); ++i)
    {
        sprintf(szFileData, "%s\t\t<HostInfo>\r\n\t\t\t<HostName>%s</HostName>\r\n",      szFileData,    g_configInfo.hostInfoVect[i].strHostName.c_str());
        
        if (g_configInfo.hostInfoVect[i].chnlSet.size() <= 0)
            continue;
        
        std::vector<int> outputVect1(g_configInfo.hostInfoVect[i].chnlSet.size());
        std::copy(g_configInfo.hostInfoVect[i].chnlSet.begin(), g_configInfo.hostInfoVect[i].chnlSet.end(), outputVect1.begin());
        for (int j = 0; j < outputVect1.size(); ++j)
        {
            sprintf(szFileData, "%s\t\t\t<ChannelNo>%d</ChannelNo>\r\n",      szFileData,    outputVect1[j]);
        }
        sprintf(szFileData, "%s\t\t</HostInfo>\r\n",      szFileData);
    }
    g_configInfo.m_lock.unlock();
    sprintf(szFileData, "%s\t</HostList>\r\n",       szFileData);
    sprintf(szFileData, "%s</ConfigInfo>\r\n",       szFileData);
    
    std::string filename = g_strConfigFileName;
    FILE* pFile = fopen(filename.c_str(), "wb");
    if (!pFile)
    {

    }  
    else
    {
        fwrite(szFileData, 1, strlen(szFileData), pFile);
        fclose(pFile);
    }
}


void WriteHostInfoFile()
{
    //============================================================================
    std::string strHostRole = "backhost";
    char szFileData[2048] = {0};
    sprintf(szFileData, "%s", "<?xml version=\"1.0\" encoding=\"GB2312\"?>\r\n");
    sprintf(szFileData, "%s<HostRunInfo>\r\n",        szFileData);
    sprintf(szFileData, "%s\t<HostList>\r\n",       szFileData);
    
    
    g_configInfo.m_lock.lock();
    for (int i = 0; i < g_configInfo.hostInfoVect.size(); ++i)
    {
//        if (g_configInfo.hostInfoVect[i].chnlSet.size() <= 0)
//            continue;
        
//        std::vector<int> outputVect1(g_configInfo.hostInfoVect[i].chnlSet.size());
//        std::copy(g_configInfo.hostInfoVect[i].chnlSet.begin(), g_configInfo.hostInfoVect[i].chnlSet.end(), outputVect1.begin());
        
        std::string strHostRole = "backhost";
        std::vector<int> outputVect;        
        std::map<std::string, structWorkerChannelInfo>::iterator mapIter;
        //set<int> chnlSetNew;
        structWorkerChannelInfo tmpWorker;
        std::string strHostName = g_configInfo.strLocalIP;

        g_sockChannelLock.lock();
        mapIter = g_sockChannelMap.find(g_configInfo.hostInfoVect[i].strHostName);
        if (mapIter != g_sockChannelMap.end())
        {
            set<int> chnlSetLast(mapIter->second.chnlSet);
            outputVect.resize(chnlSetLast.size());
            std::copy(chnlSetLast.begin(), chnlSetLast.end(), outputVect.begin());
            
            if (g_configInfo.hostInfoVect[i].strHostName == strHostName)
            {
                strHostRole = "mainhost";
            }
            else
            {
                strHostRole = "backhost";
            }

            syslog(LOG_DEBUG, "hostname:%s", g_configInfo.hostInfoVect[i].strHostName.c_str());
            for (int i = 0; i < outputVect.size(); ++i)
            {
                syslog(LOG_DEBUG, "xxxx:%d", outputVect[i]);
            }
        }
        else
        {
            outputVect.resize(0);
            g_sockChannelLock.unlock(); 
            continue;
        }
        g_sockChannelLock.unlock();         
        
        
        sprintf(szFileData, "%s\t\t<HostInfo>\r\n\t\t\t<HostName>%s</HostName>\r\n",      szFileData,    g_configInfo.hostInfoVect[i].strHostName.c_str());
        for (int j = 0; j < outputVect.size(); ++j)
        {
            sprintf(szFileData, "%s\t\t\t<ChannelNo>%d</ChannelNo>\r\n",      szFileData,    outputVect[j]);
        }
        sprintf(szFileData, "%s\t\t\t<HostRole>%s</HostRole>\r\n",      szFileData,    strHostRole.c_str());
        sprintf(szFileData, "%s\t\t</HostInfo>\r\n",      szFileData);
    }
    g_configInfo.m_lock.unlock();
    sprintf(szFileData, "%s\t</HostList>\r\n",       szFileData);
    sprintf(szFileData, "%s</HostRunInfo>\r\n",       szFileData);
    
//    if (strlen(szFileData) < 150)
//        return;
    
    int nXmlLen = strlen(szFileData);
    if (nXmlLen < 150)
        return;
    syslog(LOG_DEBUG, "Write Xml nXmlLen:%d", nXmlLen);
    
//    for (int i = 0; i < g_configInfo.hostInfoVect.size(); ++i)
//    {
//        syslog(LOG_DEBUG, "%s", g_configInfo.hostInfoVect[i].strHostName.c_str());
//    }
    
    std::string filename = "/root/jiezisoft/jzmanager/HostInfo.xml";
    FILE* pFile = fopen(filename.c_str(), "wb");
    if (!pFile)
    {

    }  
    else
    {
        fwrite(szFileData, 1, strlen(szFileData), pFile);
        fclose(pFile);
    }    
    
    
}



void UninitWinSock()
{
#if	defined(_WIN32)||defined(_WIN64)	
	WSACleanup();
#endif
}

void	MyInitLock()
{
#if	defined(_WIN32)||defined(_WIN64)
	InitializeCriticalSection(&MyMutex);
#else
	pthread_mutex_init(&MyMutex,NULL);
#endif
}


void	MyInitLock2()
{
	pthread_mutex_init(&MyMutex2,NULL);
}


void	MyUninitLock()
{
#if	defined(_WIN32)||defined(_WIN64)
	DeleteCriticalSection(&MyMutex);
#else
	pthread_mutex_destroy(&MyMutex);
#endif
}

void	MyUninitLock2()
{
	pthread_mutex_destroy(&MyMutex2);
}

void MyLock()
{
#if	defined(_WIN32)||defined(_WIN64)
    EnterCriticalSection(&MyMutex);
#else
	pthread_mutex_lock(&MyMutex);
#endif
}

void MyLock2()
{
    pthread_mutex_lock(&MyMutex2);
}

void MyUnlock()
{
#if	defined(_WIN32)||defined(_WIN64)
    LeaveCriticalSection(&MyMutex);
#else
	pthread_mutex_unlock(&MyMutex);
#endif
}

void MyUnlock2()
{
    pthread_mutex_unlock(&MyMutex2);
}


void PrintUsage(const char * szAppName)
{
	syslog(LOG_DEBUG, "Usage : %s <LocalPort> <RemoteHostName> <RemotePort> \n",szAppName);
}

int CreateSocket()
{
	int nSocket;
	nSocket=(int)socket(PF_INET,SOCK_STREAM,0);
	return nSocket;
}

int CheckSocketResult(int nResult)
{
//	check result;
#if !defined(_WIN32)&&!defined(_WIN64)
	if(nResult==-1)
		return 0;
	else
		return 1;
#else
	if(nResult==SOCKET_ERROR)
		return 0;
	else
		return 1;
#endif
}

int CheckSocketValid(int nSocket)
{
//	check socket valid

#if !defined(_WIN32)&&!defined(_WIN64)
	if(nSocket==-1)
		return 0;
	else
		return 1;
#else
	if(((SOCKET)nSocket)==INVALID_SOCKET)
		return 0;
	else
		return 1;
#endif
}

int BindPort(int nSocket,int nPort)
{
	int rc;
	int optval=1;
#if	defined(_WIN32)||defined(_WIN64)
	rc=setsockopt((SOCKET)nSocket,SOL_SOCKET,SO_REUSEADDR,
		(const char *)&optval, sizeof(int));
#else
	rc=setsockopt(nSocket,SOL_SOCKET,SO_REUSEADDR,
		(const char *)&optval,sizeof(int));
#endif
	if(!CheckSocketResult(rc))
		return 0;
   	sockaddr_in name;
   	memset(&name,0,sizeof(sockaddr_in));
   	name.sin_family=AF_INET;
   	name.sin_port=htons((unsigned short)nPort);
	name.sin_addr.s_addr=INADDR_ANY;
#if	defined(_WIN32)||defined(_WIN64)
    rc=::bind((SOCKET)nSocket,(sockaddr *)&name,sizeof(sockaddr_in));
#else
    rc=::bind(nSocket,(sockaddr *)&name,sizeof(sockaddr_in));
#endif
	if(!CheckSocketResult(rc))
		return 0;
	return 1;
}

int ConnectSocket(int nSocket,const char * szHost,int nPort)
{
    hostent *pHost=NULL;
    hostent localHost;
    char pHostData[MAX_HOST_DATA];
    int h_errorno=0;
    int h_rc=gethostbyname_r(szHost,&localHost,pHostData,MAX_HOST_DATA,&pHost,&h_errorno);
    if((pHost==0)||(h_rc!=0))
    {
        return 0;
    }

    struct in_addr in;
    memcpy(&in.s_addr, pHost->h_addr_list[0],sizeof (in.s_addr));
    sockaddr_in name;
    memset(&name,0,sizeof(sockaddr_in));
    name.sin_family=AF_INET;
    name.sin_port=htons((unsigned short)nPort);
	name.sin_addr.s_addr=in.s_addr;
#if defined(_WIN32)||defined(_WIN64)
	int rc=connect((SOCKET)nSocket,(sockaddr *)&name,sizeof(sockaddr_in));
#else
	int rc=connect(nSocket,(sockaddr *)&name,sizeof(sockaddr_in));
#endif
	if(rc>=0)
		return 1;
	return 0;
}

int CloseSocket(int nSocket)
{
	int rc=0;
	if(!CheckSocketValid(nSocket))
	{
		return rc;
	}
#if	defined(_WIN32)||defined(_WIN64)
	shutdown((SOCKET)nSocket,SD_BOTH);
	closesocket((SOCKET)nSocket);
#else
	shutdown(nSocket,SHUT_RDWR);
	close(nSocket);
#endif
	rc=1;
	return rc;
}	

int ListenSocket(int nSocket,int nMaxQueue)
{
	int rc=0;
#if	defined(_WIN32)||defined(_WIN64)
	rc=listen((SOCKET)nSocket,nMaxQueue);
#else
	rc=listen(nSocket,nMaxQueue);
#endif
	return CheckSocketResult(rc);
}

void SetSocketNotBlock(int nSocket)
{
//	改变文件句柄为非阻塞模式
#if	defined(_WIN32)||defined(_WIN64)
	ULONG optval2=1;
	ioctlsocket((SOCKET)nSocket,FIONBIO,&optval2);
#else
    long fileattr;
    fileattr=fcntl(nSocket,F_GETFL);
    fcntl(nSocket,F_SETFL,fileattr|O_NDELAY);
#endif
}

void SysSleep(long nTime)
//	延时nTime毫秒，毫秒是千分之一秒
{
#if defined(_WIN32 )||defined(_WIN64)
//	windows 代码
	MSG msg;
	while(PeekMessage(&msg,NULL,0,0,PM_NOREMOVE))
	{
		if(GetMessage(&msg,NULL,0,0)!=-1)
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
	localTimeSpec.tv_sec=nTime/1000;
	localTimeSpec.tv_nsec=(nTime%1000)*1000000;
	nanosleep(&localTimeSpec,&localLeftSpec);
#endif
}

int CheckSocketError(int nResult)
{
//	检查非阻塞套接字错误
	if(nResult>0)
		return 0;
	if(nResult==0)
		return 1;

#if !defined(_WIN32)&&!defined(_WIN64)
	if(errno==EAGAIN)
		return 0;
	return 1;
#else
	if(WSAGetLastError()==WSAEWOULDBLOCK)
		return 0;
	else
		return 1;
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




int SocketWrite(int nSocket,char * pBuffer,int nLen,int nTimeout)
{
	int nOffset=0;
	int nWrite;
	int nLeft=nLen;
	int nLoop=0;
	int nTotal=0;
	int nNewTimeout=nTimeout*10;
	while((nLoop<=nNewTimeout)&&(nLeft>0))
	{
		nWrite=send(nSocket,pBuffer+nOffset,nLeft,0);
		if(nWrite==0)
		{
			return -1;
         }
#if defined(_WIN32)||defined(_WIN64)
		if(nWrite==SOCKET_ERROR)
		{
			if(WSAGetLastError()!=WSAEWOULDBLOCK)
			{	
				return -1;
			}
		}
#else
		if(nWrite==-1)
		{
	          if(errno!=EAGAIN)
			{
				return -1;
			}
		}
#endif
		if(nWrite<0)
		{
			return nWrite;
		}	
		nOffset+=nWrite;
		nLeft-=nWrite;
		nTotal+=nWrite;
		if(nLeft>0)
		{
//	延时100ms
			SysSleep(100);
		}
		nLoop++;
	}
	return nTotal;
}

int  SocketRead(int nSocket,void * pBuffer,int nLen)
{
	if(nSocket==-1)
		return -1;
	int len=0;
#if	defined(_WIN32)||defined(_WIN64)
	len=recv((SOCKET) nSocket,(char *)pBuffer,nLen,0);
#else
	len=recv(nSocket,(char *)pBuffer,nLen,0);
#endif

	if(len==0)
	{
		return -1;
	}
	if(len==-1)
	{
#if	defined(_WIN32)||defined(_WIn64)
		int localError=WSAGetLastError();
		if(localError==WSAEWOULDBLOCK)
			return 0;
		return -1;
#else
		if(errno==0)
			return -1;
		if(errno==EAGAIN)
			return 0;
#endif		
		return len;
	}
	if(len>0)
		return len;
	else
		return -1;
}

void EndClient(void * pParam)
{
    CLIENT_PARAM * localParam=(CLIENT_PARAM *) pParam;
	MyLock();
	localParam->nEndFlag=1;
	localParam->pServerParam->pClientSocket[localParam->nIndexOff]=-1;
	delete (CLIENT_PARAM *)localParam;
	MyUnlock();
#ifdef	_WIN32
	Sleep(50);
	_endthread();
    return ;
#else
	SysSleep(50);
	pthread_exit(NULL);
	return ;
#endif

}


void EndClient2(void * pParam)
{
    CLIENT_PARAM * localParam=(CLIENT_PARAM *) pParam;
    MyLock2();
    localParam->nEndFlag=1;
    localParam->pServerParam->pClientSocket[localParam->nIndexOff]=-1;
    delete (CLIENT_PARAM *)localParam;
    MyUnlock2();
    SysSleep(50);
    pthread_exit(NULL);
    return ;
}

void * ClientProc(void * pParam)
{
//	客户端处理线程,把接收的数据发送给目标机器,并把目标机器返回的数据返回到客户端
    CLIENT_PARAM * localParam=(CLIENT_PARAM *) pParam;
	localParam->nEndFlag=0;
	int nSocket=localParam->nSocket;
        
        pthread_t threadID =  pthread_self();
        
        std::string strHostName = localParam->szClientName;

	int nLen=0;
	int nLoopTotal=0;
	int nLoopMax=20*300;	//	300 秒判断选循环
#define	nMaxLen 0x1000
	char pBuffer[nMaxLen+1]     = {0};
	char pNewBuffer[nMaxLen+1]  = {0};

	int nSocketErrorFlag=0;

	int nNewLen=0;
	while(!GlobalStopFlag&&!nSocketErrorFlag)
	{
		nLoopTotal++;
                
                
                
		//nLen=SocketRead(nSocket, pBuffer, nMaxLen);
                //sleep(1);
                
                // 2016-6-12
                //if (ReceiveTimer(nSocket) > 0)
                {                
    //	读取客户端数据
                    nLen=SocketRead(nSocket, pBuffer, nMaxLen);
                    syslog(LOG_DEBUG, "=read len:%d,%d,%d\n", nLen, __LINE__,nSocket);
                    if (nLen>0)
                    {
                            pBuffer[nLen]=0;
                            nLoopTotal=0;
                            
                            // ========================================================================
                            structThreadInfo tmpThreadInfo;
                            tmpThreadInfo.threadId = threadID;
                            std::map<pthread_t, structThreadInfo>::iterator mapIter2;
                            g_threadMapLock.lock();
                            mapIter2 = g_threadMap.find(threadID);
                            if (mapIter2 != g_threadMap.end())
                            {
                                    //mapIter->second.nSendTime = time(0);
                                    mapIter2->second.nRecvTime = time(0);
                            }
                            else
                            {
                                    //tmpThreadInfo.nSendTime     = time(0);
                                    tmpThreadInfo.nRecvTime     = time(0);
                                    g_threadMap.insert(make_pair(threadID, tmpThreadInfo));
                            }                        
                            g_threadMapLock.unlock();                            
                            // ========================================================================
                            
                            
                            
                            
                            
                            

                            // ===========start
                            int index     = 0;
                            int nTotalLen = 0;
                            memcpy(&nTotalLen, pBuffer +index, 4);
                            index += 4;

                            int  nType    = 0;
                            memcpy(&nType, pBuffer + index, 4);
                            index += 4;

                            int nCount    = 0;
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

                            std::map<std::string, structWorkerChannelInfo>::iterator mapIter;
                            set<int> chnlSetNew;
                            structWorkerChannelInfo tmpWorker;

                            g_sockChannelLock.lock();
                            mapIter = g_sockChannelMap.find(strHostName);
                            if (mapIter != g_sockChannelMap.end())
                            {
                                if (strHostName == "192.168.1.111")
                                {
                                    syslog(LOG_DEBUG, "testXX:%d", mapIter->second.chnlSet.size());
                                }
                                
                                //syslog(LOG_DEBUG, "11111111111111111111111111111111111:%s", strHostName.c_str());
                                mapIter->second.nTime = time(0);
                                set<int> chnlSetLast(mapIter->second.chnlSet);

                                int nCurSize    = chnlSet.size();
                                int nLastSize   = chnlSetLast.size();
                                if (nLastSize == nCurSize)
                                {
                                    chnlSetNew              = chnlSet;
                                }
                                else if (nLastSize > nCurSize)
                                {
                                    chnlSetNew              = chnlSetLast;
                                }
                                else
                                {
                                    chnlSetNew              = chnlSet;
                                    mapIter->second.chnlSet = chnlSet;
                                    
                                    // 2016-6-20
                                    //chnlSetNew              = chnlSetLast;
                                }
                            }
                            else
                            {
                                //syslog(LOG_DEBUG, "0000000000000000000000000000000000:%s", strHostName.c_str());
                                tmpWorker.nSocket   = nSocket;
                                tmpWorker.nTime     = time(0);
                                tmpWorker.chnlSet   = chnlSet;
                                g_sockChannelMap.insert(make_pair(strHostName, tmpWorker));
                                chnlSetNew          = chnlSet;
                                
                                syslog(LOG_DEBUG, "insert one\n");
                            }                        
                            g_sockChannelLock.unlock();


//                            // ===============================================start
//                            for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end(); ++mapIter)
//                            {
//                                syslog(LOG_DEBUG, "register host:%s\n", mapIter->first.c_str());
//                            }
//                            // ===============================================end


                            // ===========rebuild start
                            index     = 0;
                            nTotalLen = 0;

                            memcpy(pNewBuffer +index, &nTotalLen, 4);
                            index += 4;

                            nType    = 2;
                            memcpy(pNewBuffer +index, &nType, 4);
                            index += 4;

                            nCount    = chnlSetNew.size();
                            memcpy(pNewBuffer +index, &nCount, 4);
                            index += 4;


                            set<int>::iterator it;
                            for (it = chnlSetNew.begin(); it != chnlSetNew.end(); ++it)
                            {
                                int chnlNo = *it;
                                memcpy(pNewBuffer +index, &chnlNo, 4);
                                index += 4;
                            }
                            nTotalLen = nNewLen = index;
                            memcpy(pNewBuffer, &nTotalLen, 4);                        
                            // rebuild ===========end                        


                            nLen=SocketWrite(nSocket,pNewBuffer,nNewLen,30);
                            if(nLen<0)	//	断开
                                    break;
                            syslog(LOG_DEBUG, "=write len:%d,%d,%d\n", nLen, __LINE__,nSocket);
                            if (nLen > 0)
                            {
                                // ========================================================================
                                structThreadInfo tmpThreadInfo;
                                tmpThreadInfo.threadId = threadID;
                                std::map<pthread_t, structThreadInfo>::iterator mapIter2;
                                g_threadMapLock.lock();
                                mapIter2 = g_threadMap.find(threadID);
                                if (mapIter2 != g_threadMap.end())
                                {
                                        mapIter2->second.nSendTime = time(0);
                                        //mapIter->second.nRecvTime = time(0);
                                }
//                                else
//                                {
//                                        //tmpThreadInfo.nSendTime     = time(0);
//                                        tmpThreadInfo.nRecvTime     = time(0);
//                                        g_threadMap.insert(make_pair(threadID, tmpThreadInfo));
//                                }                        
                                g_threadMapLock.unlock();                            
                                // ========================================================================
                            }
                            
                            
                            
                            // 改为触发方式 

                            if (g_configInfo.strHostRole == "backhost")
                            {
                                //if (nCount > 0)
                                {
                                    g_configInfo.m_lock.lock();
                                    g_configInfo.strMaster = "yes";
                                    g_configInfo.m_lock.unlock();
                                    //g_sem.post();
                                    
                                    // read hostinfo
                                    //ReadConfigInfo4(g_configInfo);
                                }
                            }
                    }
                    if(nLen<0)
                    {
                        SysSleep(50);
                        //	读断开
                        break;
                    }                    
                }
                
		if((nSocketErrorFlag==0)&&(nLoopTotal>0))
		{
			SysSleep(50);
			if(nLoopTotal>=nLoopMax)
			{
				nLoopTotal=0;
			}
		}
	}
        
        // avoid pthread_cancel cause SIGSEGV
        int nXX = 0;        
        g_threadMapLock.lock();
        nXX = g_threadMap.erase(threadID);
        g_threadMapLock.unlock();        
        //==============================================
        
        
	CloseSocket(nSocket);
	EndClient(pParam);
	SysSleep(50);
	pthread_exit(NULL);
	return NULL;
}

void SyncConfigProc(void)
{
	char szFileData[2048] = {0};
	int nType = 24;
        
        std::string strTmpMaster = "";
        g_configInfo.m_lock.lock();
        strTmpMaster = g_configInfo.strMaster;
        g_configInfo.m_lock.unlock();
        
	if (strTmpMaster == "yes")
	{
		// sync
		std::string strFileNameNew = "/root/jiezisoft/jzmanager/JZManagerConfig.xml";
		unsigned long filesize = -1;      
		struct stat statbuff;  
		if(::stat(strFileNameNew.c_str(), &statbuff) < 0)
		{  
			return;
		}
		else
		{  
			filesize = statbuff.st_size;  
		}      

		FILE* pFile = fopen(strFileNameNew.c_str(), "rb");
		if (!pFile)
		{
				return;
		}     
		fread(szFileData, 1, filesize, pFile);
		fclose(pFile);        

		int nHostSize = g_configInfo.hostInfoVect.size();
		
		for (int n = 0; n < g_configInfo.hostInfoVect.size(); ++n)
		{
			if (g_configInfo.strLocalIP != g_configInfo.hostInfoVect[n].strHostName)
			{
				int nLen        = 0;
				int nNewLen     = 0;
				int nNewSocket  = -1;
				char pNewBuffer[nMaxLen + 1] = {0};

				nNewSocket = CreateSocket();
				if (nNewSocket == -1)
				{
					//	不能建立套接字，直接返回
					nNewSocket = CreateSocket();
					if (nNewSocket == -1)
					{
						continue;
					}
				}

				int GlobalRemotePort = atoi(g_configInfo.strLocalPortHandle2.c_str());
				if (ConnectSocket(nNewSocket, g_configInfo.hostInfoVect[n].strHostName.c_str(), GlobalRemotePort) <= 0)
				{
					CloseSocket(nNewSocket);
					continue;
				} 

				// ===========first start
				int index = 0;
				int nTotalLen = 0;

				memcpy(pNewBuffer + index, &nTotalLen, 4);
				index += 4;
				
				memcpy(pNewBuffer + index, &nType, 4);
				index += 4;

				int nFileSize = filesize;
				memcpy(pNewBuffer + index, &nFileSize, 4);
				index += 4;        
				
				memcpy(pNewBuffer + index, szFileData, filesize);
				index += filesize;

				nNewLen = nTotalLen = index;
				memcpy(pNewBuffer, &nTotalLen, 4);

				nLen = SocketWrite(nNewSocket, pNewBuffer, nNewLen, 30);
				syslog(LOG_DEBUG, "write file len:%d,%d\n", nLen, __LINE__);
				CloseSocket(nNewSocket);
			}				
		}
	}

	return;
}

void * ClientProcForSync(void * pParam)
{
//	客户端处理线程,把接收的数据发送给目标机器,并把目标机器返回的数据返回到客户端
    CLIENT_PARAM * localParam=(CLIENT_PARAM *) pParam;
	localParam->nEndFlag=0;
	int nSocket=localParam->nSocket;
        
        std::string strHostName = localParam->szClientName;

	int nLen=0;

	int nLoopMax=20*300;	//	300 秒判断选循环
#define	nMaxLen 0x1000
	char pBuffer[nMaxLen+1]     = {0};
	char pNewBuffer[nMaxLen+1]  = {0};

	int nSocketErrorFlag=0;

	int nNewLen=0;
        nLen=SocketRead(nSocket, pBuffer, nMaxLen);

        // 读取客户端数据
        if (nLen>0)
        {
            pBuffer[nLen]=0;
            
            char szTmpData[5 + 1] = {0};
            std::string strLogData = "Recv Data:";
            for (size_t i = 0; i < nLen; ++i)
            {
                    memset(szTmpData, 0x00, 5);
                    sprintf(szTmpData, "%02X ", (BYTE)pBuffer[i]);
                    strLogData += szTmpData;
            }
            syslog(LOG_DEBUG, "%s\n", strLogData.c_str());            
            
            
            
            
            

            // ===========start
            int index     = 0;
            int nTotalLen = 0;
            memcpy(&nTotalLen, pBuffer +index, 4);
            index += 4;

            int  nType    = 0;
            memcpy(&nType, pBuffer + index, 4);
            index += 4;

            
         std::string strTmpMaster = "";
        g_configInfo.m_lock.lock();
        strTmpMaster = g_configInfo.strMaster;
        g_configInfo.m_lock.unlock();
        syslog(LOG_DEBUG, "strTmpMaster:%s\n", strTmpMaster.c_str());   
            // ===========end
            if (nType == 8 && strTmpMaster == "yes")
            {
                // ===========rebuild start
                index     = 0;
                nTotalLen = 0;

                memcpy(pNewBuffer +index, &nTotalLen, 4);
                index += 4;

                nType    = 9;
                memcpy(pNewBuffer +index, &nType, 4);
                index += 4;

                std::string strIP = g_configInfo.strLocalIP;
                int nTmp    = strIP.size();
                memcpy(pNewBuffer +index, &nTmp, 4);
                index += 4;
                
                memcpy(pNewBuffer +index, strIP.c_str(), nTmp);
                index += nTmp;

                nTotalLen = nNewLen = index;
                memcpy(pNewBuffer, &nTotalLen, 4);                        
                // rebuild ===========end                        

                nLen=SocketWrite(nSocket,pNewBuffer,nNewLen,30);                       
            }
            else if (nType == 8 && strTmpMaster == "no")
            {
                // ===========rebuild start
                index     = 0;
                nTotalLen = 0;

                memcpy(pNewBuffer +index, &nTotalLen, 4);
                index += 4;

                nType    = 9;
                memcpy(pNewBuffer +index, &nType, 4);
                index += 4;

                std::string strIP = "";
                int nTmp    = strIP.size();
                memcpy(pNewBuffer +index, &nTmp, 4);
                index += 4;
                
                if (nTmp > 0)
                {
                    memcpy(pNewBuffer +index, strIP.c_str(), nTmp);
                    index += nTmp;
                }

                nTotalLen = nNewLen = index;
                memcpy(pNewBuffer, &nTotalLen, 4);                        
                // rebuild ===========end                        

                nLen=SocketWrite(nSocket,pNewBuffer,nNewLen,30);                  
            }
            else if (nType == 10 && strTmpMaster == "no")
            {
                int index     = 8;
                int nFileSize = 0;
                memcpy(&nFileSize, pBuffer +index, 4);
                index += 4;

                char *pFileBuff = new char[nFileSize + 1];
                
                memcpy(pFileBuff, pBuffer + index, nFileSize);
                pFileBuff[nFileSize] = '\0';  
                
                std::string strFileNameNew = "/root/jiezisoft/jzmanager/HostInfo.xml";
                FILE* pFile = fopen(strFileNameNew.c_str(), "wb");
                if (!pFile)
                {

                }  
                else
                {
                    fwrite(pFileBuff, 1, nFileSize, pFile);
                    fclose(pFile);
                }
                delete []pFileBuff;
                //ReadConfigInfo3(g_configInfo);
            }
            else if (nType == 20 && strTmpMaster == "yes")
            {
//                // auto channel---> man channel
//                pBuffer[nLen] = 0;
//
//                // ===========start
//                index = 0;
//                nTotalLen = 0;
//                memcpy(&nTotalLen, pBuffer + index, 4);
//                index += 4;
//
//                nType = 0;
//                memcpy(&nType, pBuffer + index, 4);
//                index += 4;
//
//                int nCount = 0;
//                memcpy(&nCount, pBuffer + index, 4);
//                index += 4;
//
//                set<int> chnlSet;
//                for (int i = 0; i < nCount; ++i)
//                {
//                    int chnlNo = 0;
//                    memcpy(&chnlNo, pBuffer + index, 4);
//                    index += 4;
//                    chnlSet.insert(chnlNo);
//                }
//                
//                int nIPLen = 0;
//                memcpy(&nIPLen, pBuffer + index, 4);
//                index += 4;
//                
//                std::string strIP = "";
//                char szIPAddr[50] = {0};
//                memcpy(szIPAddr, pBuffer + index, nIPLen);
//                index += nIPLen;
//                strIP = szIPAddr;                  
//                
//                // ===========end                
//                std::vector<int> outputVect1(chnlSet.size());
//                std::copy(chnlSet.begin(), chnlSet.end(), outputVect1.begin());                
//                
//                
//                syslog(LOG_DEBUG, "lock begin!");
//                g_configInfo.m_lock.lock();
//                for (int i = 0; i < outputVect1.size(); ++i)
//                {
//                    for (int j = 0; j < g_configInfo.hostInfoVect.size(); ++j)
//                    {
//                         g_configInfo.hostInfoVect[j].chnlSet.erase(outputVect1[i]);
//                    }
//                }
//                g_configInfo.m_lock.unlock();
//                 syslog(LOG_DEBUG, "lock end!");
//                // cache
//                
//                //g_sockChannelMap modifys
//                 syslog(LOG_DEBUG, "lock2 begin!");
//                std::map<std::string, structWorkerChannelInfo>::iterator mapIter;
//                
//                g_sockChannelLock.lock();
//                for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();++mapIter)
//                {
//                    structWorkerChannelInfo &tmpWorker   = mapIter->second;
//                    for (int i = 0; i < outputVect1.size(); ++i)
//                    {
//                        tmpWorker.chnlSet.erase(outputVect1[i]);
//                    }                       
//                }      
//                g_sockChannelLock.unlock();                
//                syslog(LOG_DEBUG, "lock2 end!");
//                // sync other computer
//                WriteConfigFile();
//                
//                SyncConfigProc();
                 
            }
            else if (nType == 22 && strTmpMaster == "yes")
            {
//                // man channel---> auto channel
//                
//                pBuffer[nLen] = 0;
//
//                // ===========start
//                index = 0;
//                nTotalLen = 0;
//                memcpy(&nTotalLen, pBuffer + index, 4);
//                index += 4;
//
//                nType = 0;
//                memcpy(&nType, pBuffer + index, 4);
//                index += 4;
//
//                int nCount = 0;
//                memcpy(&nCount, pBuffer + index, 4);
//                index += 4;
//
//                set<int> chnlSet;
//                for (int i = 0; i < nCount; ++i)
//                {
//                    int chnlNo = 0;
//                    memcpy(&chnlNo, pBuffer + index, 4);
//                    index += 4;
//                    chnlSet.insert(chnlNo);
//                }
//                
//                int nIPLen = 0;
//                memcpy(&nIPLen, pBuffer + index, 4);
//                index += 4;
//                
//                std::string strIP = "";
//                char szIPAddr[50] = {0};
//                memcpy(szIPAddr, pBuffer + index, nIPLen);
//                index += nIPLen;
//                strIP = szIPAddr;           
//                
//                
//                // ===========end                
//                std::vector<int> outputVect1(chnlSet.size());
//                std::copy(chnlSet.begin(), chnlSet.end(), outputVect1.begin());                
//                
//                g_configInfo.m_lock.lock();
//                for (int i = 0; i < outputVect1.size(); ++i)
//                {
//                    for (int j = 0; j < g_configInfo.hostInfoVect.size(); ++j)
//                    {
//                         if (g_configInfo.hostInfoVect[j].strHostName == strIP)
//                         {
//                             g_configInfo.hostInfoVect[j].chnlSet.insert(outputVect1[i]);
//                         }
//                    }
//                }    
//                g_configInfo.m_lock.unlock();
//                
//                // cache
////========================================================================================
//                std::map<std::string, structWorkerChannelInfo>::iterator mapIter;
//                g_sockChannelLock.lock();
//                mapIter = g_sockChannelMap.find(strIP);
//                if (mapIter != g_sockChannelMap.end())
//                {
//                    set<int> &chnlSetLast = mapIter->second.chnlSet;
//                    for (int i = 0; i < outputVect1.size(); ++i)
//                        chnlSetLast.insert(outputVect1[i]);
//                }                       
//                g_sockChannelLock.unlock();
//                
//                // sync other computer
//                WriteConfigFile();
//                SyncConfigProc();
            }
            else if (nType == 24 && strTmpMaster == "no")
            {
                int index     = 8;
                int nFileSize = 0;
                memcpy(&nFileSize, pBuffer +index, 4);
                index += 4;

                char *pFileBuff = new char[nFileSize + 1];
                
                memcpy(pFileBuff, pBuffer + index, nFileSize);
                pFileBuff[nFileSize] = '\0';  
                
                std::string strFileNameNew = "/root/jiezisoft/jzmanager/JZManagerConfig.xml";
                FILE* pFile = fopen(strFileNameNew.c_str(), "wb");
                if (!pFile)
                {

                }  
                else
                {
                    fwrite(pFileBuff, 1, nFileSize, pFile);
                    fclose(pFile);
                }
                delete []pFileBuff;
                
                // read 2
                ReadConfigInfo2(g_configInfo);
            }
            else if (nType == 26 && strTmpMaster == "yes")
            {
                syslog(LOG_DEBUG, "recv type is:%d,%d,%s", 26, __LINE__, strTmpMaster.c_str());
                    char szFileData[2048] = {0};
                    std::string		strFileNameNew	= "/root/jiezisoft/jzmanager/HostInfo.xml";
                    unsigned long	filesize		= -1;      
                    struct stat statbuff;  
                    if(::stat(strFileNameNew.c_str(), &statbuff) < 0)
                    {  
                        syslog(LOG_DEBUG, "stat file error!:%d", __LINE__);
                            CloseSocket(nSocket);
                            EndClient2(pParam);
                            SysSleep(50);
                            pthread_exit(NULL);
                            return NULL;
                    }
                    else
                    {  
                            filesize = statbuff.st_size;  
                    }    

                    FILE* pFile = fopen(strFileNameNew.c_str(), "rb");
                    if (!pFile)
                    {
                        syslog(LOG_DEBUG, "fopen file error!:%d", __LINE__);
                            CloseSocket(nSocket);
                            EndClient2(pParam);
                            SysSleep(50);
                            pthread_exit(NULL);
                            return NULL;
                    }     
                    fread(szFileData, 1, filesize, pFile);
                    fclose(pFile);   
                    szFileData[filesize] = '\0';   


                    // ===========rebuild start
                    index     = 0;
                    nTotalLen = 0;

                    memcpy(pNewBuffer +index, &nTotalLen, 4);
                    index += 4;

                    nType    = 27;
                    memcpy(pNewBuffer +index, &nType, 4);
                    index += 4;

                    int nTmp    = filesize;
                    memcpy(pNewBuffer +index, &nTmp, 4);
                    index += 4;

                    if (nTmp > 0)
                    {
                            memcpy(pNewBuffer +index, szFileData, nTmp);
                            index += nTmp;
                    }

                    nTotalLen = nNewLen = index;
                    memcpy(pNewBuffer, &nTotalLen, 4);                        
                    // rebuild ===========end                        

                    nLen=SocketWrite(nSocket,pNewBuffer,nNewLen,30);   
            }
            else if (nType == 28 && strTmpMaster == "no")
            {
                    int index     = 8;
                    int nFileSize = 0;
                    memcpy(&nFileSize, pBuffer +index, 4);
                    index += 4;
                    
                    if (nFileSize <= 150)
                    {
                        goto exit1;
                    }

                    char *pFileBuff = new char[nFileSize + 1];

                    memcpy(pFileBuff, pBuffer + index, nFileSize);
                    pFileBuff[nFileSize] = '\0';  

                    std::string strFileNameNew = "/root/jiezisoft/jzmanager/HostInfo.xml";
                    FILE* pFile = fopen(strFileNameNew.c_str(), "wb");
                    if (!pFile)
                    {

                    }  
                    else
                    {
                            fwrite(pFileBuff, 1, nFileSize, pFile);
                            fclose(pFile);
                    }
                    delete []pFileBuff;
                    
                    syslog(LOG_DEBUG, "xml:%s,%d,%d", pFileBuff, nFileSize, __LINE__);

                    // update config
                    ReadConfigInfo3(g_configInfo);
            }            
        }

exit1:        
	CloseSocket(nSocket);
	EndClient2(pParam);
	SysSleep(50);
	pthread_exit(NULL);
	return NULL;
}


void * AcceptProc(void * pParam)
{
    SERVER_PARAM * localParam=(SERVER_PARAM *) pParam;
    int s;
    unsigned int Len;
    sockaddr_in inAddr;
	unsigned long nIndex;
	char pClientName[256] = {0};
	unsigned int nCurIndex=(unsigned int)-1;
	unsigned int nIndexOff=0;
	CLIENT_PARAM * localClientParam=NULL;

	localParam->nEndFlag=0;
	for(nIndex=0;nIndex<MAX_SOCKET;nIndex++)
	{
		localParam->pClientSocket[nIndex]=-1;
	}
	while(!GlobalStopFlag)
	{
            Len=sizeof(sockaddr_in);
            inAddr=localParam->ServerSockaddr;

            s=accept(localParam->nSocket,(sockaddr *)(&inAddr), (socklen_t *)&Len);


		if(s==0)
			break;	//	socket error
		if(s==-1)
		{
			if(CheckSocketError(s))
				break;	//	socket error
			SysSleep(50);
			continue;
		}
		MyLock();
		strcpy(pClientName,inet_ntoa(inAddr.sin_addr));
                
//                struct in_addr *hipaddr = new struct in_addr;
//                bzero(hipaddr, sizeof(struct in_addr));
//                if (!inet_aton(pClientName, hipaddr))
//                {
//                        delete hipaddr;
//			CloseSocket(s);
//			SysSleep(50);
//			continue;                      
//                }
                
//                // 20160608
//                struct hostent *hptr = NULL;
//                hptr = gethostbyaddr(&(inAddr.sin_addr), sizeof(struct in_addr), AF_INET);
//                if (hptr != NULL)
//                {
//                    memset(pClientName, 0x00, sizeof(pClientName));
//                    strcpy(pClientName, hptr->h_name);
//                    
//                    //delete hipaddr;
//                }
//                else
//                {
//                    syslog(LOG_DEBUG, "%d\n", h_errno);
//                    //HOST_NOT_FOUND
//                    
//                    
//                        //delete hipaddr;
//			CloseSocket(s);
//			SysSleep(50);
//			continue;                    
//                }
                
                
                
//	在Linux下,inet_ntoa()函数是线程不安全的
		MyUnlock();
		//SetSocketNotBlock(s);
		nIndexOff=nCurIndex;
		MyLock();
//	查找空闲的套接字位置,
//	因为客户端的套接字线程处理程序也要操作此数据,因此需要加锁
		for(nIndex=0;nIndex<MAX_SOCKET;nIndex++)
		{
			nIndexOff++;
			nIndexOff%=MAX_SOCKET;
			if(localParam->pClientSocket[nIndexOff]==-1)
			{
//	此偏移量处存放新建立的套接字
				localParam->pClientSocket[nIndexOff]=s;
				nCurIndex=nIndexOff;
				break;
			}
		}
		MyUnlock();
		if(nIndex>=MAX_SOCKET)
		{
//	没有找到存放套接字的偏移量,太多的连接
			CloseSocket(s);
			SysSleep(50);
			continue;
		}
		localClientParam                = new CLIENT_PARAM;
		localClientParam->nEndFlag      = 1;
		localClientParam->nIndexOff     = nCurIndex;
		localClientParam->nSocket       = s;
		localClientParam->pServerParam  = localParam;
		memset(localClientParam->szClientName, 0, 256);
		strcpy(localClientParam->szClientName, pClientName);
		int nThreadErr = 0;

#if	defined(_WIN32)||defined(_WIN64)
		if(_beginthread(ClientProc,NULL,localClientParam)<=1)
			nThreadErr=1;
#else
		pthread_t localThreadId;
        nThreadErr=pthread_create(&localThreadId,NULL,
            (THREADFUNC)ClientProc,localClientParam);
		if(nThreadErr==0)
			pthread_detach(localThreadId);
//	释放线程私有数据,不必等待pthread_join();
#endif
		if(nThreadErr)
		{
//	建立线程失败
			MyLock();
			localParam->pClientSocket[nIndexOff]=-1;
			MyUnlock();
			continue;
		}
	}
	int nUsedSocket = MAX_SOCKET;
	while (nUsedSocket > 0)
	{
		nUsedSocket=0;
		SysSleep(50);
		MyLock();
		for(nIndex=0;nIndex<MAX_SOCKET;nIndex++)
		{
			if(localParam->pClientSocket[nIndex]!=-1)
			{
				nUsedSocket=1;
				break;
			}
		}
		MyUnlock();
	}

	SysSleep(50);
	localParam->nEndFlag    = 1;
	pthread_exit(NULL);
	return NULL;
}

void * AcceptProcForSync(void * pParam)
{
    SERVER_PARAM * localParam=(SERVER_PARAM *) pParam;
    int s;
    unsigned int Len;
    sockaddr_in inAddr;
    unsigned long nIndex;
    char pClientName[256] = {0};
    unsigned int nCurIndex=(unsigned int)-1;
    unsigned int nIndexOff=0;
    CLIENT_PARAM * localClientParam=NULL;

	localParam->nEndFlag=0;
	for(nIndex=0;nIndex<MAX_SOCKET;nIndex++)
	{
		localParam->pClientSocket[nIndex]=-1;
	}
	while(!GlobalStopFlag)
	{
            Len=sizeof(sockaddr_in);
            inAddr=localParam->ServerSockaddr;

            s=accept(localParam->nSocket,(sockaddr *)(&inAddr), (socklen_t *)&Len);


		if(s==0)
			break;	//	socket error
		if(s==-1)
		{
			if(CheckSocketError(s))
				break;	//	socket error
			SysSleep(50);
			continue;
		}
		MyLock2();
		strcpy(pClientName,inet_ntoa(inAddr.sin_addr));
 
//	在Linux下,inet_ntoa()函数是线程不安全的
		MyUnlock2();
		//SetSocketNotBlock(s);
		nIndexOff=nCurIndex;
		MyLock2();
//	查找空闲的套接字位置,
//	因为客户端的套接字线程处理程序也要操作此数据,因此需要加锁
		for(nIndex=0;nIndex<MAX_SOCKET;nIndex++)
		{
			nIndexOff++;
			nIndexOff%=MAX_SOCKET;
			if(localParam->pClientSocket[nIndexOff]==-1)
			{
//	此偏移量处存放新建立的套接字
				localParam->pClientSocket[nIndexOff]=s;
				nCurIndex=nIndexOff;
				break;
			}
		}
		MyUnlock2();
		if(nIndex>=MAX_SOCKET)
		{
//	没有找到存放套接字的偏移量,太多的连接
			CloseSocket(s);
			SysSleep(50);
			continue;
		}
		localClientParam                = new CLIENT_PARAM;
		localClientParam->nEndFlag      = 1;
		localClientParam->nIndexOff     = nCurIndex;
		localClientParam->nSocket       = s;
		localClientParam->pServerParam  = localParam;
		memset(localClientParam->szClientName, 0, 256);
		strcpy(localClientParam->szClientName, pClientName);
		int nThreadErr = 0;

		pthread_t localThreadId;
                nThreadErr=pthread_create(&localThreadId,NULL, (THREADFUNC)ClientProcForSync,localClientParam);
		if(nThreadErr==0)
			pthread_detach(localThreadId);
//	释放线程私有数据,不必等待pthread_join();

		if(nThreadErr)
		{
//	建立线程失败
			MyLock2();
			localParam->pClientSocket[nIndexOff]=-1;
			MyUnlock2();
			continue;
		}
	}
	int nUsedSocket = MAX_SOCKET;
	while (nUsedSocket > 0)
	{
		nUsedSocket=0;
		SysSleep(50);
		MyLock2();
		for(nIndex=0;nIndex<MAX_SOCKET;nIndex++)
		{
                    if(localParam->pClientSocket[nIndex]!=-1)
                    {
                            nUsedSocket=1;
                            break;
                    }
		}
		MyUnlock2();
	}

	SysSleep(50);
	localParam->nEndFlag    = 1;
	pthread_exit(NULL);
	return NULL;
}


void * VoteProc(void * pParam)
{
    int nResult = 0;
    while(!GlobalStopFlag)
    {
        std::string strTmpMaster = "";
        g_configInfo.m_lock.lock();
        strTmpMaster = g_configInfo.strMaster;
        g_configInfo.m_lock.unlock();
        
        
        
        //if (g_configInfo.strMaster == "yes" && nResult == 0)
        if (strTmpMaster == "yes" && nResult == 0)
        {
            sleep(15);
            //sleep(30 * 2);
            //SysSleep(20 * 1000);
            std::map<std::string, structWorkerChannelInfo>::iterator mapIter;
            set<int> chnlSetNew;
            structWorkerChannelInfo tmpWorker;

            g_sockChannelLock.lock();
            for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();)
            {
                int nLastTime   = mapIter->second.nTime;
                int nNowTime    = time(0);
                int nDiffTime   = nNowTime - nLastTime;
                if (nDiffTime > 15)
                {
                    tmpWorker   = mapIter->second;
                    g_sockChannelMap.erase(mapIter);
                    
                    syslog(LOG_DEBUG, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>15:%d", tmpWorker.nSocket);
                    
                    break;
                } 
                else
                {
                    mapIter++;
                }
            }      
            g_sockChannelLock.unlock();
            //sleep(1);
            SysSleep(50);

            // ===========================================================================================
//            g_sockChannelLock.lock();
//            set<int> chnlSetLast(tmpWorker.chnlSet);
//            set<int>::iterator it;
//            for (it = chnlSetLast.begin(),mapIter = g_sockChannelMap.begin(); it != chnlSetLast.end(); ++it)
//            {
//                int chnlNo = *it;
//                //for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();mapIter++)
//                if (mapIter != g_sockChannelMap.end())
//                {
//                    mapIter->second.chnlSet.insert(chnlNo);
//                    mapIter++;
//                } 
//                else
//                {
//                    mapIter = g_sockChannelMap.begin();
//                    if (mapIter != g_sockChannelMap.end())
//                    {
//                        mapIter->second.chnlSet.insert(chnlNo);
//                        mapIter++;
//                    }
//                }
//            }            
//            g_sockChannelLock.unlock(); 
            // ==================================================================================================
            
            // average alloc
            g_sockChannelLock.lock();
            set<int> chnlSetLast(tmpWorker.chnlSet);
            set<int>::iterator it;
            for (it = chnlSetLast.begin(); it != chnlSetLast.end(); ++it)
            {
                
                int nMin = -1;
                std::map<std::string, structWorkerChannelInfo>::iterator mapIter2   = g_sockChannelMap.begin();
                std::map<std::string, structWorkerChannelInfo>::iterator mapIterMin = g_sockChannelMap.begin();
                for (mapIter2 = g_sockChannelMap.begin(); mapIter2 != g_sockChannelMap.end(); ++mapIter2)
                {
                        int nElemSize = mapIter2->second.chnlSet.size();
                        
                        if (nElemSize == 0)
                        {
                                nMin = nElemSize;
                                mapIterMin = mapIter2;   
                                break;
                        }
                        else if (nMin == -1)
                        {
                                nMin = nElemSize;
                                mapIterMin = mapIter2;                            
                        }
                        else if (nElemSize < nMin)
                        {
                                nMin = nElemSize;
                                mapIterMin = mapIter2;
                        }
                } 

                int chnlNo = *it;
                syslog(LOG_DEBUG, "====================N%d", chnlNo);
                if (mapIterMin != g_sockChannelMap.end())
                {
                    mapIterMin->second.chnlSet.insert(chnlNo);
                    syslog(LOG_DEBUG, "insert two\n");
                }
            }            
            g_sockChannelLock.unlock();             
            
            // 处理未注册的通道
            if (g_configInfo.strHostRole == "backhost")
            {
//                if (g_sockChannelMap.size() <= 0)
//                {
//                    sleep(20);
//                    continue;
//                }
                sleep(15);
                
                
                std::set<int> regiterChannelSet;
                std::set<int> allChannelSet;
                g_sockChannelLock.lock();
                for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();mapIter++)
                {
                    structWorkerChannelInfo tmpWorkerInfo;
                    tmpWorkerInfo   = mapIter->second;
                    set<int> chnlSetLast(tmpWorkerInfo.chnlSet);
                    
                    for (set<int>::iterator it = chnlSetLast.begin(); it != chnlSetLast.end(); ++it)
                    {
                        regiterChannelSet.insert(*it);
                    }
                }      
                g_sockChannelLock.unlock();   
                
                
                g_configInfo.m_lock.lock();
                for (int i = 0; i != g_configInfo.hostInfoVect.size(); ++i)
                {
                    for (set<int>::iterator it = g_configInfo.hostInfoVect[i].chnlSet.begin(); it != g_configInfo.hostInfoVect[i].chnlSet.end(); ++it)
                    {
                        allChannelSet.insert(*it);
                    }                    
                }
                g_configInfo.m_lock.unlock();
                
                set<int> diffSet;
                set_difference(allChannelSet.begin(), allChannelSet.end(), regiterChannelSet.begin(), regiterChannelSet.end(), inserter(diffSet, diffSet.begin()));

                if (diffSet.empty())
                {
                    ;
                }  
                else
                {
//                    g_sockChannelLock.lock();
//                    set<int> chnlSetLast(diffSet);
//                    set<int>::iterator it;
//                    for (it = chnlSetLast.begin(),mapIter = g_sockChannelMap.begin(); it != chnlSetLast.end(); ++it)
//                    {
//                        int chnlNo = *it;
//                        //for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();mapIter++)
//                        if (mapIter != g_sockChannelMap.end())
//                        {
//                            mapIter->second.chnlSet.insert(chnlNo);
//                            mapIter++;
//                        } 
//                        else
//                        {
//                            mapIter = g_sockChannelMap.begin();
//                            if (mapIter != g_sockChannelMap.end())
//                            {
//                                mapIter->second.chnlSet.insert(chnlNo);
//                                mapIter++;
//                            }
//                        }
//                    }            
//                    g_sockChannelLock.unlock();  
                    
//------------------------------------------------------------------------                    
                    g_sockChannelLock.lock();
                    set<int> chnlSetLast(diffSet);
                    set<int>::iterator it;
                    for (it = chnlSetLast.begin(); it != chnlSetLast.end(); ++it)
                    {
                            int nMin = -1;
                            std::map<std::string, structWorkerChannelInfo>::iterator mapIter2   = g_sockChannelMap.begin();
                            std::map<std::string, structWorkerChannelInfo>::iterator mapIterMin = g_sockChannelMap.begin();
                            for (mapIter2 = g_sockChannelMap.begin(); mapIter2 != g_sockChannelMap.end(); ++mapIter2)
                            {
                                int nElemSize = mapIter2->second.chnlSet.size();
//                                    if (nMin == 0 && nElemSize >= nMin)
//                                    {
//                                            nMin = nElemSize;
//                                            mapIterMin = mapIter2;
//                                    }
//                                    else if ( nMin != 0 && nElemSize < nMin)
//                                    {
//                                            nMin = nElemSize;
//                                            mapIterMin = mapIter2;
//                                    }
                                if (nElemSize == 0)
                                {
                                        nMin = nElemSize;
                                        mapIterMin = mapIter2;   
                                        break;
                                }
                                else if (nMin == -1)
                                {
                                        nMin = nElemSize;
                                        mapIterMin = mapIter2;                            
                                }
                                else if (nElemSize < nMin)
                                {
                                        nMin = nElemSize;
                                        mapIterMin = mapIter2;
                                }                                    
                            } 

                            int chnlNo = *it;
                            syslog(LOG_DEBUG, "====================2N%d", chnlNo);
                            if (mapIterMin != g_sockChannelMap.end())
                            {
                                    mapIterMin->second.chnlSet.insert(chnlNo);
                                    syslog(LOG_DEBUG, "insert three\n");
                            }
                    }            
                    g_sockChannelLock.unlock();                     
                } 
            }
        }
        else
        {
            // timeout
            ;
        }
        //sleep(3);
        SysSleep(1 * 1000);
    }


    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}


// 2016-6-15

void * SyncProc(void * pParam)
{
    int nResult = 0;    
    int nLen        = 0;
    int nNewLen     = 0;
    int nNewSocket  = -1;
    char pNewBuffer[nMaxLen + 1] = {0};
    char szFileData[2048] = {0};
    while(!GlobalStopFlag)
    {
        if (g_configInfo.strMaster == "yes" && nResult == 0)
        {
            // sync
            sleep(5);
            
            if (g_sockChannelMap.size() <= 0)
            {
                sleep(10);
                continue;
            }
            
            boost::timer tt1;
            WriteHostInfoFile();
            syslog(LOG_DEBUG, "write host file time:%lf", tt1.elapsed());
            g_sem.post();
            
//            boost::timer tt2;
//            std::string strFileNameNew = "/root/jiezisoft/jzmanager/HostInfo.xml";
//            unsigned long filesize = -1;      
//            struct stat statbuff;  
//            if(::stat(strFileNameNew.c_str(), &statbuff) < 0)
//            {  
//                sleep(1);
//                continue;
//            }
//            else
//            {  
//                filesize = statbuff.st_size;  
//            }    
//            
//            FILE* pFile = fopen(strFileNameNew.c_str(), "rb");
//            if (!pFile)
//            {
//                    sleep(1);
//                    continue;
//            }     
//            fread(szFileData, 1, filesize, pFile);
//            fclose(pFile);   
//            szFileData[filesize] = '\0';    
//            syslog(LOG_DEBUG, "fread host file time:%lf", tt2.elapsed());
            
            
//            // ok
//            boost::timer tt3;
//             std::map<std::string, structWorkerChannelInfo>::iterator mapIter;
//            std::vector<std::string> hostInfoVect;
//            g_sockChannelLock.lock();
//            for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();++mapIter)
//            {
//                std::string strName   = mapIter->first;
//                hostInfoVect.push_back(strName);
//            }      
//            g_sockChannelLock.unlock();  
//
//            //int nHostSize = g_configInfo.hostInfoVect.size();
//            int nHostSize = hostInfoVect.size();
//            
//            for (int n = 0; n < nHostSize; ++n)
//            {
//                //if (g_configInfo.strLocalIP != g_configInfo.hostInfoVect[n].strHostName)
//                if (g_configInfo.strLocalIP != hostInfoVect[n])
//                {
//                    nNewSocket = CreateSocket();
//                    if (nNewSocket == -1)
//                    {
//                        //	不能建立套接字，直接返回
//                        nNewSocket = CreateSocket();
//                        if (nNewSocket == -1)
//                        {
//                            continue;
//                        }
//                    }
//
//                    int GlobalRemotePort = atoi(g_configInfo.strLocalPortHandle2.c_str());
//                    //if (ConnectSocket(nNewSocket, g_configInfo.hostInfoVect[n].strHostName.c_str(), GlobalRemotePort) <= 0)
//                    if (ConnectSocket(nNewSocket, hostInfoVect[n].c_str(), GlobalRemotePort) <= 0)
//                    {
//                        CloseSocket(nNewSocket);
//                        continue;
//                    } 
//
//                    // ===========first start
//                    int index = 0;
//                    int nTotalLen = 0;
//
//                    memcpy(pNewBuffer + index, &nTotalLen, 4);
//                    index += 4;
//
//                    int nType = 10;
//                    memcpy(pNewBuffer + index, &nType, 4);
//                    index += 4;
//
//
//                    int nFileSize = filesize;
//                    memcpy(pNewBuffer + index, &nFileSize, 4);
//                    index += 4;        
//
//                    memcpy(pNewBuffer + index, szFileData, filesize);
//                    index += filesize;
//
//                    nNewLen = nTotalLen = index;
//                    memcpy(pNewBuffer, &nTotalLen, 4);
//
//                    nLen = SocketWrite(nNewSocket, pNewBuffer, nNewLen, 30);
//                    syslog(LOG_DEBUG, "write file len:%d,%d\n", nLen, __LINE__);
//                    CloseSocket(nNewSocket);
//                }                
//            } 
//            syslog(LOG_DEBUG, "send host file time:%lf", tt3.elapsed());
        }
        sleep(10);
    }


    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}



void * ProcessSendProc(void * pParam)
{
    int nResult = 0;    
    int nLen        = 0;
    int nNewLen     = 0;
    int nNewSocket  = -1;
    char pNewBuffer[nMaxLen + 1] = {0};
    char szFileData[2048] = {0};
    while(!GlobalStopFlag)
    {
        if (g_configInfo.strMaster == "yes" && nResult == 0)
        {
            // sync
            nResult = g_sem.wait_timed(3);
            if (nResult == 0)
            {
                boost::timer tt;
                std::string strFileNameNew = "/root/jiezisoft/jzmanager/HostInfo.xml";
                unsigned long filesize = -1;      
                struct stat statbuff;  
                if(::stat(strFileNameNew.c_str(), &statbuff) < 0)
                {  
                    sleep(1);
                    continue;
                }
                else
                {  
                    filesize = statbuff.st_size;  
                }    

                FILE* pFile = fopen(strFileNameNew.c_str(), "rb");
                if (!pFile)
                {
                        sleep(1);
                        continue;
                }     
                fread(szFileData, 1, filesize, pFile);
                fclose(pFile);   
                szFileData[filesize] = '\0';    
                syslog(LOG_DEBUG, "fread host file time:%lf", tt.elapsed());  
                
                if (filesize < 20)
                {
                        sleep(1);
                        continue;
                }                  
                
                
                // ok
                boost::timer tt3;
                 std::map<std::string, structWorkerChannelInfo>::iterator mapIter;
                std::vector<std::string> hostInfoVect;
                g_sockChannelLock.lock();
                for (mapIter = g_sockChannelMap.begin(); mapIter != g_sockChannelMap.end();++mapIter)
                {
                    std::string strName   = mapIter->first;
                    hostInfoVect.push_back(strName);
                }      
                g_sockChannelLock.unlock();  

                //int nHostSize = g_configInfo.hostInfoVect.size();
                int nHostSize = hostInfoVect.size();

                for (int n = 0; n < nHostSize; ++n)
                {
                    //if (g_configInfo.strLocalIP != g_configInfo.hostInfoVect[n].strHostName)
                    if (g_configInfo.strLocalIP != hostInfoVect[n])
                    {
                        nNewSocket = CreateSocket();
                        if (nNewSocket == -1)
                        {
                            //	不能建立套接字，直接返回
                            nNewSocket = CreateSocket();
                            if (nNewSocket == -1)
                            {
                                continue;
                            }
                        }

                        int GlobalRemotePort = atoi(g_configInfo.strLocalPortHandle2.c_str());
                        //if (ConnectSocket(nNewSocket, g_configInfo.hostInfoVect[n].strHostName.c_str(), GlobalRemotePort) <= 0)
                        if (ConnectSocket(nNewSocket, hostInfoVect[n].c_str(), GlobalRemotePort) <= 0)
                        {
                            CloseSocket(nNewSocket);
                            continue;
                        } 

                        // ===========first start
                        int index = 0;
                        int nTotalLen = 0;

                        memcpy(pNewBuffer + index, &nTotalLen, 4);
                        index += 4;

                        int nType = 10;
                        memcpy(pNewBuffer + index, &nType, 4);
                        index += 4;


                        int nFileSize = filesize;
                        memcpy(pNewBuffer + index, &nFileSize, 4);
                        index += 4;        

                        memcpy(pNewBuffer + index, szFileData, filesize);
                        index += filesize;

                        nNewLen = nTotalLen = index;
                        memcpy(pNewBuffer, &nTotalLen, 4);

                        nLen = SocketWrite(nNewSocket, pNewBuffer, nNewLen, 30);
                        syslog(LOG_DEBUG, "write file len:%d,%d\n", nLen, __LINE__);
                        CloseSocket(nNewSocket);
                    }                
                } 
                syslog(LOG_DEBUG, "send host file time:%lf", tt3.elapsed());                
            }
            else
            {
                //syslog(LOG_DEBUG, "timeout\n");
            }
        }
        sleep(2);
    }


    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}






#if !defined(_WIN32)&&!defined(_WIN64)
extern "C" void	sig_term(int signo)
{
	if(signo==SIGTERM)
	{
		GlobalStopFlag=1;
		signal(SIGTERM,sig_term);
		signal(SIGHUP,SIG_IGN);
		signal(SIGKILL,sig_term);
		signal(SIGINT,sig_term);
    	signal(SIGPIPE,SIG_IGN);
    	signal(SIGALRM,SIG_IGN);
	}
}

long daemon_init()
{
	pid_t pid;
    signal(SIGALRM,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);

	if((pid=fork())<0)
		return 0;
	else if(pid!=0)
		exit(0);
	setsid();

	signal(SIGTERM,sig_term);
	signal(SIGINT,sig_term);
	signal(SIGHUP,SIG_IGN);
	signal(SIGKILL,sig_term);
    signal(SIGPIPE,SIG_IGN);
    signal(SIGALRM,SIG_IGN);

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	return 1;
}
#else
BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
		case CTRL_CLOSE_EVENT:	//	close console	
            GlobalStopFlag=1;
            return TRUE;
            break;
    }
    return FALSE;
}

#endif

int startForSync(int &nSocket2, SERVER_PARAM *&pParam)
{
	int nLocalPort	=-1;
	nLocalPort	= atoi(g_configInfo.strLocalPortHandle2.c_str());

	if (nLocalPort<=0)// || (nRemotePort<=0))
	{
            syslog(LOG_DEBUG, "invalid local port port:%d\n", nLocalPort);
            return -1;
	}

	int nSocket = 0; // CreateSocket();
        nSocket2 = CreateSocket();
        nSocket = nSocket2;
//	create socket
	if(!CheckSocketValid(nSocket))
	{
		UninitWinSock();
                nSocket2 = 0;
		return -1;
	}
	int rc=0;
	rc=BindPort(nSocket,nLocalPort);
	if(!rc)
	{
		CloseSocket(nSocket);
		UninitWinSock();
                nSocket2 = 0;
		return -1;
	}
	rc=ListenSocket(nSocket,30);
	if(!rc)
	{
		CloseSocket(nSocket);
		UninitWinSock();
                nSocket2 = 0;
		return -1;
	}
	MyInitLock();				
	//SetSocketNotBlock(nSocket);
	SERVER_PARAM * localServerParam = new SERVER_PARAM;
        pParam = localServerParam;
                
   	sockaddr_in name;
   	memset(&name,0,sizeof(sockaddr_in));
   	name.sin_family=AF_INET;
   	name.sin_port=htons((unsigned short)nLocalPort);
	name.sin_addr.s_addr=INADDR_ANY;
	localServerParam->ServerSockaddr=name;
	localServerParam->nSocket=nSocket;
	localServerParam->nEndFlag=1;

	pthread_t localThreadId;

	int nThreadErr=0;

        nThreadErr=pthread_create(&localThreadId,NULL, (THREADFUNC)AcceptProcForSync,localServerParam);
	if(nThreadErr==0)
		pthread_detach(localThreadId);
//	释放线程私有数据,不必等待pthread_join();

	if(nThreadErr)
	{
//	建立线程失败
		MyUninitLock2();
                nSocket2 = 0;
		return -1;
	}
        
	return 0;
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
                    // online
                    //syslog(LOG_DEBUG, "%s running\n", g_configInfo.strNetCardName.c_str());
            }
            else
            {
                
                g_configInfo.m_lock.lock();
                if (g_configInfo.strMaster == "yes")
                {
                    g_configInfo.strMaster = "no";
                }
//                else if (g_configInfo.strHostRole == "backhost")
//                {
//                    configInfo.strMaster = "no";
//                }
//                else
//                {
//                    configInfo.strMaster = "no";
//                }
                g_configInfo.m_lock.unlock();                
                
                
                
                
                // offline
                syslog(LOG_DEBUG, "%s not running\n", g_configInfo.strNetCardName.c_str());

                // 杀掉机器N上的通道进程
                {
                        // 杀掉机器N上的主通道进程
                        syslog(LOG_DEBUG, "kill  process on machine\n");
                        char sh_cmd[512] = {0};
                        sprintf(sh_cmd, "bash /root/jiezisoft/hckillserver.sh");
                        system(sh_cmd);
                }
                //sleep(15);
            }

            sleep(2);
	} 
        SysSleep(50);
	pthread_exit(NULL);
	return NULL;
}



void * CancelRecvThread(void * pParam)
{
    while (!GlobalStopFlag)
    {	
        std::map<pthread_t, structThreadInfo>::iterator mapIter;
        structThreadInfo tmpThreadInfo;

        g_threadMapLock.lock();

        for (mapIter = g_threadMap.begin(); mapIter != g_threadMap.end();)
        {
                int nLastSendTime   = mapIter->second.nSendTime;
                int nLastRecvTime   = mapIter->second.nRecvTime;
                int nNowTime    	= time(0);
                if (abs(nNowTime - nLastSendTime) > 15 && abs(nNowTime - nLastRecvTime) > 15)
                {
                        tmpThreadInfo   = mapIter->second;
                        g_threadMap.erase(mapIter);
                        syslog(LOG_DEBUG, "X>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>15:%lu", tmpThreadInfo.threadId);
                        break;
                } 
                else
                {
                        mapIter++;
                }
        }   		                    
        g_threadMapLock.unlock();		
		
        
        pthread_t tmpThreadId = tmpThreadInfo.threadId;
        if (tmpThreadId > 0)
        {			
            syslog(LOG_DEBUG, "manager begin cancel process\n");
            // cancel
            if (tmpThreadId != 0)
            {
                    pthread_cancel(tmpThreadId);
                    syslog(LOG_DEBUG, "manager cancel process finish\n");
            }
        }
        sleep(5);
    }


    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}


int main(int argc, char * argv[])
{
	openlog("JZManager", LOG_PID, LOG_LOCAL7);
	ReadConfigInfo(g_configInfo);
//        WriteConfigFile();
//        return 0;
        
        WriteConfigFile();
	//if(argc<4)
	//{
	//	PrintUsage(argv[0]);
	//	return -1;
	//}
	int nLocalPort	=-1;
	int nRemotePort	=-1;
	nLocalPort		= atoi(g_configInfo.strLocalPortHandle.c_str());
	//nRemotePort		= atoi(g_configInfo.strKaKouPortHandle.c_str());
	if (nLocalPort<=0)// || (nRemotePort<=0))
	{
		syslog(LOG_DEBUG, "invalid local port or remote port \n");
		return -1;
	}

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
	int nSocket = CreateSocket();
//	create socket
	if(!CheckSocketValid(nSocket))
	{
		UninitWinSock();
		return -1;
	}
	int rc=0;
	rc=BindPort(nSocket,nLocalPort);
	if(!rc)
	{
		CloseSocket(nSocket);
		UninitWinSock();
		return -1;
	}
	rc=ListenSocket(nSocket,30);
	if(!rc)
	{
		CloseSocket(nSocket);
		UninitWinSock();
		return -1;
	}
	MyInitLock();				
	//SetSocketNotBlock(nSocket);
	SERVER_PARAM * localServerParam=new SERVER_PARAM;
   	sockaddr_in name;
   	memset(&name,0,sizeof(sockaddr_in));
   	name.sin_family=AF_INET;
   	name.sin_port=htons((unsigned short)nLocalPort);
	name.sin_addr.s_addr=INADDR_ANY;
	localServerParam->ServerSockaddr=name;
	localServerParam->nSocket=nSocket;
	localServerParam->nEndFlag=1;

	pthread_t localThreadId;

	int nThreadErr=0;

        nThreadErr=pthread_create(&localThreadId,NULL,
            (THREADFUNC)AcceptProc,localServerParam);
	if(nThreadErr==0)
		pthread_detach(localThreadId);
//	释放线程私有数据,不必等待pthread_join();

	if(nThreadErr)
	{
//	建立线程失败
		MyUninitLock();
		CloseSocket(nSocket);
		UninitWinSock();
		delete localServerParam;
		return -1;
	}
        
        
        //if (g_configInfo.strHostRole == "mainhost")
        {
            pthread_t voteThreadId;

            int nThreadErr2=0;

            nThreadErr2=pthread_create(&voteThreadId,NULL,
                (THREADFUNC)VoteProc,localServerParam);
            if(nThreadErr2==0)
                    pthread_detach(voteThreadId);
    //	释放线程私有数据,不必等待pthread_join();

            if(nThreadErr2)
            {
    //	建立线程失败
                    MyUninitLock();
                    CloseSocket(nSocket);
                    UninitWinSock();
                    delete localServerParam;
                    return -1;
            }              
        }
        
        
        //if (g_configInfo.strHostRole == "mainhost")
        {
            pthread_t voteThreadId;

            int nThreadErr3=0;

            nThreadErr3=pthread_create(&voteThreadId,NULL,
                (THREADFUNC)SyncProc,localServerParam);
            if(nThreadErr3==0)
                    pthread_detach(voteThreadId);
    //	释放线程私有数据,不必等待pthread_join();

            if(nThreadErr3)
            {
    //	建立线程失败
                    MyUninitLock();
                    CloseSocket(nSocket);
                    UninitWinSock();
                    delete localServerParam;
                    return -1;
            }              
        }        
        
//==========================send thread
        {
            pthread_t sendThreadId;

            int nThreadErr4=0;

            nThreadErr4=pthread_create(&sendThreadId,NULL,
                (THREADFUNC)ProcessSendProc,localServerParam);
            if(nThreadErr4==0)
                    pthread_detach(sendThreadId);

            if(nThreadErr4)
            {
                    MyUninitLock();
                    CloseSocket(nSocket);
                    UninitWinSock();
                    delete localServerParam;
                    return -1;
            }              
        } 
        
//================================================================
		{
			pthread_t ThreadId5;

			int nThreadErr5=0;

			nThreadErr5 = pthread_create(&ThreadId5,NULL, (THREADFUNC)ProcessNICProc,NULL);
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
        
//====================================================================        
        {
            pthread_t ThreadId6;

            int nThreadErr6=0;

            nThreadErr6=pthread_create(&ThreadId6,NULL, (THREADFUNC)CancelRecvThread,NULL);
            if(nThreadErr6==0)
                    pthread_detach(ThreadId6);
    //	释放线程私有数据,不必等待pthread_join();

            if(nThreadErr6)
            {
    //	建立线程失败
                    MyUninitLock();
                    UninitWinSock();
                    return -1;
            }              
        }        
        
        
        
        
        int nSocket2 = 0;
        SERVER_PARAM *pParam = NULL;
        startForSync(nSocket2, pParam);
        
        
	while((GlobalStopFlag==0)||(localServerParam->nEndFlag==0))
	{
		SysSleep(500);
	}
	MyUninitLock();
        MyUninitLock2();
	CloseSocket(nSocket);
        if (nSocket2 != 0)
            CloseSocket(nSocket2);
	UninitWinSock();
	delete localServerParam;
        
        if (pParam != NULL)
            delete pParam;
        
	return 0;
}

