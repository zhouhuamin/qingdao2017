// : Defines the entry point for the console application.
//	map local port to remote host port

#if defined(_WIN32)||defined(_WIN64)
#pragma warning(disable: 4996)
#include	<winsock2.h>
#include	<windows.h>
#include	<process.h>
#pragma comment(lib,"ws2_32.lib")	//	64位环境下需要做相应的调整

WORD wVersionRequested;
WSADATA wsaData;

void InitWinSock()
{
	wVersionRequested = MAKEWORD( 2, 0 );
	WSAStartup(wVersionRequested,&wsaData);
}
#else
#include	<pthread.h>
#include	<sys/socket.h>
#include    <sys/stat.h>
#include	<sys/types.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include 	<arpa/inet.h>
#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>
       #include <sys/wait.h>


#endif
#include <unistd.h>
#include <strings.h>
#include	"sys/types.h"
#include	"string.h"
#include 	<sys/types.h>
#include 	<fcntl.h>
#include	"stdio.h"
#include	"stdlib.h"

#include "HttpRequest.h"
#include <syslog.h>
#include <string>
#include <boost/shared_array.hpp>
#include "tinyxml/tinyxml.h"
#include "UtilTools.h"

#include "json_parser.h"
#include "JZBase64.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/std/utility.hpp>

#include <map>
#include <set>
#include <exception>
#include <iostream>
#include <vector>

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
int GlobalStopFlag=0;
#define	MAX_SOCKET	0x2000
//	max client per server
#define MAX_HOST_DATA 4096
int GlobalRemotePort=-1;
char GlobalRemoteHost[256];

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

struct structConfigInfo
{
	std::string strKaKouIPHandle;
	std::string strKaKouPortHandle;
	std::string strWcfURLHandle;
	std::string strLocalPortHandle;
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
        std::string BAR_CODE;
};

struct structValidateResult
{
    std::string success;
    std::string code;
    std::string message;
};



struct TestConfig
{
	std::string title;
	int number;
	std::map<int, std::string> groups;
	std::set<std::string> classes;
	void load(const std::string& filename);
	void save(const std::string& filename);
	void save2(const std::string& filename, const std::string &AREA_ID, const std::string &CHNL_NO, const std::string &I_E_TYPE, \
	const std::string &SEQ_NO);
};

structConfigInfo g_configInfo;

void TestConfig::load(const std::string& filename)
{
	using boost::property_tree::ptree;
	ptree pt;
	read_xml(filename, pt, boost::property_tree::xml_parser::trim_whitespace);

	title = pt.get_child("content.title").get<std::string>("<xmlattr>.value");
	std::cout << title << std::endl;

	number = pt.get<int>("content.number");
	std::cout << number << std::endl;

	ptree &groups_node = pt.get_child("content.groups");
	BOOST_FOREACH(const ptree::value_type& vt, groups_node)
	{
		std::string num = vt.second.get<std::string>("<xmlattr>.num");
		std::string type = vt.second.get<std::string>("<xmlattr>.type");
		groups.insert(std::pair<int, std::string>(atoi(num.c_str()), type));
		std::cout << num << "," << type << std::endl;
	}

	ptree &classes_node = pt.get_child("content.classes");
	BOOST_FOREACH(const ptree::value_type& vt, classes_node)
	{
		classes.insert(vt.second.data());
		std::cout << vt.second.data() << std::endl;
	}
}

void TestConfig::save(const std::string& filename)
{
	//strCoding cfm;
	using boost::property_tree::ptree;
	ptree pt, pattr1;
	string strTitle = "中国人";
	//cfm.GB2312ToUTF_8(title, (char*)strTitle.c_str(), strTitle.size());
	number = 12345;

	pattr1.add<std::string>("<xmlattr>.value", title);
	pt.add_child("content.title", pattr1);
	pt.put("content.number", number);

	typedef std::map<int, std::string> map_type;
	BOOST_FOREACH(const map_type::value_type &grp, groups)
	{
		ptree pattr2;
		pattr2.add<int>("<xmlattr>.num", grp.first);
		pattr2.add<std::string>("<xmlattr>.type", grp.second);
		pt.add_child("content.groups.class", pattr2);
	}

	BOOST_FOREACH(const std::string& cls, classes)
	{
		pt.add("content.classes.name", cls);
	}

	// 格式化输出，指定编码（默认utf-8）
	boost::property_tree::xml_writer_settings<char> settings('\t', 1); // , "GB2312");
	write_xml(filename, pt, std::locale(), settings);
}


void TestConfig::save2(const std::string& filename, const std::string &AREA_ID, const std::string &CHNL_NO, const std::string &I_E_TYPE, \
const std::string &SEQ_NO)
{
	char szOP_REASON[1024] = {0};
	using boost::property_tree::ptree;
	ptree pt;
	ptree pattr1;
	ptree pattr2;
	ptree pattr3;
	ptree pattr4;
	string strOpReason = "放行抬杆";

	CUtilTools::Gb2312ToUtf8(szOP_REASON, 1024, strOpReason.c_str(), strOpReason.size());
	title = szOP_REASON;
	number = 12345;

	//std::string AREA_ID = "1234567890";
	//std::string CHNL_NO	= "1000000005";
	//std::string I_E_TYPE = "I";
	//std::string SEQ_NO	 = "20150801144651336141";
	std::string EXCUTE_COMMAND	= "00000000000000000000";

	pattr1.add<std::string>("<xmlattr>.AREA_ID", AREA_ID);
	pattr1.add<std::string>("<xmlattr>.CHNL_NO", CHNL_NO);
	pattr1.add<std::string>("<xmlattr>.I_E_TYPE", I_E_TYPE);
	pattr1.add<std::string>("<xmlattr>.SEQ_NO", SEQ_NO);

	pt.add_child("COMMAND_INFO", pattr1);

	pt.put("COMMAND_INFO.EXCUTE_COMMAND", EXCUTE_COMMAND);

	pt.put("COMMAND_INFO.GPS.VE_NAME", "");
	pt.put("COMMAND_INFO.GPS.GPS_ID", "");
	pt.put("COMMAND_INFO.GPS.ORIGIN_CUSTOMS", "");
	pt.put("COMMAND_INFO.GPS.DEST_CUSTOMS", "");

	pt.put("COMMAND_INFO.SEAL.ESEAL_ID", "");
	pt.put("COMMAND_INFO.SEAL.SEAL_KEY", "");


	pt.put("COMMAND_INFO.OP_TYPE", "1");
	pt.put("COMMAND_INFO.OP_REASON", title);
	pt.put("COMMAND_INFO.OP_ID", "");

	// 格式化输出，指定编码（默认utf-8）
	boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "UTF-8");
	write_xml(filename, pt, std::locale(), settings);
}

