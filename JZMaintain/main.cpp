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
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/format.hpp>     
#include <boost/tokenizer.hpp>     
#include <boost/algorithm/string.hpp>  

#include "tinyxml/tinyxml.h"
#include "jzLocker.h"

using namespace std;
using namespace boost;
using namespace boost::gregorian;
using namespace boost::posix_time; 

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


struct structChnlRebootInfo
{
    int nLastDay;
    int nLastHour;
    int nDay;
    int nHour;
    int nMinute;
    int nChnlNo;
    std::string strChnlNo;
    std::string strAbsoluteTime;
    std::string strRelativeTime;
    std::string strRelativeAttr;
    
    structChnlRebootInfo():nLastDay(0),nLastHour(0),nDay(0),nHour(0),nMinute(0),nChnlNo(0),strChnlNo(""),strAbsoluteTime(""),strRelativeTime(""),strRelativeAttr("")
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
    
    string strAMTime;
    string strPMTime;
    
    std::vector<structHostInfo>         hostInfoVect;
    std::vector<structChnlRebootInfo>   chnlRebootInfo;
    
    structConfigInfo():strMaster(""),strHostRole(""),strLocalPortHandle(""),strLocalPortHandle2(""),strLocalIP(""),\
                        strNetCardName(""),strHeartbeatTime(""),strAMTime(""),strPMTime(""),hostInfoVect(),chnlRebootInfo()
    {
        ;
    }
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


int     m_nETHStatus = 0;
locker	m_ETHLocker;

structConfigInfo g_configInfo;

locker                                      g_sockChannelLock;
std::map<std::string, structWorkerChannelInfo>      g_sockChannelMap;
std::string g_strConfigFileName = "";

std::map<pthread_t, structThreadInfo> 	g_threadMap;
locker					g_threadMapLock;



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
    sprintf(cpPath, "%sJZMaintainConfig.xml", strFullExeName.c_str());
    g_strConfigFileName = cpPath;

    TiXmlDocument doc(cpPath);
    doc.LoadFile();

    TiXmlNode* node = 0;
    TiXmlElement* areaInfoElement = 0;
    TiXmlElement* itemElement = 0;

    TiXmlHandle docHandle( &doc );
    TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("ConfigInfo");
    TiXmlHandle NetCardNameHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("NetCardName", 0).FirstChild();
    
    TiXmlHandle AMCheckTimeHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("AMCheckTime", 0).FirstChild();
    TiXmlHandle PMCheckTimeHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("PMCheckTime", 0).FirstChild();    
    
    if (AMCheckTimeHandle.Node() != NULL)
        configInfo.strAMTime = AMCheckTimeHandle.Text()->Value();
    
    if (PMCheckTimeHandle.Node() != NULL)
        configInfo.strPMTime = PMCheckTimeHandle.Text()->Value();    
     
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
     
    syslog(LOG_DEBUG, "%s, %s", configInfo.strAMTime.c_str(), configInfo.strPMTime.c_str());
    syslog(LOG_DEBUG, "%s, %s", configInfo.strNetCardName.c_str(), configInfo.strLocalIP.c_str());
    

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

