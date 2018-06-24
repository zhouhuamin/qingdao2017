#include "HttpRequest.h"
#include  <syslog.h>

using namespace std;

HttpRequest::HttpRequest()
{
    m_iSocketFd = INVALID_SOCKET;
}

HttpRequest::~HttpRequest()
{
	
}


/**
*	???��??述�?HttpGet请�?
*	???��?��??�??
*				strUrl�??     Http请�?URL
*				strResponse�??Http请�???�??
*	�?? ?? ?��?
*				1表示????
*				0表示失败
**/
int HttpRequest::HttpGet(const char* strUrl, char* strResponse)
{
	return HttpRequestExec("GET", strUrl, NULL, strResponse);
}


/**
*	???��??述�?HttpPost请�?
*	???��?��??�??
*				strUrl�??     Http请�?URL
*               strData�??    POST请�????????��??
*				strResponse�??Http请�???�??
*	�?? ?? ?��?
*				1表示????
*				0表示失败
**/
int HttpRequest::HttpPost(const char* strUrl, const char* strData, char* strResponse)
{
	return HttpRequestExec("POST", strUrl, strData, strResponse);
}


//?��?HTTP请�?�??GET??POST
int HttpRequest::HttpRequestExec(const char* strMethod, const char* strUrl, const char* strData, char* strResponse)
{
	//?��??URL????????
	if((strUrl == NULL) || (0 == strcmp(strUrl, ""))) 
        {
		syslog(LOG_DEBUG, "%s %s %d\tURL为空\n", __FILE__, __FUNCTION__, __LINE__); 
		return 0;
	}
	
	//????URL?�度
	if(URLSIZE < strlen(strUrl)) 
        {
		syslog(LOG_DEBUG, "%s %s %d\tURL???�度�???��?�??%d\n", __FILE__, __FUNCTION__, __LINE__, URLSIZE); 
		return 0;
	}
	
	//??�??HTTP??�??�??
	std::string strHttpHead = HttpHeadCreate(strMethod, strUrl, strData);
	
	//?��??�???��??m_iSocketFd????????�??????就�?��?��?????��??
	if(m_iSocketFd != INVALID_SOCKET) 
        {
            //�????SocketFd????为�????�????读�?��??
            if(SocketFdCheck(m_iSocketFd) > 0) 
            {
                std::string strResult = HttpDataTransmit(strHttpHead, m_iSocketFd);
                if("" != strResult) 
                {
                    strcpy(strResponse, strResult.c_str());
                    return 1;
                }
            }
	}

	//Create socket
	m_iSocketFd = INVALID_SOCKET;
	m_iSocketFd = socket(AF_INET, SOCK_STREAM, 0); 
        if (m_iSocketFd < 0 ) 
        { 
		syslog(LOG_DEBUG, "%s %s %d\tsocket error! Error code: %d�??Error message: %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno)); 
		return 0;
	}
  
	//Bind address and port
	int iPort = GetPortFromUrl(strUrl);
	if(iPort < 0) {
		syslog(LOG_DEBUG, "%s %s %d\t�??URL?��??�???�失�??\n", __FILE__, __FUNCTION__, __LINE__); 
		return 0;
	}	
	std::string strIP = GetIPFromUrl(strUrl);
	if(strIP == "") 
        {
		syslog(LOG_DEBUG, "%s %s %d\t�??URL?��??IP?��??失败\n", __FILE__, __FUNCTION__, __LINE__);
		return 0;
	}
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(iPort); 
	if (inet_pton(AF_INET, strIP.c_str(), &servaddr.sin_addr) <= 0 ) 
        { 
	  syslog(LOG_DEBUG, "%s %s %d\tinet_pton error! Error code: %d�??Error message: %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno)); 
	  close(m_iSocketFd);
	  m_iSocketFd = INVALID_SOCKET;
	  return 0; 
	}
	
	//Set non-blocking
	int flags = fcntl(m_iSocketFd, F_GETFL, 0);
	if(fcntl(m_iSocketFd, F_SETFL, flags|O_NONBLOCK) == -1) 
        {
		close(m_iSocketFd);
		m_iSocketFd = INVALID_SOCKET;
		syslog(LOG_DEBUG, "%s %s %d\tfcntl error! Error code: %d�??Error message: %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno)); 
		return 0;
	}

	//???��??��?�????
	int iRet = connect(m_iSocketFd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if(iRet == 0) 
        {
		std::string strResult = HttpDataTransmit(strHttpHead, m_iSocketFd);
		if("" != strResult) 
                {
			strcpy(strResponse, strResult.c_str());
			return 1;
		} 
                else 
                {
			close(m_iSocketFd);
                        m_iSocketFd = INVALID_SOCKET;
			return 0;
		}
	}
	else if(iRet < 0) 
        {
		if(errno != EINPROGRESS) 
                {
			return 0;
		}
	}
	
	iRet = SocketFdCheck(m_iSocketFd);
	if(iRet > 0) 
        {
            std::string strResult = HttpDataTransmit(strHttpHead, m_iSocketFd);
            if("" == strResult) 
            {
                close(m_iSocketFd);
                m_iSocketFd = INVALID_SOCKET;
                return 0;
            }
            else 
            {
                strcpy(strResponse, strResult.c_str());
                return 1;
            }
	}
	else 
        {
            close(m_iSocketFd);
            m_iSocketFd = INVALID_SOCKET;
            return 0;
	}
	
	return 1;
}


//??�??HTTP�????�??
std::string HttpRequest::HttpHeadCreate(const char* strMethod, const char* strUrl, const char* strData)
{
	std::string strHost = GetHostAddrFromUrl(strUrl);
	std::string strParam = GetParamFromUrl(strUrl);
	
        //char strHttpHead[2048] = {0};
        
        char *strHttpHead = new char[512 * 1024];
        memset(strHttpHead, 0x00, 512 * 1024);

	strcat(strHttpHead, strMethod); 
	strcat(strHttpHead, " /"); 
	strcat(strHttpHead, strParam.c_str());

	strcat(strHttpHead, " HTTP/1.1\r\n");
	strcat(strHttpHead, "Accept: */*\r\n"); 
        strcat(strHttpHead, "Accept-Charset: UTF-8\r\n"); 
	strcat(strHttpHead, "Accept-Language: cn\r\n"); 
	strcat(strHttpHead, "User-Agent: Mozilla/4.0\r\n");
	strcat(strHttpHead, "Host: "); 
	strcat(strHttpHead, strHost.c_str());
	strcat(strHttpHead, "\r\n");
	strcat(strHttpHead, "Cache-Control: no-cache\r\n"); 
	strcat(strHttpHead, "Connection: Close\r\n");
	if(0 == strcmp(strMethod, "POST"))
	{
		char len[8] = {0};
		unsigned uLen = strlen(strData);
		sprintf(len, "%d", uLen);		
		
		strcat(strHttpHead, "Content-Type: application/json;charset=UTF-8\r\n");
		strcat(strHttpHead, "Content-Length: "); 
                strcat(strHttpHead, len); 
                strcat(strHttpHead, "\r\n\r\n"); 
                strcat(strHttpHead, strData); 
	}
	strcat(strHttpHead, "\r\n\r\n");
	
        std::string strTmpHttpHead = strHttpHead;
        
        delete []strHttpHead;
        
	return strTmpHttpHead;	
}


////????HTTP请�?并�?��????�??
//char* HttpRequest::HttpDataTransmit(char *strHttpHead, const int iSockFd)
//{
//	char* buf = (char*)malloc(BUFSIZE);
//	memset(buf, 0, BUFSIZE);
//	int ret = send(iSockFd,(void *)strHttpHead,strlen(strHttpHead)+1,0); 
//	free(strHttpHead);
//	if (ret < 0) { 
//		syslog(LOG_DEBUG, "%s %s %d\tsend error! Error code: %d�??Error message: %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno)); 
//		close(iSockFd);
//		return NULL; 
//	}
//	
//	while(1)
//	{
//		ret = recv(iSockFd, (void *)buf, BUFSIZE,0); 
//		if (ret == 0) //�???��?��??
//		{  		
//			close(iSockFd);
//			return NULL; 
//		}
//		else if(ret > 0) {						
//			return buf; 
//		}
//		else if(ret < 0) //?��??
//		{ 
//			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
//				continue;
//			}
//			else {
//				close(iSockFd);
//				return NULL;
//			}
//		}
//	}
//}

//????HTTP请�?并�?��????�??
std::string HttpRequest::HttpDataTransmit(const std::string &strHttpHead, const int iSockFd)
{
        int len = 0;
        char buf[BUFSIZE + 1] = {0};
        memset(buf, 0x00, sizeof(buf));
        
        int nSendLen = strHttpHead.size();
        
	//int ret = send(iSockFd,(void *)strHttpHead.c_str(), strHttpHead.size(), 0); 
        
        int ret = SocketWrite(iSockFd, (char *)strHttpHead.c_str(), nSendLen, 30);
               
	if (ret < 0) 
        { 
		syslog(LOG_DEBUG, "%s %s %d\tsend error! Error code: %d�??Error message: %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno)); 
		close(iSockFd);
		return ""; 
	}
	
	while(1)
	{
		ret = recv(iSockFd, buf + len, BUFSIZE,0); 
		if (ret == 0) //�???��?��??
		{  		
			close(iSockFd);
                        buf[BUFSIZE] = '\0';
                        std::string strRetBuf = buf;
			return strRetBuf; 
		}
		else if(ret > 0) 
                {
                    len += ret;
                    
                    if (len >= BUFSIZE)
                    {
                        printf("len=%d\n", len);
                        return "";
                    }
                    buf[len] = '\0';
			//return buf; 
		}
		else if(ret < 0) //?��??
		{ 
			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) 
                        {
				continue;
			}
			else 
                        {
				close(iSockFd);
				return "";
			}
		}
	}
}




//�??HTTP请�?URL�???��??主�?��?��??�??�?????????��????�????IP?��??
std::string HttpRequest::GetHostAddrFromUrl(const char* strUrl)
{	
	char url[URLSIZE] = {0};
	strcpy(url, strUrl);
	
	char* strAddr = strstr(url, "http://");//?��????没�??http://
	if(strAddr == NULL) {
		strAddr = strstr(url, "https://");//?��????没�??https://
		if(strAddr != NULL) {
			strAddr += 8;
		}
	} else {
		strAddr += 7;
	}
	
	if(strAddr == NULL) {
		strAddr = url;
	}
	int iLen = strlen(strAddr);

        char strHostAddr[100] = {0};
        
        
	for(int i=0; i<iLen+1; i++) 
        {
		if(strAddr[i] == '/') 
                {
			break;
		} else 
                {
			strHostAddr[i] = strAddr[i];
		}
	}
        
        std::string strTmpHostAddr = strHostAddr;

	return strTmpHostAddr;
}


//�??HTTP请�?URL�???��??HTTP请�????
std::string HttpRequest::GetParamFromUrl(const char* strUrl)
{	
	char url[URLSIZE] = {0};
	strcpy(url, strUrl);
	
	char* strAddr = strstr(url, "http://");//?��????没�??http://
	if(strAddr == NULL) 
        {
		strAddr = strstr(url, "https://");//?��????没�??https://
		if(strAddr != NULL) 
                {
			strAddr += 8;
		}
	} 
        else 
        {
		strAddr += 7;
	}
	
	if(strAddr == NULL) 
        {
		strAddr = url;
	}
	int iLen = strlen(strAddr);        
        
        char strParam[2048] = {0};
        
	int iPos = -1;
	for(int i=0; i<iLen+1; i++) 
        {
		if(strAddr[i] == '/') 
                {
			iPos = i;
			break;
		}
	}
	if(iPos == -1) 
        {
		strcpy(strParam, "");;
	} 
        else 
        {
		strcpy(strParam, strAddr+iPos+1);
	}
        
        std::string strTmpParam = strParam;
	return strTmpParam;
}


//�??HTTP请�?URL�???��??�???��??
int HttpRequest::GetPortFromUrl(const char* strUrl)
{
	int iPort = -1;
	std::string strHostAddr = GetHostAddrFromUrl(strUrl);	
	if(strHostAddr == "") 
        {
		return -1;
	}
	
	char strAddr[URLSIZE] = {0};
	strcpy(strAddr, strHostAddr.c_str());
	
	char* strPort = strchr(strAddr, ':');
	if(strPort == NULL) 
        {
		iPort = 80;
	} 
        else 
        {
		iPort = atoi(++strPort);
	}
	return iPort;
}


//�??HTTP请�?URL�???��??IP?��??
std::string HttpRequest::GetIPFromUrl(const char* strUrl)
{
	std::string strHostAddr = GetHostAddrFromUrl(strUrl);
	int iLen = strHostAddr.size();
        
        char strAddr[100] = {0};
        
	int iCount = 0;
	int iFlag = 0;
	for(int i=0; i<iLen; i++) 
        {
		if(strHostAddr[i] == ':') 
                {
			break;
		}
		
		strAddr[i] = strHostAddr[i];
		if(strHostAddr[i] == '.') 
                {
			iCount++;
			continue;
		}
		if(iFlag == 1) 
                {
			continue;
		}
		
		if((strHostAddr[i] >= '0') || (strHostAddr[i] <= '9')) 
                {
			iFlag = 0;
		} 
                else 
                {
			iFlag = 1;
		}
	}
	
	if(strlen(strAddr) <= 1) 
        {
		return "";
	}
	
	//?��??????为�?��????�????IP?��??�????????�???????��???��??IP?��??
	if((iCount == 3) && (iFlag == 0)) 
        {
                std::string strTmpAddr = strAddr;
		return strTmpAddr;
	} 
        else 
        {
		struct hostent *he = gethostbyname(strAddr);                
		if (he == NULL) 
                {
			return "";
		} 
                else 
                {
			struct in_addr** addr_list = (struct in_addr **)he->h_addr_list;			 
			for(int i = 0; addr_list[i] != NULL; i++) 
                        {
				return inet_ntoa(*addr_list[i]);
			}
			return "";
		}
	}
}


//�????SocketFd????为�????�????读�?��??
int HttpRequest::SocketFdCheck(const int iSockFd)
{
	struct timeval timeout ;
	fd_set rset,wset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(iSockFd, &rset);
    FD_SET(iSockFd, &wset);
    timeout.tv_sec = 3;
	timeout.tv_usec = 500;
	int iRet = select(iSockFd+1, &rset, &wset, NULL, &timeout);
	if(iRet > 0)
	{
		//?��??SocketFd????为�????�????读�?��??
		int iW = FD_ISSET(iSockFd,&wset);
		int iR = FD_ISSET(iSockFd,&rset);
		if(iW && !iR)
		{
			char error[4] = "";
			socklen_t len = sizeof(error);
			int ret = getsockopt(iSockFd,SOL_SOCKET,SO_ERROR,error,&len);
			if(ret == 0)
			{
				if(!strcmp(error, ""))
				{
					return iRet;//表示已�???�??好�????述�????
				}
				else 
				{
					syslog(LOG_DEBUG, "%s %s %d\tgetsockopt error code:%d,error message:%s", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
				}
			}
			else
			{
				syslog(LOG_DEBUG, "%s %s %d\tgetsockopt failed. error code:%d,error message:%s", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
			}
		}
		else
		{			
			syslog(LOG_DEBUG, "%s %s %d\tsockF%d%d\t\n", __FILE__, __FUNCTION__, __LINE__, iW, iR);
		}
	}
	else if(iRet == 0)
	{
		return 0;//表示�????
	}
	else
	{
		return -1;//select?��??�????????述�????�??0
	}
	return -2;//?��???�??
}


//???��???
void HttpRequest::DebugOut(const char *fmt, ...)
{
#ifdef __DEBUG__
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
#endif
}


int HttpRequest::SocketWrite(int nSocket,char * pBuffer,int nLen,int nTimeout)
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
		if(nWrite==-1)
		{
			if(errno!=EAGAIN)
			{
				return -1;
			}
		}

		if(nWrite<0)
		{
			return nWrite;
		}	
		nOffset+=nWrite;
		nLeft-=nWrite;
		nTotal+=nWrite;
		if(nLeft>0)
		{
			//	��ʱ100ms
			usleep(100000);
		}
		nLoop++;
	}
	return nTotal;
}
//int HttpRequest::m_iSocketFd = INVALID_SOCKET;