unsigned long GetFileSize(const char *path)
{
	unsigned long filesize = -1;
	struct stat statbuff;
	if (stat(path, &statbuff) < 0)
	{
		return filesize;
	}
	else
	{
		filesize = statbuff.st_size;
	}
	return filesize;
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


void ReadConfigInfo(struct structConfigInfo &configInfo)
{
	//char szAppName[MAX_PATH] = {0};
	//size_t  len = 0;
	//::GetModuleFileName(AfxGetInstanceHandle(), szAppName, sizeof(szAppName));
	//len = strlen(szAppName);
	//for(size_t i=len; i>0; i--)
	//{
	//	if(szAppName[i] == '\\')
	//	{
	//		szAppName[i+1]='\0';
	//		break;
	//	}
	//}


	//CString strFileName=szAppName;
	//strFileName += "ProxyConfig.xml";
	char exeFullPath[256]	= {0};
	char EXE_FULL_PATH[256]	= {0};

	GetProcessPath(exeFullPath); //得到程序模块名称，全路径

	string strFullExeName(exeFullPath);

	int nLast = strFullExeName.find_last_of("/");

	strFullExeName = strFullExeName.substr(0, nLast + 1);

	strcpy(EXE_FULL_PATH, strFullExeName.c_str());

	char cpPath[256] = {0};

	char temp[256] = {0};

	sprintf(cpPath, "%sProxyConfig.xml", strFullExeName.c_str());



	TiXmlDocument doc(cpPath);
	doc.LoadFile();

	TiXmlNode* node = 0;
	TiXmlElement* areaInfoElement = 0;
	TiXmlElement* itemElement = 0;

	TiXmlHandle docHandle( &doc );
	TiXmlHandle ConfigInfoHandle = docHandle.FirstChildElement("ConfigInfo");
	TiXmlHandle KaKouIPHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("KaKouIP", 0).FirstChild();
	TiXmlHandle KaKouPortHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("KaKouPort", 0).FirstChild();
	TiXmlHandle WebURLHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("WebURL", 0).FirstChild();
	TiXmlHandle LocalPortHandle = docHandle.FirstChildElement("ConfigInfo").ChildElement("LocalPort", 0).FirstChild();

	string strKaKouIPHandle		= "";
	string strKaKouPortHandle	= "";
	string strWcfURLHandle		= "";
	string strLocalPortHandle	= "";

	if (KaKouIPHandle.Node() != NULL)
		configInfo.strKaKouIPHandle = strKaKouIPHandle 			= KaKouIPHandle.Text()->Value();

	if (KaKouPortHandle.Node() != NULL)
		configInfo.strKaKouPortHandle = strKaKouPortHandle 		= KaKouPortHandle.Text()->Value();

	if (WebURLHandle.Node() != NULL)
		configInfo.strWcfURLHandle = strWcfURLHandle 			= WebURLHandle.Text()->Value();

	if (LocalPortHandle.Node() != NULL)
		configInfo.strLocalPortHandle = strLocalPortHandle 		= LocalPortHandle.Text()->Value();

	syslog(LOG_DEBUG, "%s, %s, %s, %s", strKaKouIPHandle.c_str(), strKaKouPortHandle.c_str(), strWcfURLHandle.c_str(), strLocalPortHandle.c_str());
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

void	MyUninitLock()
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


void SplitString(std::string source, std::vector<std::string>& dest, const std::string &division)
{
	if (source.empty())
	{
		return;
	}

	int pos = 0;
	int pre_pos = 0;
	while( -1 != pos )
	{
		pre_pos = pos;

		pos = source.find_first_of(division, pos);
		if (pos == -1)
		{
			std::string str = source.substr(pos + 1);
			dest.push_back(str);
			break;
		}

		std::string tmp = source.substr(pre_pos, (pos-pre_pos));
		dest.push_back(tmp);

		source = source.substr(pos + 1);
		pos = 0;
	}
}


int ParseSalesAndBuysInfoJson(const structGatherInfo  &gatherInfo, const std::string &strData, std::string &strRetXml)  
{  
//  std::string str = "{\"code\":0,\"images\":[{\"url\":\"fmn057/20111221/1130/head_kJoO_05d9000251de125c.jpg\"},{\"url\":\"fmn057/20111221/1130/original_kJoO_05d9000251de125c.jpg\"}]}"; 
//  std::string str = "{\"status\":\"OK\",\"DataType\":\"1\",\"DataValue\":\"购货单位,车号,司机,手机,产品名称,规格型号,单位,计划数量,订单号1,订单号1剩余量,订单号2,订单号2剩余量,订单号3,订单号3剩余量,订单号4,订单号4剩余量,订单号5,订单号5剩余量,订单号6,订单号6剩余量,订单号7,订单号7剩余量,订单号8,订单号8剩余量,订单号9,订单号9剩余量,订单号10,订单号10剩余量,是否允许发卡\"}";

//			std::string strFileName = "/root/taian-test/222222.data";
//			FILE *pFile	= fopen(strFileName.c_str(), "wb");
//			if (pFile == NULL)
//			{
//				;
//			}
//			int nReadFileSize = fwrite(str.c_str(), 1, str.size(), pFile);
//                        {
//                            fclose(pFile);   
//                        }  
  
  
  
  using namespace boost::property_tree;  

  std::stringstream ss(strData);
//std::stringstream ss(str);    
  ptree pt;  
  try{      
    read_json(ss, pt);  
  }  
  catch(ptree_error & e) 
  {  
      syslog(LOG_DEBUG, "%s", e.what());    
    return 1;   
  }  

  try
  {  
        std::string status = pt.get<std::string>("status");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", status.c_str());

        std::string DataType = pt.get<std::string>("DataType");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", DataType.c_str());    

        std::string DataValue = pt.get<std::string>("DataValue");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", DataValue.c_str());  
        
        char szNOTE[4 * 1024]   = {0};
        CUtilTools::Utf8ToGb2312(szNOTE, 4096, DataValue.c_str(), DataValue.size());
        szNOTE[4095] = '\0';
        DataValue = szNOTE;            
        
    
        // "DataType":"1"  销售数据
        // "DataType":"2"  采购数据
        if (DataType == "1")
        {
        
            
            
            std::string source          = DataValue;
            std::vector<std::string>    dest;
            std::string division        = ",";
            SplitString(source, dest, division);
            
            if (dest.size() >= 29)
            {
                ;
            }
            else
            {
                int nLeft = 29 - dest.size();
                for (int i = 0; i < nLeft; ++i)
                {
                    std::string strEmpty = "";
                    dest.push_back(strEmpty);
                }
            }
            
            // data put in where
            // generate xml
            std::string AREA_ID     = gatherInfo.AREA_ID;
            std::string CHNL_NO     = gatherInfo.CHNL_NO;
            std::string I_E_TYPE    = gatherInfo.IE_FLAG;
            std::string SEQ_NO      = gatherInfo.SEQ_NO;    
            std::string MESSAGE_TYPE = "11";

            using boost::property_tree::ptree;
            ptree pt;
            ptree pattr1;
            std::string EXECUTE_COMMAND	= "00000000000000000000";

            pattr1.add<std::string>("<xmlattr>.AREA_ID", AREA_ID);
            pattr1.add<std::string>("<xmlattr>.CHNL_NO", CHNL_NO);
            pattr1.add<std::string>("<xmlattr>.I_E_TYPE", I_E_TYPE);
            pattr1.add<std::string>("<xmlattr>.SEQ_NO", SEQ_NO);

            pt.add_child("COMMAND_INFO", pattr1);

            pt.put("COMMAND_INFO.MESSAGE_TYPE", MESSAGE_TYPE);
            pt.put("COMMAND_INFO.EXECUTE_COMMAND", EXECUTE_COMMAND);

            pt.put("COMMAND_INFO.GPS.VE_NAME", "");
            pt.put("COMMAND_INFO.GPS.GPS_ID", "");
            pt.put("COMMAND_INFO.GPS.ORIGIN_CUSTOMS", "");
            pt.put("COMMAND_INFO.GPS.DEST_CUSTOMS", "");

            pt.put("COMMAND_INFO.SEAL.ESEAL_ID", "");
            pt.put("COMMAND_INFO.SEAL.SEAL_KEY", "");

            // ===================================================================
            // =====================================================================

            pt.put("COMMAND_INFO.FORM_TYPE", "1");
            pt.put("COMMAND_INFO.FORM_ID", "");
            pt.put("COMMAND_INFO.OP_HINT", "");  


            // 格式化输出，指定编码（默认utf-8）
            boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "GB2312");
            //write_xml(filename, pt, std::locale(), settings);

            ostringstream oss;
            write_xml(oss, pt, settings);

            strRetXml = oss.str();  
            syslog(LOG_DEBUG, "%s", strRetXml.c_str());    
            // end               
            
            
            
        }    
  }  
  catch (ptree_error & e)  
  {  
    return 2;  
  }  
  return 0;  
}  



int ParseIssueCardInfoJson(const std::string &strData)  
{  
//  std::string str = "{\"code\":0,\"images\":[{\"url\":\"fmn057/20111221/1130/head_kJoO_05d9000251de125c.jpg\"},{\"url\":\"fmn057/20111221/1130/original_kJoO_05d9000251de125c.jpg\"}]}"; 


//  std::string str = "{\"status\":\"OK\"}";
//  std::string str = "{\"status\":\"Fail\"}";
  
  using namespace boost::property_tree;  

  std::stringstream ss(strData);  
  ptree pt;  
  try{      
    read_json(ss, pt);  
  }  
  catch(ptree_error & e) {  
    return 1;   
  }  

  try
  {  
        std::string status = pt.get<std::string>("status");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", status.c_str());
        
        // ============================================================================
        std::string info = pt.get<std::string>("info");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", info.c_str());            
  }  
  catch (ptree_error & e)  
  {  
    return 2;  
  }  
  return 0;  
} 


int ParseBuysAndSalesWeightInfoJson(const structGatherInfo  &gatherInfo, const std::string &strData, std::string &strRetXml)  
{  
//  std::string str = "{\"code\":0,\"images\":[{\"url\":\"fmn057/20111221/1130/head_kJoO_05d9000251de125c.jpg\"},{\"url\":\"fmn057/20111221/1130/original_kJoO_05d9000251de125c.jpg\"}]}"; 


//  std::string str = "{\"status\":\"OK\",\"DataType\":\"1\",\"DataValue\":\"编号，日期，单位，车号，货名，发货单位，毛重，皮重，净重，扣重，实重，毛重司磅员，皮重司磅员，毛重时间，皮重时间，订单号\"}";
  
  using namespace boost::property_tree;  

  std::stringstream ss(strData);  
  ptree pt;  
  try{      
    read_json(ss, pt);  
  }  
  catch(ptree_error & e) {  
    return 1;   
  }  

  try
  {  
        std::string status = pt.get<std::string>("status");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", status.c_str());

        std::string DataType = pt.get<std::string>("DataType");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", DataType.c_str());      

        std::string DataValue = pt.get<std::string>("DataValue");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", DataValue.c_str());    
        
        char szNOTE[4 * 1024]   = {0};
        CUtilTools::Utf8ToGb2312(szNOTE, 4096, DataValue.c_str(), DataValue.size());
        szNOTE[4095] = '\0';
        DataValue = szNOTE;            
        
        
    
        // "DataType":"1"  采购过磅单
        // "DataType":"2"  销售过磅单
        if (DataType == "0")
        {
            std::string source          = DataValue;
            std::vector<std::string>    dest;
            std::string division        = ",";
            SplitString(source, dest, division);
            
            if (dest.size() >= 21)
            {
                ;
            }
            else
            {
                int nLeft = 21 - dest.size();
                for (int i = 0; i < nLeft; ++i)
                {
                    std::string strEmpty = "";
                    dest.push_back(strEmpty);
                }
            }            
            // generate xml
            std::string AREA_ID     = gatherInfo.AREA_ID;
            std::string CHNL_NO     = gatherInfo.CHNL_NO;
            std::string I_E_TYPE    = gatherInfo.IE_FLAG;
            std::string SEQ_NO      = gatherInfo.SEQ_NO;    
            std::string MESSAGE_TYPE = "16";

            using boost::property_tree::ptree;
            ptree pt;
            ptree pattr1;
            std::string EXECUTE_COMMAND	= "00000000000000000000";

            pattr1.add<std::string>("<xmlattr>.AREA_ID", AREA_ID);
            pattr1.add<std::string>("<xmlattr>.CHNL_NO", CHNL_NO);
            pattr1.add<std::string>("<xmlattr>.I_E_TYPE", I_E_TYPE);
            pattr1.add<std::string>("<xmlattr>.SEQ_NO", SEQ_NO);

            pt.add_child("COMMAND_INFO", pattr1);

            pt.put("COMMAND_INFO.MESSAGE_TYPE", MESSAGE_TYPE);
            pt.put("COMMAND_INFO.EXECUTE_COMMAND", EXECUTE_COMMAND);

            pt.put("COMMAND_INFO.GPS.VE_NAME", "");
            pt.put("COMMAND_INFO.GPS.GPS_ID", "");
            pt.put("COMMAND_INFO.GPS.ORIGIN_CUSTOMS", "");
            pt.put("COMMAND_INFO.GPS.DEST_CUSTOMS", "");

            pt.put("COMMAND_INFO.SEAL.ESEAL_ID", "");
            pt.put("COMMAND_INFO.SEAL.SEAL_KEY", "");

            // ===================================================================

            pt.put("COMMAND_INFO.FORM_TYPE", "1");
            pt.put("COMMAND_INFO.FORM_ID", "");
            pt.put("COMMAND_INFO.OP_HINT", "");  


            // 格式化输出，指定编码（默认utf-8）
            boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "GB2312");
            //write_xml(filename, pt, std::locale(), settings);

            ostringstream oss;
            write_xml(oss, pt, settings);

            strRetXml = oss.str();        
            // end                
        }
        
        
        
        
        if (DataType == "1")
        {
            std::string source          = DataValue;
            std::vector<std::string>    dest;
            std::string division        = ",";
            SplitString(source, dest, division);
            
            if (dest.size() >= 22)
            {
                ;
            }
            else
            {
                int nLeft = 22 - dest.size();
                for (int i = 0; i < nLeft; ++i)
                {
                    std::string strEmpty = "";
                    dest.push_back(strEmpty);
                }
            }            
            // generate xml
            std::string AREA_ID     = gatherInfo.AREA_ID;
            std::string CHNL_NO     = gatherInfo.CHNL_NO;
            std::string I_E_TYPE    = gatherInfo.IE_FLAG;
            std::string SEQ_NO      = gatherInfo.SEQ_NO;    
            std::string MESSAGE_TYPE = "13";

            using boost::property_tree::ptree;
            ptree pt;
            ptree pattr1;
            std::string EXECUTE_COMMAND	= "00000000000000000000";

            pattr1.add<std::string>("<xmlattr>.AREA_ID", AREA_ID);
            pattr1.add<std::string>("<xmlattr>.CHNL_NO", CHNL_NO);
            pattr1.add<std::string>("<xmlattr>.I_E_TYPE", I_E_TYPE);
            pattr1.add<std::string>("<xmlattr>.SEQ_NO", SEQ_NO);

            pt.add_child("COMMAND_INFO", pattr1);

            pt.put("COMMAND_INFO.MESSAGE_TYPE", MESSAGE_TYPE);
            pt.put("COMMAND_INFO.EXECUTE_COMMAND", EXECUTE_COMMAND);

            pt.put("COMMAND_INFO.GPS.VE_NAME", "");
            pt.put("COMMAND_INFO.GPS.GPS_ID", "");
            pt.put("COMMAND_INFO.GPS.ORIGIN_CUSTOMS", "");
            pt.put("COMMAND_INFO.GPS.DEST_CUSTOMS", "");

            pt.put("COMMAND_INFO.SEAL.ESEAL_ID", "");
            pt.put("COMMAND_INFO.SEAL.SEAL_KEY", "");

            // ===================================================================
            // =====================================================================

            pt.put("COMMAND_INFO.FORM_TYPE", "1");
            pt.put("COMMAND_INFO.FORM_ID", "");
            pt.put("COMMAND_INFO.OP_HINT", "");  


            // 格式化输出，指定编码（默认utf-8）
            boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "GB2312");
            //write_xml(filename, pt, std::locale(), settings);

            ostringstream oss;
            write_xml(oss, pt, settings);

            strRetXml = oss.str();        
            // end               
        }
        
        if (DataType == "3")
        {
            std::string source          = DataValue;
            std::vector<std::string>    dest;
            std::string division        = ",";
            SplitString(source, dest, division);
            
            if (dest.size() >= 21)
            {
                ;
            }
            else
            {
                int nLeft = 21 - dest.size();
                for (int i = 0; i < nLeft; ++i)
                {
                    std::string strEmpty = "";
                    dest.push_back(strEmpty);
                }
            }            
            // generate xml
            std::string AREA_ID     = gatherInfo.AREA_ID;
            std::string CHNL_NO     = gatherInfo.CHNL_NO;
            std::string I_E_TYPE    = gatherInfo.IE_FLAG;
            std::string SEQ_NO      = gatherInfo.SEQ_NO;    
            std::string MESSAGE_TYPE = "15";

            using boost::property_tree::ptree;
            ptree pt;
            ptree pattr1;
            std::string EXECUTE_COMMAND	= "00000000000000000000";

            pattr1.add<std::string>("<xmlattr>.AREA_ID", AREA_ID);
            pattr1.add<std::string>("<xmlattr>.CHNL_NO", CHNL_NO);
            pattr1.add<std::string>("<xmlattr>.I_E_TYPE", I_E_TYPE);
            pattr1.add<std::string>("<xmlattr>.SEQ_NO", SEQ_NO);

            pt.add_child("COMMAND_INFO", pattr1);

            pt.put("COMMAND_INFO.MESSAGE_TYPE", MESSAGE_TYPE);
            pt.put("COMMAND_INFO.EXECUTE_COMMAND", EXECUTE_COMMAND);

            pt.put("COMMAND_INFO.GPS.VE_NAME", "");
            pt.put("COMMAND_INFO.GPS.GPS_ID", "");
            pt.put("COMMAND_INFO.GPS.ORIGIN_CUSTOMS", "");
            pt.put("COMMAND_INFO.GPS.DEST_CUSTOMS", "");

            pt.put("COMMAND_INFO.SEAL.ESEAL_ID", "");
            pt.put("COMMAND_INFO.SEAL.SEAL_KEY", "");

            pt.put("COMMAND_INFO.FORM_TYPE", "1");
            pt.put("COMMAND_INFO.FORM_ID", "");
            pt.put("COMMAND_INFO.OP_HINT", "");  


            // 格式化输出，指定编码（默认utf-8）
            boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "GB2312");
            //write_xml(filename, pt, std::locale(), settings);

            ostringstream oss;
            write_xml(oss, pt, settings);

            strRetXml = oss.str();        
            // end               
        }        
        
        
        if (DataType == "2")
        {
            std::string source          = DataValue;
            std::vector<std::string>    dest;
            std::string division        = ",";
            SplitString(source, dest, division);
            
            
            if (dest.size() >= 37)
            {
                ;
            }
            else
            {
                int nLeft = 37 - dest.size();
                for (int i = 0; i < nLeft; ++i)
                {
                    std::string strEmpty = "";
                    dest.push_back(strEmpty);
                }
            }                 
            // generate xml
            std::string AREA_ID     = gatherInfo.AREA_ID;
            std::string CHNL_NO     = gatherInfo.CHNL_NO;
            std::string I_E_TYPE    = gatherInfo.IE_FLAG;
            std::string SEQ_NO      = gatherInfo.SEQ_NO;    
            std::string MESSAGE_TYPE = "14";

            using boost::property_tree::ptree;
            ptree pt;
            ptree pattr1;
            std::string EXECUTE_COMMAND	= "00000000000000000000";

            pattr1.add<std::string>("<xmlattr>.AREA_ID", AREA_ID);
            pattr1.add<std::string>("<xmlattr>.CHNL_NO", CHNL_NO);
            pattr1.add<std::string>("<xmlattr>.I_E_TYPE", I_E_TYPE);
            pattr1.add<std::string>("<xmlattr>.SEQ_NO", SEQ_NO);

            pt.add_child("COMMAND_INFO", pattr1);

            pt.put("COMMAND_INFO.MESSAGE_TYPE", MESSAGE_TYPE);
            pt.put("COMMAND_INFO.EXECUTE_COMMAND", EXECUTE_COMMAND);

            pt.put("COMMAND_INFO.GPS.VE_NAME", "");
            pt.put("COMMAND_INFO.GPS.GPS_ID", "");
            pt.put("COMMAND_INFO.GPS.ORIGIN_CUSTOMS", "");
            pt.put("COMMAND_INFO.GPS.DEST_CUSTOMS", "");

            pt.put("COMMAND_INFO.SEAL.ESEAL_ID", "");
            pt.put("COMMAND_INFO.SEAL.SEAL_KEY", "");

            // ===================================================================
            // =====================================================================
            pt.put("COMMAND_INFO.FORM_TYPE", "1");
            pt.put("COMMAND_INFO.FORM_ID", "");
            pt.put("COMMAND_INFO.OP_HINT", "");  


            // 格式化输出，指定编码（默认utf-8）
            boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "GB2312");
            //write_xml(filename, pt, std::locale(), settings);

            ostringstream oss;
            write_xml(oss, pt, settings);

            strRetXml = oss.str();        
            // end            
        }    
  }  
  catch (ptree_error & e)  
  {  
    return 2;  
  }  
  return 0;  
}  