    xmlRootElement = pConfigInfoNode->ToElement();
    if (xmlRootElement != NULL && (pNode = xmlRootElement->FirstChild("ChannelList")) != NULL)
    {
            xmlListElement = pNode->ToElement();

            for (pNode = xmlListElement->FirstChild("ChannelNo"); pNode != NULL; pNode = pNode->NextSibling("ChannelNo"))
            {
                    structChnlRebootInfo      tmpRebootInfo;
                    string AbsoluteTime        = "";
                    string RelativeTime        = "";
                    string SeqNo               = "";
                    string Attr                = "";

                    xmlSubElement = pNode->ToElement() ;
                    
                    if (xmlSubElement != NULL)
                    {
                        if (xmlSubElement->Attribute("SeqNo") != NULL)
                        {
                            SeqNo = xmlSubElement->Attribute("SeqNo");
                            tmpRebootInfo.strChnlNo = SeqNo;
                            tmpRebootInfo.nChnlNo   = atoi(SeqNo.c_str());
                        }
                    }                    
                    
                    
                    if (xmlSubElement != NULL)
                            pNodeTmp = xmlSubElement->FirstChildElement("AbsoluteTime");

                    if (pNodeTmp != NULL && pNodeTmp->ToElement() != NULL && pNodeTmp->ToElement()->GetText() != 0)
                    {
                            AbsoluteTime = pNodeTmp->ToElement()->GetText();
                            
                            std::vector<std::string> splitVect;
                            split(splitVect, AbsoluteTime, is_any_of(":"));
                            if (splitVect.size() > 0)
                            {
                                tmpRebootInfo.nHour = atoi(splitVect[0].c_str());
                            }
                            if (splitVect.size() > 1)
                            {
                                tmpRebootInfo.nMinute = atoi(splitVect[1].c_str());
                            }                            
                            
                    }
                    tmpRebootInfo.strAbsoluteTime = AbsoluteTime;

                    if (xmlSubElement != NULL)
                            pNodeTmp = xmlSubElement->FirstChildElement("RelativeTime");

                    if (pNodeTmp != NULL && pNodeTmp->ToElement() != NULL && pNodeTmp->ToElement()->GetText() != 0)
                            RelativeTime = pNodeTmp->ToElement()->GetText();
                    tmpRebootInfo.strRelativeTime = RelativeTime;
                    
                    if (pNodeTmp != NULL)
                    {
                        xmlSubElement = pNodeTmp->ToElement() ;
                        if (xmlSubElement != NULL)
                        {
                            if (xmlSubElement->Attribute("attr") != NULL)
                            {
                                Attr = xmlSubElement->Attribute("attr");
                                tmpRebootInfo.strRelativeAttr = Attr;
                            }
//                            if (xmlSubElement->GetText() != 0)
//                                    RelativeTime = xmlSubElement->GetText();
//                            tmpRebootInfo.strRelativeTime = RelativeTime;                            
                            
                        }                         
                    }
                    
                     
                    
                    
                    g_configInfo.chnlRebootInfo.push_back(tmpRebootInfo);
            }
    }     
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
}

void ReadConfigInfo5(struct structConfigInfo &configInfo)
{
    char cpPath[256] = {0};
    //char temp[256] = {0};
    sprintf(cpPath, "/root/jiezisoft/jzmanager/HostInfo.xml");
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
                                    //syslog(LOG_DEBUG, "HostName:%s, ChannelNo:%s", tmpHostInfo.strHostName.c_str(), strChnlNo.c_str());
                            }

                            //syslog(LOG_DEBUG, "\n");
                    } 
                    outputVect1.resize(tmpHostInfo.chnlSet.size());
                    std::copy(tmpHostInfo.chnlSet.begin(), tmpHostInfo.chnlSet.end(), outputVect1.begin());


//=====================================================
                    g_configInfo.m_lock.lock();
                    g_configInfo.hostInfoVect.push_back(tmpHostInfo);                    
                    g_configInfo.m_lock.unlock();
//==================================================
            }
    }     

//    syslog(LOG_DEBUG, "%s, %s", configInfo.strHostRole.c_str(), configInfo.strLocalPortHandle.c_str());
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
                
                m_ETHLocker.lock();
                m_nETHStatus = 0;
                m_ETHLocker.unlock();                
            }

            sleep(2);
	} 
        SysSleep(50);
	pthread_exit(NULL);
	return NULL;
}




void * JZReadFileThread(void * pParam)
{

    
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
        
        // get hostinfo.xml
        ReadConfigInfo5(g_configInfo);
        
////=====================================================
//        std::vector<int> outputVect1;
//        g_configInfo.m_lock.lock();
//        int nVectSize = g_configInfo.hostInfoVect.size();
//        for (int i = 0; i != nVectSize; ++i)
//        {
//                if (g_configInfo.hostInfoVect[i].strHostName == g_configInfo.strLocalIP)
//                {
//                        outputVect1.resize(g_configInfo.hostInfoVect[i].chnlSet.size());
//                        std::copy(g_configInfo.hostInfoVect[i].chnlSet.begin(), g_configInfo.hostInfoVect[i].chnlSet.end(), outputVect1.begin());
//                        break;
//                }
//        }
// 
//        g_configInfo.m_lock.unlock();
////==================================================        
//        if (outputVect1.size() <= 0)
//        {
//            sleep(5);
//            continue;
//        }

        sleep(15);
    }


    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}