int ParseVRInfoJson(const std::string &strData)  
{  
    return 0;  
}  


int ParseValidateCarPassRetInfo(const structGatherInfo  &gatherInfo, const std::string &strData, std::string &strRetXml, structValidateResult &result)  
{  
//  std::string str = "{\"code\":0,\"images\":[{\"url\":\"fmn057/20111221/1130/head_kJoO_05d9000251de125c.jpg\"},{\"url\":\"fmn057/20111221/1130/original_kJoO_05d9000251de125c.jpg\"}]}"; 
//  std::string str = "{\"status\":\"OK\",\"DataType\":\"1\",\"DataValue\":\"购货单位,车号,司机,手机,产品名称,规格型号,单位,计划数量,订单号1,订单号1剩余量,订单号2,订单号2剩余量,订单号3,订单号3剩余量,订单号4,订单号4剩余量,订单号5,订单号5剩余量,订单号6,订单号6剩余量,订单号7,订单号7剩余量,订单号8,订单号8剩余量,订单号9,订单号9剩余量,订单号10,订单号10剩余量,是否允许发卡\"}";

//			std::string strFileName = "/root/taian-test/222222.data";
//			FILE *pFile	= fopen(strFileName.c_str(), "wb");
//			if (pFile == NULL)
//			{
//				;
//			}
//			int nReadFileSize = fwrite(str.c_str(), 1, str.size(), pFile);
//                        {
//                            fclose(pFile);   
//                        }  
    
    
    //std::string str = "{\"success\":\"false\",\"message\":\"车牌号为空\",\"code\":\"params_empty\"}";
  
    std::string str = "";
    int nStart = strData.find("{", 0);
    int nEnd   = strData.find("}", 0);
    if (nStart >= 0 && nEnd >= 0)
    {
        str = strData.substr(nStart, nEnd - nStart + 1);
    }
    else
    {
        return -1;
    }
    
    std::string strsss = str;
    
//        char szNOTE[4 * 1024]   = {0};
//        CUtilTools::Utf8ToGb2312(szNOTE, 4096, strsss.c_str(), strsss.size());
//        szNOTE[4095] = '\0';
//        strsss = szNOTE;       
    
    
  
  using namespace boost::property_tree;  

  std::stringstream ss(strsss);
//std::stringstream ss(str);    
  ptree pt;  
  try
  {      
    read_json(ss, pt);  
  }  
  catch(ptree_error & e) 
  {  
      syslog(LOG_DEBUG, "%s", e.what());    
    return 1;   
  }  

  try
  {  
        std::string success = pt.get<std::string>("success");   // 得到"code"的value  
        result.success = success;
        syslog(LOG_DEBUG, "%s", success.c_str());

        std::string code = pt.get<std::string>("code");   // 得到"code"的value  
        result.code = code;
        syslog(LOG_DEBUG, "%s", code.c_str());    

        std::string message = pt.get<std::string>("message");   // 得到"code"的value  
        result.message = message;
        syslog(LOG_DEBUG, "%s", message.c_str());  
        
        char szNOTE[4 * 1024]   = {0};
        CUtilTools::Utf8ToGb2312(szNOTE, 4096, message.c_str(), message.size());
        szNOTE[4095] = '\0';
        message = szNOTE;            
        
        std::string EXECUTE_COMMAND	= "";
        std::string OP_HINT             = message;
        if (success == "true")
        {   
            if (code == "0")
                EXECUTE_COMMAND	= "00000000000000000000";
            else
                EXECUTE_COMMAND	= "11000000000000000000";
        } 
        else if (success == "false")
        {
            EXECUTE_COMMAND	= "11000000000000000000";
        }
        

        // data put in where
        // generate xml
        std::string AREA_ID     = gatherInfo.AREA_ID;
        std::string CHNL_NO     = gatherInfo.CHNL_NO;
        std::string I_E_TYPE    = gatherInfo.IE_FLAG;
        std::string SEQ_NO      = gatherInfo.SEQ_NO;    

        using boost::property_tree::ptree;
        ptree pt;
        ptree pattr1;
        

        pattr1.add<std::string>("<xmlattr>.AREA_ID", AREA_ID);
        pattr1.add<std::string>("<xmlattr>.CHNL_NO", CHNL_NO);
        pattr1.add<std::string>("<xmlattr>.I_E_TYPE", I_E_TYPE);
        pattr1.add<std::string>("<xmlattr>.SEQ_NO", SEQ_NO);

        pt.add_child("COMMAND_INFO", pattr1);

        pt.put("COMMAND_INFO.CHECK_RESULT", EXECUTE_COMMAND);

        pt.put("COMMAND_INFO.GPS.VE_NAME", "");
        pt.put("COMMAND_INFO.GPS.GPS_ID", "");
        pt.put("COMMAND_INFO.GPS.ORIGIN_CUSTOMS", "");
        pt.put("COMMAND_INFO.GPS.DEST_CUSTOMS", "");

        pt.put("COMMAND_INFO.SEAL.ESEAL_ID", "");
        pt.put("COMMAND_INFO.SEAL.SEAL_KEY", "");

        // ===================================================================
        // =====================================================================

        pt.put("COMMAND_INFO.FORM_TYPE", "1");
        pt.put("COMMAND_INFO.FORM_ID", "");
        pt.put("COMMAND_INFO.OP_HINT", OP_HINT);  


        // 格式化输出，指定编码（默认utf-8）
        boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "GB2312");
        //write_xml(filename, pt, std::locale(), settings);

        ostringstream oss;
        write_xml(oss, pt, settings);

        strRetXml = oss.str();  
        //syslog(LOG_DEBUG, "%s", strRetXml.c_str());    
        // end                 
  }  
  catch (ptree_error & e)  
  {  
    return 2;  
  }  
  return 0;  
}  



int ParseCarPassLogRetInfo(const structGatherInfo  &gatherInfo, const std::string &strData, std::string &strRetXml)  
{  
//  std::string str = "{\"code\":0,\"images\":[{\"url\":\"fmn057/20111221/1130/head_kJoO_05d9000251de125c.jpg\"},{\"url\":\"fmn057/20111221/1130/original_kJoO_05d9000251de125c.jpg\"}]}"; 
//  std::string str = "{\"status\":\"OK\",\"DataType\":\"1\",\"DataValue\":\"购货单位,车号,司机,手机,产品名称,规格型号,单位,计划数量,订单号1,订单号1剩余量,订单号2,订单号2剩余量,订单号3,订单号3剩余量,订单号4,订单号4剩余量,订单号5,订单号5剩余量,订单号6,订单号6剩余量,订单号7,订单号7剩余量,订单号8,订单号8剩余量,订单号9,订单号9剩余量,订单号10,订单号10剩余量,是否允许发卡\"}";

//			std::string strFileName = "/root/taian-test/222222.data";
//			FILE *pFile	= fopen(strFileName.c_str(), "wb");
//			if (pFile == NULL)
//			{
//				;
//			}
//			int nReadFileSize = fwrite(str.c_str(), 1, str.size(), pFile);
//                        {
//                            fclose(pFile);   
//                        }  
    std::string str = "";
    int nStart = strData.find("{", 0);
    int nEnd   = strData.find("}", 0);
    if (nStart >= 0 && nEnd >= 0)
    {
        str = strData.substr(nStart, nEnd - nStart + 1);
    }
    else
    {
        return -1;
    }
    
    std::string strsss = str;  
  
  
  using namespace boost::property_tree;  

  std::stringstream ss(strsss);
//std::stringstream ss(str);    
  ptree pt;  
  try{      
    read_json(ss, pt);  
  }  
  catch(ptree_error & e) 
  {  
      syslog(LOG_DEBUG, "%s", e.what());    
    return 1;   
  }  

  try
  {  
        std::string success = pt.get<std::string>("success");   // 得到"code"的value  
        syslog(LOG_DEBUG, "%s", success.c_str());

//        std::string DataType = pt.get<std::string>("code");   // 得到"code"的value  
//        syslog(LOG_DEBUG, "%s", DataType.c_str());    
        
        if (success == "false")
        {
            std::string message = pt.get<std::string>("message");   // 得到"code"的value  
            syslog(LOG_DEBUG, "%s", message.c_str());  
            
            char szNOTE[1024]   = {0};
            CUtilTools::Utf8ToGb2312(szNOTE, 1024, message.c_str(), message.size());
            szNOTE[1023] = '\0';
            message = szNOTE;              
        }           
  }  
  catch (ptree_error & e)  
  {  
    return 2;  
  }  
  return 0;  
}  