void * JZCrontabThread(void * pParam)
{
    
    int nChnlNo = *(int *)pParam;
 
    
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
        
        // get hostinfo.xml
        
//=====================================================
        std::vector<int> outputVect1;
        g_configInfo.m_lock.lock();
        int nVectSize = g_configInfo.hostInfoVect.size();
        for (int i = 0; i != nVectSize; ++i)
        {
                if (g_configInfo.hostInfoVect[i].strHostName == g_configInfo.strLocalIP)
                {
                        outputVect1.resize(g_configInfo.hostInfoVect[i].chnlSet.size());
                        std::copy(g_configInfo.hostInfoVect[i].chnlSet.begin(), g_configInfo.hostInfoVect[i].chnlSet.end(), outputVect1.begin());
                        break;
                }
        }
 
        g_configInfo.m_lock.unlock();
//==================================================        
        if (outputVect1.size() <= 0)
        {
            sleep(5);
            continue;
        }
        
        
        
        std::vector<int>::iterator result = find(outputVect1.begin(), outputVect1.end(), nChnlNo);
        if (result != outputVect1.end())
        {
            ;
        }
        else
        {
            sleep(5);
            continue;            
        }
        //======================================
        
        
        date today = day_clock::local_day();
        int nNowDay = today.day();
        
        ptime now = second_clock::local_time();  
        time_duration td = now.time_of_day();
        int nHour   = td.hours();
        int nMinute = td.minutes();
        //int nSecond = td.seconds();
        int nExecFlag = 0;
        
        //for (int i = 0; i < outputVect1.size(); ++i)
        {
            int chnlNo = nChnlNo; // outputVect1[i];
            
            int j = 0;
            int nRebootSize = g_configInfo.chnlRebootInfo.size();
            for (j = 0; j < nRebootSize; ++j)
            {
                if (chnlNo == g_configInfo.chnlRebootInfo[j].nChnlNo)
                    break;
            }
            if (j < nRebootSize)
            {
                if (g_configInfo.chnlRebootInfo[j].nDay == 0)
                {
                    if (g_configInfo.chnlRebootInfo[j].nHour == nHour && g_configInfo.chnlRebootInfo[j].nMinute == nMinute)
                    {
                        char sh_cmd[512] = {0};
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/killchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);  

                        memset(sh_cmd, 0x00, sizeof(sh_cmd));
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/startchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);    
                        nExecFlag = 1;
                        sleep(3);
                    }
                }
                else
                {
                    if (g_configInfo.chnlRebootInfo[j].strRelativeAttr == "day"  && g_configInfo.chnlRebootInfo[j].nLastDay == 0 && g_configInfo.chnlRebootInfo[j].nHour == nHour && g_configInfo.chnlRebootInfo[j].nMinute == nMinute)
                    {
                        g_configInfo.m_lock.lock();
                        g_configInfo.chnlRebootInfo[j].nLastDay = nNowDay;
                        g_configInfo.m_lock.unlock();
                        
                        char sh_cmd[512] = {0};
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/killchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);  

                        memset(sh_cmd, 0x00, sizeof(sh_cmd));
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/startchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);    
                        nExecFlag = 1;
                        sleep(3);
                    }
                    else if (g_configInfo.chnlRebootInfo[j].strRelativeAttr == "day"  && abs(g_configInfo.chnlRebootInfo[j].nLastDay - nNowDay) == atoi(g_configInfo.chnlRebootInfo[j].strRelativeTime.c_str()) && g_configInfo.chnlRebootInfo[j].nHour == nHour && g_configInfo.chnlRebootInfo[j].nMinute == nMinute)
                    {
                        g_configInfo.m_lock.lock();
                        g_configInfo.chnlRebootInfo[j].nLastDay = nNowDay;
                        g_configInfo.m_lock.unlock();
                        
                        char sh_cmd[512] = {0};
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/killchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);  

                        memset(sh_cmd, 0x00, sizeof(sh_cmd));
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/startchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);    
                        nExecFlag = 1;
                        sleep(3);
                    }
                    
                    
                    
                    if (g_configInfo.chnlRebootInfo[j].strRelativeAttr == "hour"  && g_configInfo.chnlRebootInfo[j].nLastHour == 0 && g_configInfo.chnlRebootInfo[j].nHour == nHour && g_configInfo.chnlRebootInfo[j].nMinute == nMinute)
                    {
                        g_configInfo.m_lock.lock();
                        g_configInfo.chnlRebootInfo[j].nLastHour = nHour;
                        g_configInfo.m_lock.unlock();
                        
                        char sh_cmd[512] = {0};
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/killchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);  

                        memset(sh_cmd, 0x00, sizeof(sh_cmd));
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/startchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);    
                        nExecFlag = 1;
                        sleep(3);
                    }                    
                    else if (g_configInfo.chnlRebootInfo[j].strRelativeAttr == "hour"  && abs(g_configInfo.chnlRebootInfo[j].nLastHour - nHour) == atoi(g_configInfo.chnlRebootInfo[j].strRelativeTime.c_str()) && g_configInfo.chnlRebootInfo[j].nHour == nHour && g_configInfo.chnlRebootInfo[j].nMinute == nMinute)
                    {
                        g_configInfo.m_lock.lock();
                        g_configInfo.chnlRebootInfo[j].nLastHour = nHour;
                        g_configInfo.m_lock.unlock();
                        
                        char sh_cmd[512] = {0};
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/killchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);  

                        memset(sh_cmd, 0x00, sizeof(sh_cmd));
                        sprintf(sh_cmd, "bash /root/jiezisoft/A1/C%d/startchannel.sh", chnlNo);
                        system(sh_cmd);
                        syslog(LOG_DEBUG, "%s\n", sh_cmd);    
                        nExecFlag = 1;
                        sleep(3);
                    }                    
                    
                    
                    
                    
                }                  
            }        
        }

        if (nExecFlag == 1)
        {
            ptime now2 = second_clock::local_time();  
            time_duration td2 = now2.time_of_day();
            int nHour2   = td.hours();
            int nMinute2 = td.minutes();    
            
            if (nMinute == nMinute2)
            {
                int nCount = 30;
                while (nCount--)
                {
                    sleep(3);
                }
            }
        }
        sleep(20);
    }


    SysSleep(50);
    pthread_exit(NULL);
    return NULL;
}