int SendCarPassLog(CValidateCarPass &pass)
{
    char szTime[64 + 1] = {0};
    time_t t = time(0);
    sprintf(szTime, "%ld", (long)t);
    CCarPassLog log;

    std::string seqNo       = pass.getSeqNo();
    std::string carNo       = pass.getCarNo();
    std::string stationNo   = pass.getStationNo();
    std::string channelNo   = pass.getChannelNo();
    std::string type        = pass.getType();
    std::string carCapturePic = "";                 // pass.getCarCapturePic();
    std::string time        = szTime;
    std::string eTagNo      = pass.geteTagNo();
    std::string ic          = pass.getIc();

    log.setSeqNo(seqNo);
    log.setCarNo(carNo);
    log.setStationNo(stationNo);
    log.setChannelNo(channelNo);
    log.setType(type);
    log.setCarCapturePic(carCapturePic);
    log.setTime(time);
    log.seteTagNo(eTagNo);
    log.setIc(ic);

    json_parser parser;
    std::string strRequestData = parser.generateCarPassLog(log); 
    
    
    std::string strRetXml = "";
    HttpRequest Http;

    boost::shared_array<char> spBody;
    spBody.reset(new char[BUFSIZE + 1]);
    char *pBody = spBody.get();        
    memset(pBody, 0, BUFSIZE + 1);


    char szRequestUrl[1024] = {0};

    std::string USER_SERIALNO = "";
    sprintf(szRequestUrl, "http://%s/is/carPassLog", g_configInfo.strWcfURLHandle.c_str());                          

    syslog(LOG_DEBUG, "%s\n", szRequestUrl);


    // if(Http.HttpGet(szRequestUrl, pBody)) 
    if (Http.HttpPost(szRequestUrl, strRequestData.c_str(), pBody))
    {
            syslog(LOG_DEBUG, "%s\n", pBody);
    } 
    else 
    {
            syslog(LOG_DEBUG, " HttpGet request failed!\n");
    }      

    char * body_ptr = NULL;
    if ((body_ptr = strstr(pBody, "\r\n\r\n")) != NULL)
    {
        body_ptr += 4;
        if (*body_ptr != 0)
        {
            std::string strData = body_ptr;

            syslog(LOG_DEBUG, "%s\n", strData.c_str());
            //ParseSalesAndBuysInfoJson(tmpGatherInfo, strData, strRetXml);
            
            structGatherInfo  tmpGatherInfo;
            ParseCarPassLogRetInfo(tmpGatherInfo, strData, strRetXml);

            if (1)
            {
                ;
            }
        }
    }
    else
    {
        ;
    }    
    
    
    
    return 0;
}



#ifdef	_WIN32
void ClientProc(void * pParam)
#else
void * ClientProc(void * pParam)
#endif
{
//	客户端处理线程,把接收的数据发送给目标机器,并把目标机器返回的数据返回到客户端
    CLIENT_PARAM * localParam=(CLIENT_PARAM *) pParam;
	localParam->nEndFlag=0;
	int nSocket=localParam->nSocket;

	int nLen=0;
#if	defined(_WIN32)||defined(_WIN64)
	MSG msg;
#endif
	int nLoopTotal=0;
	int nLoopMax=20*300;	//	300 秒判断选循环
#define	nMaxLen 0x1000
	char pBuffer[nMaxLen+1];
	char pNewBuffer[nMaxLen+1];
	memset(pBuffer,0,nMaxLen);
	memset(pNewBuffer,0,nMaxLen);
	int nSocketErrorFlag=0;

	int nNewLen=0;
	int nNewSocket=CreateSocket();
	if(nNewSocket==-1)
	{
//	不能建立套接字，直接返回
		CloseSocket(nSocket);
		EndClient(pParam);
	}
	if(ConnectSocket(nNewSocket,GlobalRemoteHost,GlobalRemotePort)<=0)
	{
//	不能建立连接，直接返回
		CloseSocket(nSocket);
		CloseSocket(nNewSocket);
		EndClient(pParam);
	}
	SetSocketNotBlock(nNewSocket);
	while(!GlobalStopFlag&&!nSocketErrorFlag)
	{
		nLoopTotal++;
#if	defined(_WIN32)||defined(_WIN64)		
		while(PeekMessage(&msg,NULL,0,0,PM_NOREMOVE))
		{
			if(GetMessage(&msg,NULL,0,0)!=-1)
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg);
			}
		}
#endif
		nLen=SocketRead(nSocket,pBuffer,nMaxLen);	