int main(int argc, char * argv[])
{
	openlog("JZMaintain", LOG_PID, LOG_LOCAL7);
	ReadConfigInfo(g_configInfo);
        
        
//================================================================
        {
                pthread_t ThreadId4;

                int nThreadErr4=0;

                nThreadErr4 = pthread_create(&ThreadId4,NULL, (THREADFUNC)JZReadFileThread,NULL);
                if (nThreadErr4 == 0)
                        pthread_detach(ThreadId4);
                //	释放线程私有数据,不必等待pthread_join();

                if (nThreadErr4)
                {
                        //	建立线程失败
                        return -1;
                }              
        }  
//====================================================================         
        
        
        
        
        
        

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
                        return -1;
                }              
        }  
//====================================================================  
        
        int nRebootSize = g_configInfo.chnlRebootInfo.size();
        std::vector<pthread_t> tidVect(nRebootSize);
        std:;vector<int>       chnlVect;
        for (int i = 0; i < nRebootSize; ++i)
        {
            chnlVect.push_back(g_configInfo.chnlRebootInfo[i].nChnlNo);
        }
        for (int i = 0; i < nRebootSize; ++i)
        {
            //pthread_t ThreadId6;

            int nThreadErr6=0;

            nThreadErr6=pthread_create(&tidVect[i],NULL, (THREADFUNC)JZCrontabThread, &chnlVect[i]);
            if(nThreadErr6==0)
                    pthread_detach(tidVect[i]);
    //	释放线程私有数据,不必等待pthread_join();

            if(nThreadErr6)
            {
    //	建立线程失败
                    return -1;
            }              
        }        
        
        while (GlobalStopFlag == 0)
	{
		SysSleep(500);
	}
        
	return 0;
}