//	读取客户端数据
		if(nLen>0 && pBuffer[8] == 0x21)
		{
			pBuffer[nLen]=0;
			nLoopTotal=0;

			char szStationNo[10 + 1] = {0};
			char szChnlNo[10 + 1]	 = {0};

			memcpy(szStationNo, pBuffer + 9, 10);
			memcpy(szChnlNo,    pBuffer + 19, 10);


			char szIEFlag[2] = {pBuffer[29], '\0'};
			unsigned char szXmlLen[4] = {0};
			szXmlLen[0] = pBuffer[34];
			szXmlLen[1] = pBuffer[35];
			szXmlLen[2] = pBuffer[36];
			szXmlLen[3] = pBuffer[37];
			int nXmlLen2 = 0;
			nXmlLen2 = (unsigned int)szXmlLen[0] + (unsigned int)(szXmlLen[1] << 8) + (unsigned int)(szXmlLen[2] << 16) + (unsigned int)(szXmlLen[3] << 24);
			syslog(LOG_DEBUG, "nXmlLen2=%d\n", nXmlLen2);
			if (nXmlLen2 <= 0)
			{
				break;
			}

			boost::shared_array<char> spRecvXmlData;
			spRecvXmlData.reset(new char[nXmlLen2 + 1]);
			char *pRecvXmlData = spRecvXmlData.get();
			memcpy(pRecvXmlData, pBuffer + 38, nXmlLen2);
			pRecvXmlData[nXmlLen2] = '\0';

			//syslog(LOG_DEBUG, "%s\n", pRecvXmlData);

			std::string AREA_ID		= "";
			std::string CHNL_NO		= "";
			std::string I_E_TYPE            = "";
			std::string SEQ_NO		= "";

			TiXmlDocument doc;
			doc.Parse(pRecvXmlData);
			TiXmlHandle docHandle(&doc);
			TiXmlHandle GATHER_INFOHandle		= docHandle.FirstChildElement("GATHER_INFO");
			if (GATHER_INFOHandle.Node() != NULL)
				AREA_ID 		= GATHER_INFOHandle.Element()->Attribute("AREA_ID");
			if (GATHER_INFOHandle.Node() != NULL)
				CHNL_NO 		= GATHER_INFOHandle.Element()->Attribute("CHNL_NO");
			if (GATHER_INFOHandle.Node() != NULL)
				I_E_TYPE 		= GATHER_INFOHandle.Element()->Attribute("I_E_TYPE");
			if (GATHER_INFOHandle.Node() != NULL)
				SEQ_NO 		= GATHER_INFOHandle.Element()->Attribute("SEQ_NO");
                        
                        structGatherInfo  tmpGatherInfo;
                        tmpGatherInfo.AREA_ID       = AREA_ID;
                        tmpGatherInfo.CHNL_NO       = CHNL_NO;
                        tmpGatherInfo.IE_FLAG       = I_E_TYPE;
                        tmpGatherInfo.SEQ_NO        = SEQ_NO;

			// start
			//TiXmlHandle PCAR_NOHandle   = docHandle.FirstChildElement("GATHER_INFO").ChildElement("OPTCAR", 0).ChildElement("PCAR_NO", 0).FirstChild();
                        TiXmlHandle DR_IC_NOHandle  = docHandle.FirstChildElement("GATHER_INFO").ChildElement("IC", 0).ChildElement("DR_IC_NO", 0).FirstChild();
                        TiXmlHandle CAR_EC_NOHandle = docHandle.FirstChildElement("GATHER_INFO").ChildElement("CAR", 0).ChildElement("CAR_EC_NO", 0).FirstChild();
                        TiXmlHandle PCAR_PICNAMEHandle = docHandle.FirstChildElement("GATHER_INFO").ChildElement("OPTCAR", 0).ChildElement("PCAR_PICNAME", 0).FirstChild();
                        TiXmlHandle VE_NAMEHandle = docHandle.FirstChildElement("GATHER_INFO").ChildElement("CAR", 0).ChildElement("VE_NAME", 0).FirstChild();
                                             
                        
                        std::string PCAR_NO             = "";
                        std::string CAR_EC_NO           = "";
                        std::string DR_IC_NO            = "";
                        std::string PCAR_PICNAME        = "";
                        std::string VE_NAME             = "";

//			if (PCAR_NOHandle.Node() != NULL)
//				PCAR_NO	= PCAR_NOHandle.Text()->Value();   
                        
			if (CAR_EC_NOHandle.Node() != NULL)
				CAR_EC_NO	= CAR_EC_NOHandle.Text()->Value();                         
                  
			if (DR_IC_NOHandle.Node() != NULL)
				DR_IC_NO	= DR_IC_NOHandle.Text()->Value();                         
        		if (PCAR_PICNAMEHandle.Node() != NULL)
				PCAR_PICNAME	= PCAR_PICNAMEHandle.Text()->Value();                 
         		if (VE_NAMEHandle.Node() != NULL)
				VE_NAME	= VE_NAMEHandle.Text()->Value();                         
                        
                        if (PCAR_PICNAME != "")
                        {
                            // get pic data
                        }
                        
//                        SendCarPassLog();
//===================================================================================================
                        
        char szNOTE[100]   = {0};
        if (VE_NAME != "")
        {
            CUtilTools::Gb2312ToUtf8(szNOTE, 100, VE_NAME.c_str(), VE_NAME.size());
            szNOTE[99] = '\0';
            VE_NAME = szNOTE;   
        }
             
//        
//std::string strFileName = "/root/recvpic/3711020004/20151130/15/3711020004_20151130153724228301_B.jpg";
//			unsigned long nFileSize = GetFileSize(strFileName.c_str());
//                        
//                        char *pImageData = new char[nFileSize + 1];
//			FILE *pFile	= fopen(strFileName.c_str(), "rb");
//			if (pFile == NULL)
//			{
//				;
//			}
//			int nReadFileSize = fread(pImageData, 1, nFileSize, pFile);
//			if (nReadFileSize != nFileSize)
//			{
//				fclose(pFile);
//			}
//                        else
//                        {
//                            fclose(pFile);   
//                        }   
//                        JZBase64 base64;
//                        std::string strImageData = base64.Encode((const unsigned char *)pImageData, nFileSize);
//                
//        
//        PCAR_PICNAME = strImageData;
        
        //VE_NAME="&#x95FD;D H73482";
                        
                        
	CValidateCarPass pass;

        std::string seqNo       = SEQ_NO;
	std::string carNo       = VE_NAME; // PCAR_NO;
	std::string stationNo   = AREA_ID;
	std::string channelNo   = CHNL_NO;
	std::string type        = (I_E_TYPE == "E" ? "0":"1");
	std::string carCapturePic = PCAR_PICNAME;
	std::string eTagNo      = CAR_EC_NO;
	std::string ic          = DR_IC_NO;
	
        pass.setSeqNo(seqNo);
	pass.setCarNo(carNo);
	pass.setStationNo(stationNo);
	pass.setChannelNo(channelNo);
	pass.setType(type);
	pass.setCarCapturePic(carCapturePic);
	pass.seteTagNo(eTagNo);
	pass.setIc(ic);
	
        json_parser parser;
	std::string strRequestData = parser.generateValidateCarPass(pass);
        
        
//=====================================================================================================                        
//        char szNOTE[4096]   = {0};
//        if (VE_NAME != "")
//        {
//            //CUtilTools::Gb2312ToUtf8(szNOTE, 100, VE_NAME.c_str(), VE_NAME.size());
//            CUtilTools::Gb2312ToUtf8(szNOTE, 4096, strRequestData.c_str(), strRequestData.size());
//            szNOTE[4095] = '\0';
//            strRequestData = szNOTE;   
//        }
        
   
        
			
        
        
        
                        std::string strRetXml = "";
                        HttpRequest Http;

                        boost::shared_array<char> spBody;
                        spBody.reset(new char[BUFSIZE + 1]);
                        char *pBody = spBody.get();        
                        memset(pBody, 0, BUFSIZE + 1);
                        
                        
                        char szRequestUrl[1024] = {0};

                        sprintf(szRequestUrl, "http://%s/is/validateCarPass", \
                            g_configInfo.strWcfURLHandle.c_str()
                            );                          
                        
                        syslog(LOG_DEBUG, "%s\n", szRequestUrl);

                        // if(Http.HttpGet(szRequestUrl, pBody)) 
                        if (Http.HttpPost(szRequestUrl, strRequestData.c_str(), pBody))
                        {
                                syslog(LOG_DEBUG, "%s\n", pBody);
                        } 
                        else 
                        {
                                syslog(LOG_DEBUG, " HttpGet request failed!\n");
                        }      

                        char * body_ptr = NULL;
                        if ((body_ptr = strstr(pBody, "\r\n\r\n")) != NULL)
                        {
                            body_ptr += 4;
                            if (*body_ptr != 0)
                            {
                                std::string strData = body_ptr;

                                syslog(LOG_DEBUG, "%s\n", strData.c_str());
                                
                                structValidateResult result;
                                ParseValidateCarPassRetInfo(tmpGatherInfo, strData, strRetXml, result);                                
                                
                                if (result.code == "0")
                                {
                                    SendCarPassLog(pass);
                                }
                            }
                        }
                        else
                        {
                            ;
                        }
        
                        
			// start
                        if (strRetXml == "")
                            break;

			std::string strSendXml = strRetXml;

			const char packetHead[4]={(char)0xE2, (char)0x5C, (char)0x4B, (char)0x89};
			const char packetEnd[2]={(char)0xFF, (char)0xFF};

			std::string strStationNo = szStationNo;
			std::string strChnlNo	 = szChnlNo;

//			char szCommandInfo[4096] = {0};
//
//			sprintf(szCommandInfo, "<COMMAND_INFO AREA_ID=\"%s\" CHNL_NO=\"%s\" I_E_TYPE=\"%s\" SEQ_NO=\"%s\">\r\n"
//"<EXCUTE_COMMAND>00000000000000000000</EXCUTE_COMMAND>\r\n"
//"<GPS>\r\n"
//"<VE_NAME></VE_NAME>\r\n"
//"<GPS_ID></GPS_ID>\r\n"
//"<ORIGIN_CUSTOMS></ORIGIN_CUSTOMS>\r\n"
//"<DEST_CUSTOMS></DEST_CUSTOMS>\r\n"
//"</GPS>\r\n"
//"<SEAL>\r\n"
//"<ESEAL_ID></ESEAL_ID>\r\n"
//"<SEAL_KEY></SEAL_KEY>\r\n"
//"<OPEN_TIMES></OPEN_TIMES>\r\n"
//"</SEAL>\r\n"
//"<OP_TYPE>0</OP_TYPE>\r\n"
//"<OP_REASON>放行抬杆</OP_REASON>\r\n"
//"<OP_ID>0</OP_ID>\r\n"
//"</COMMAND_INFO>\r\n", strStationNo.c_str(), strChnlNo.c_str(), I_E_TYPE.c_str(), SEQ_NO.c_str());


//			TestConfig tc;
//			std::string strFileName = "/root/zhouhm/test/";
//			strFileName += SEQ_NO;
//			strFileName += ".xml";
//			tc.save2(strFileName, AREA_ID, CHNL_NO, I_E_TYPE, SEQ_NO);
//
//			//std::string strFileName = "/root/zhouhm/FutureVersion/NNSimulationPlatform_UTF8/FangXin.xml";
//			unsigned long nFileSize = GetFileSize(strFileName.c_str());
//			FILE *pFile	= fopen(strFileName.c_str(), "rb");
//			if (pFile == NULL)
//			{
//				break;
//			}
//			int nReadFileSize = fread(szCommandInfo, 1, nFileSize, pFile);
//			if (nReadFileSize != nFileSize)
//			{
//				fclose(pFile);
//				break;
//			}
//			fclose(pFile);
//			remove(strFileName.c_str());
//			
//
//
//			// 抬杆放行
//			syslog(LOG_DEBUG, "%s\n", szCommandInfo);
//			syslog(LOG_DEBUG, "======================================================================\n");
//			syslog(LOG_DEBUG, "======================================================================\n");
//			syslog(LOG_DEBUG, "======================================================================\n");


			//strSendXml = szCommandInfo;

			//int nXmlLen = nFileSize; // strSendXml.size();
                        int nXmlLen = strSendXml.size();
			int PackDataLen = 34 + 4 + nXmlLen + 2;
			syslog(LOG_DEBUG, "PackDataLen=%d\n", PackDataLen);

			boost::shared_array<BYTE> spPacketData;
			spPacketData.reset(new BYTE[PackDataLen + 1]);
			bzero(spPacketData.get(), PackDataLen + 1);

			BYTE *p = spPacketData.get();
			memcpy(p, packetHead ,4);
			p += 4;

			memcpy(p, (BYTE*)&PackDataLen, sizeof(int));
			p += 4;

			char szDataType[2] = {0x22, 0x00};
			memcpy(p, szDataType, 1);
			p += 1;	

			memcpy(p, strStationNo.data(), strStationNo.size());
			p += 10;

			memcpy(p, strChnlNo.data(), strChnlNo.size());
			p += 10;

			memcpy(p, szIEFlag, 1);
			p += 1;							

			memset(p, 0x00, 4);
			p += 4;	

			memcpy(p, &nXmlLen, 4);
			p+=4;
			memcpy(p, strSendXml.data(), nXmlLen);
			//memcpy(p, szCommandInfo, nXmlLen);
			p += nXmlLen;
			memcpy(p, packetEnd ,2);
			// end

			//nNewLen=SocketWrite(nNewSocket,pBuffer,nLen,30);
			nNewLen=SocketWrite(nNewSocket, (char*)spPacketData.get(), PackDataLen, 30);
			if(nNewLen<0)	//	断开
				break;
		}
                if(nLen>0 && pBuffer[8] == 0x52)
                {
			;              
                }
		if(nLen>0 && pBuffer[8] == 0x53)
		{
			;
		}                
		if(nLen>0 && pBuffer[8] == 0x54)
		{
			;
		}                 
                
		if(nLen>0 && pBuffer[8] == 0x32)
		{
			pBuffer[nLen]=0;
			nLoopTotal=0;

			//if (pBuffer[8] != 0x21)
			//{
			//	break;
			//}

			char szStationNo[10 + 1] = {0};
			char szChnlNo[10 + 1]	 = {0};

			memcpy(szStationNo, pBuffer + 9, 10);
			memcpy(szChnlNo,    pBuffer + 19, 10);


			char szIEFlag[2] = {pBuffer[29], '\0'};
			unsigned char szXmlLen[4] = {0};
			szXmlLen[0] = pBuffer[34];
			szXmlLen[1] = pBuffer[35];
			szXmlLen[2] = pBuffer[36];
			szXmlLen[3] = pBuffer[37];
			int nXmlLen2 = 0;
			nXmlLen2 = (unsigned int)szXmlLen[0] + (unsigned int)(szXmlLen[1] << 8) + (unsigned int)(szXmlLen[2] << 16) + (unsigned int)(szXmlLen[3] << 24);
			syslog(LOG_DEBUG, "nXmlLen2=%d\n", nXmlLen2);
			if (nXmlLen2 <= 0)
			{
				break;
			}

			boost::shared_array<char> spRecvXmlData;
			spRecvXmlData.reset(new char[nXmlLen2 + 1]);
			char *pRecvXmlData = spRecvXmlData.get();
			memcpy(pRecvXmlData, pBuffer + 38, nXmlLen2);
			pRecvXmlData[nXmlLen2] = '\0';

			syslog(LOG_DEBUG, "%s\n", pRecvXmlData);
		}
		if(nLen<0)
		{
			//	读断开
			break;
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
	CloseSocket(nNewSocket);
	CloseSocket(nSocket);
	EndClient(pParam);
#ifdef	_WIN32
	Sleep(50);
	_endthread();
	return ;
#else
	SysSleep(50);
	pthread_exit(NULL);
	return NULL;
#endif
}


#ifdef	_WIN32
void AcceptProc(void * pParam)
#else
void * AcceptProc(void * pParam)
#endif
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
//	在Linux下,inet_ntoa()函数是线程不安全的
		MyUnlock();
		SetSocketNotBlock(s);
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
		localClientParam=new CLIENT_PARAM;
		localClientParam->nEndFlag=1;
		localClientParam->nIndexOff=nCurIndex;
		localClientParam->nSocket=s;
		localClientParam->pServerParam=localParam;
		memset(localClientParam->szClientName,0,256);
		strcpy(localClientParam->szClientName,pClientName);
		int nThreadErr=0;

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
	int nUsedSocket=MAX_SOCKET;
	while(nUsedSocket>0)
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
#ifdef	_WIN32
	Sleep(50);
	localParam->nEndFlag=1;
	_endthread();
    return ;
#else
	SysSleep(50);
	localParam->nEndFlag=1;
	pthread_exit(NULL);
	return NULL;
#endif
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

int main(int argc, char * argv[])
//int rsmain()
{
	openlog("yuanquproxy", LOG_PID, LOG_LOCAL4);
	ReadConfigInfo(g_configInfo);
        
////=============================================
//        std::string strData = "43\r\n\
//{\"success\":\"false\",\"message\":\"车牌号为空\",\"code\":\"params_empty\"}\r\n\
//0\r\n";
//        
//        char szNOTE[4 * 1024]   = {0};
//        CUtilTools::Gb2312ToUtf8(szNOTE, 4096, strData.c_str(), strData.size());
//        szNOTE[4095] = '\0';
//        strData = szNOTE;          
//        
//        
////    std::string str = "";
////    int nStart = strData.find("{", 0);
////    int nEnd   = strData.find("}", 0);
////    if (nStart >= 0 && nEnd >= 0)
////        str = strData.substr(nStart, nEnd - nStart + 1);
////    std::string strsss = str;
//structGatherInfo  tmpGatherInfo;
//std::string strRetXml;
//ParseValidateCarPassRetInfo(tmpGatherInfo, strData, strRetXml);
////===============================================        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
	//if(argc<4)
	//{
	//	PrintUsage(argv[0]);
	//	return -1;
	//}
	int nLocalPort	=-1;
	int nRemotePort	=-1;
	nLocalPort		= atoi(g_configInfo.strLocalPortHandle.c_str());
	nRemotePort		=  atoi(g_configInfo.strKaKouPortHandle.c_str());
	if ((nLocalPort<=0) || (nRemotePort<=0))
	{
		syslog(LOG_DEBUG, "invalid local port or remote port \n");
		return -1;
	}

	// char * szRemoteHost = "192.168.1.130"; // argv[2];
	GlobalRemotePort = nRemotePort;
	strcpy(GlobalRemoteHost, g_configInfo.strKaKouIPHandle.c_str());
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
	SetSocketNotBlock(nSocket);
	SERVER_PARAM * localServerParam=new SERVER_PARAM;
   	sockaddr_in name;
   	memset(&name,0,sizeof(sockaddr_in));
   	name.sin_family=AF_INET;
   	name.sin_port=htons((unsigned short)nLocalPort);
	name.sin_addr.s_addr=INADDR_ANY;
	localServerParam->ServerSockaddr=name;
	localServerParam->nSocket=nSocket;
	localServerParam->nEndFlag=1;
#if defined(_WIN32)||defined(_WIN64)
	uintptr_t	localThreadId;
#else
	pthread_t localThreadId;
#endif

	int nThreadErr=0;

#if	defined(_WIN32)||defined(_WIN64)
	if((localThreadId=_beginthread(AcceptProc,NULL,localServerParam))<=1)
		nThreadErr=1;
#else
    nThreadErr=pthread_create(&localThreadId,NULL,
            (THREADFUNC)AcceptProc,localServerParam);
	if(nThreadErr==0)
		pthread_detach(localThreadId);
//	释放线程私有数据,不必等待pthread_join();
#endif
	if(nThreadErr)
	{
//	建立线程失败
		MyUninitLock();
		CloseSocket(nSocket);
		UninitWinSock();
		delete localServerParam;
		return -1;
	}
	while((GlobalStopFlag==0)||(localServerParam->nEndFlag==0))
	{
		SysSleep(500);
	}
	MyUninitLock();
	CloseSocket(nSocket);
	UninitWinSock();
	delete localServerParam;
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

//#define SOFTWARE_VERSION  "version 1.0.0.0"         //软件版本号
//
//int main(int argc, char *argv[])
//{
//    int iRet;
//    int iStatus;
//    pid_t pid;
//    //显示版本号
//    if (argc == 2)
//    {
//        //如果是查看版本号
//        if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "-V") || !strcmp(argv[1], "-Version") || !strcmp(argv[1], "-version"))
//        {
//            printf("%s  %s\n", argv[0], SOFTWARE_VERSION);
//            return 0;
//        }
//    }
//    
//    
////    json_parser parser;
////    parser.tester();
//    
//    
//    
//    
//    
//    
//
//    Daemon();
//
//createchildprocess:
//    //开始创建子进程
//    //syslog(LOG_DEBUG, "begin to create	the child process of %s\n", argv[0]);
//
//    int itest = 0;
//    switch (fork())//switch(fork())
//    {
//        case -1 : //创建子进程失败
//            //syslog(LOG_DEBUG, "cs创建子进程失败\n");
//            return -1;
//        case 0://子进程
//            //syslog(LOG_DEBUG, "cs创建子进程成功\n");
//            rsmain();
//            return -1;
//        default://父进程
//            pid = ::wait(&iStatus);
//            //syslog(LOG_DEBUG, "子进程退出，5秒后重新启动......\n");
//            sleep(3);
//            goto createchildprocess; //重新启动进程
//            break;
//    }
//    return 0;
//}

