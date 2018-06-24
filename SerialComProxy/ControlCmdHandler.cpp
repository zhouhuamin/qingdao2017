// ControlCmdHandler.cpp: implementation of the CControlCmdHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <netinet/tcp.h>

#include "ControlCmdHandler.h"
#include "SysUtil.h"
#include "SysMessage.h"
#include "MSG_Center.h"
#include "SimpleConfig.h"
#include <syslog.h>

#include <map>
#include <string>
#include <bits/stl_map.h>
using namespace std;

bool CControlCmdHandler::m_bConnected = false;

#ifdef WIN32

void HandlerProc(void * pParam)
{
    if (!pParam)
    {
        return;
    }
    CControlCmdHandler* pHandler = (CControlCmdHandler*) pParam;
    if (!pHandler)
    {
        return;
    }

    pHandler->svc();

    SysUtil::SysSleep(500);

    return;
}
#else

static ACE_THR_FUNC_RETURN HandlerProc(void *pParam)
{
    if (!pParam)
    {
        return 0;
    }
    CControlCmdHandler* pHandler = (CControlCmdHandler*) pParam;
    if (!pHandler)
    {
        return 0;
    }

    pHandler->svc();

    SysUtil::SysSleep(500);

    return 0;
}

#endif

CControlCmdHandler::CControlCmdHandler(void)
{
    dwRecvBuffLen = 0;
    bGoWork_ = true;
    socket_ = -1;
    bThread_flag_ = false;
    chRecvBuffer=new char[MAX_MSG_BODYLEN];
    chDest=new char[MAX_MSG_BODYLEN];
    
}

CControlCmdHandler::~CControlCmdHandler(void)
{
    if (bGoWork_)
    {
        bGoWork_ = false;
        SysUtil::SysSleep(1000);
    }

    m_bConnected = false;
    if (socket_ > 0)
    {
        SysUtil::CloseSocket(socket_);
    }
    
    delete [] chRecvBuffer;
    
}

int CControlCmdHandler::open(void*)
{
    
    
//     int rcvbuf = 2 * 1024 * 1024;
//    int rcvbufsize = sizeof (int);
//    setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, (char*) &rcvbuf,rcvbufsize);
    
    
    MSG_CENTER::instance()->setEventhandle(socket_);
    
    if (!bThread_flag_)
    {
        bThread_flag_=true;

        ACE_Thread_Manager::instance()->spawn(HandlerProc, this);

    }
    
    


    return 0;
}

int CControlCmdHandler::svc()
{
    unsigned long dwRecvBuffLen = 0;
  
    int nRecvLen = 0;
    const char packetHead[4]={(char)0xE2, (char)0x5C, (char)0x4B, (char)0x89};
    const char packetEnd[2]={(char)0xFF, (char)0xFF};
    
 

    while (bGoWork_)
    {
        if (socket_ > 0)
        {
            /*阻塞接收信令*/
            if (SysUtil::ReceiveTimer(socket_) > 0)
            {
                int nRet = SysUtil::SocketRead(socket_, chRecvBuffer + nRecvLen, MAX_MSG_BODYLEN - nRecvLen);
                nRecvLen += nRet;

                if (nRet > 0)
                {

                    int nOffset = 0;
                    int nLen = 0;
                    while (VerifyRecvPacket(chRecvBuffer, chDest, nRecvLen, nOffset, nLen) == 0)
                    {
                        //NET_PACKET_HEAD* pMsgHead = (NET_PACKET_HEAD*) (chDest + nOffset);
                        //int nPacketLen = pMsgHead->packet_len;

                        //if (nLen == nPacketLen + sizeof (NET_PACKET_HEAD))
                        {
                            NET_PACKET_MSG* pMsg_ = NULL;
                            CMSG_Center::get_message(pMsg_);
                            if (!pMsg_)
                            {
                                return -1;
                            }

                            memset(pMsg_, 0, sizeof (NET_PACKET_MSG));
                            
                            
                            pMsg_->msg_head.msg_type = SYS_MSG_SYSTEM_MSG_UPLOADDATA_TO_CUSTOMS_ACK;

                            T_Upload_Customs_Data* pCustomsData = (T_Upload_Customs_Data*) pMsg_->msg_body;

                            //拷贝初始报文
                            memcpy(&pCustomsData->szCustomsData, chRecvBuffer, nLen);
                            pCustomsData->nCustomsDataLen = nLen;

                            pMsg_->msg_head.packet_len = nLen + sizeof (T_Upload_Customs_Data);                            


                            //拷贝初始报文

                            MSG_CENTER::instance()->put(pMsg_);
                            syslog(LOG_DEBUG, "recv pass message from serial-server len=%d\n", nLen);

                            // send feedback
                            struct timeval timeo = {5, 0};
                            socklen_t len = sizeof(timeo);
                            timeo.tv_sec = 5;
                            setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);                            
                            
                            int nFeedbackLen           = 100;
                            char szCode[4]             = {0};
                            char szBody[86]            = {0};
                            char szSendBuffer[100 + 1] = {0};
                            structFeedback_Info *pFeedback = (structFeedback_Info *)szSendBuffer;
                            memcpy(pFeedback->szHead,  packetHead, 4);
                            pFeedback->nAllLen = nFeedbackLen;
                            memcpy(pFeedback->szCode,  szCode, 4);
                            memcpy(pFeedback->szBody,  szBody, 86);
                            memcpy(pFeedback->szEnd,   packetEnd, 2);
                            int nSendLen = SysUtil::SocketWrite(socket_, szSendBuffer, 100, 3);
                            syslog(LOG_DEBUG, "send feedback message to serial-server len=%d\n", nFeedbackLen);
                            // send end
                        }
                    }
                }
                else
                {
                    SysUtil::CloseSocket(socket_);
                    socket_ = -1;
                    m_bConnected = false;

                    nRecvLen = 0;

                    SysUtil::SysSleep(50);
                }
            }
        }
        else
        {
            SysUtil::SysSleep(500);
        }
    }

    SysUtil::SysSleep(50);
    return 0;
}

int CControlCmdHandler::ConnectToAlarmServer(short port, const char* chIP, CMSG_Center *pCenter)
{
    if (socket_ > 0)
    {
        SysUtil::CloseSocket(socket_);
        socket_ = -1;
    }

    socket_ = SysUtil::CreateSocket();
    if (SysUtil::ConnectSocket(socket_, chIP, port) == 0)
    {
        
//        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, CMSG_Center::channel_lock_, -1);
//        std::map<string, T_ChannelStatus*>::iterator iter;
//        for (iter = CMSG_Center::m_StaChannelStatusMap.begin(); iter != CMSG_Center::m_StaChannelStatusMap.end(); ++iter)
//        {
//            T_ChannelStatus* pChannelStatus = iter->second;
//
//            if (pChannelStatus && pChannelStatus->is_active == 1)
//            {
//                registerChannel(pChannelStatus->channel_id, socket_);
//            }
//        }
        

        
        set_keep_live(3, 5);

        m_bConnected = true;
        open(NULL);

        syslog(LOG_DEBUG, "connect to %s:%d succ......\n",chIP,port);
        return 0;
    }
    else
    {
        SysUtil::CloseSocket(socket_);
        socket_ = -1;

        syslog(LOG_DEBUG, "connect to %s:%d fail......\n",chIP,port);
        return -1;
    }
}

int CControlCmdHandler::ReceiveTimer(int fd)
{
    int iret = 0;
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;

    iret = select(fd + 1, &rset, NULL, NULL, &tv);

    return iret;
}

int CControlCmdHandler::SendHeartbeatMsg()
{
    char chReq[1024] = {0};
    memset(chReq, 1024, 0);

    int sendLen = 0;
    memcpy(chReq, SYS_NET_MSGHEAD, 8); //包头数据
    sendLen += 8;

    NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chReq + sendLen);
    pHead->msg_type = SYS_MSG_SYSTEM_MSG_KEEPLIVE;
    pHead->packet_len = 0;

    sendLen += sizeof (NET_PACKET_HEAD);

    memcpy(chReq + sendLen, SYS_NET_MSGTAIL, 8); //包头数据
    sendLen += 8;
    int nRet = SysUtil::SocketWrite(socket_, chReq, sendLen, 100);

    return 0;
}

int CControlCmdHandler::set_keep_live(int keep_alive_times, int keep_alive_interval)
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


    if (WSAIoctl((unsigned int) socket_, SIO_KEEPALIVE_VALS,
        (LPVOID) & inKeepAlive, ulInLen,
        (LPVOID) & outKeepAlive, ulOutLen,
        &ulBytesReturn, NULL, NULL) == SOCKET_ERROR)
    {
        ACE_DEBUG((LM_INFO,
                ACE_TEXT("(%P|%t) WSAIoctl failed. error code(%d)!\n"), WSAGetLastError()));
    }

#else                                        //linux下
    int keepAlive = 1; //设定KeepAlive
    int keepIdle = keep_alive_interval; //开始首次KeepAlive探测前的TCP空闭时间
    int keepInterval = keep_alive_interval; //两次KeepAlive探测间的时间间隔
    int keepCount = keep_alive_times; //判定断开前的KeepAlive探测次数
    if (setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, (const char*) & keepAlive, sizeof (keepAlive)) == -1)
    {
//        ACE_DEBUG((LM_INFO,
//                ACE_TEXT("(%P|%t) setsockopt SO_KEEPALIVE error!\n")));
        
        syslog(LOG_DEBUG, "setsockopt SO_KEEPALIVE error!\n");
    }
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPIDLE, (const char *) & keepIdle, sizeof (keepIdle)) == -1)
    {
//        ACE_DEBUG((LM_INFO,
//                ACE_TEXT("(%P|%t) setsockopt TCP_KEEPIDLE error!\n")));
        
        syslog(LOG_DEBUG, "setsockopt TCP_KEEPIDLE error!\n");
    }
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPINTVL, (const char *) & keepInterval, sizeof (keepInterval)) == -1)
    {
//        ACE_DEBUG((LM_INFO,
//                ACE_TEXT("(%P|%t) setsockopt TCP_KEEPINTVL error!\n")));
        
        syslog(LOG_DEBUG, "setsockopt TCP_KEEPINTVL error!\n");
    }
    if (setsockopt(socket_, SOL_TCP, TCP_KEEPCNT, (const char *) & keepCount, sizeof (keepCount)) == -1)
    {
//        ACE_DEBUG((LM_INFO,
//                ACE_TEXT("(%P|%t)setsockopt TCP_KEEPCNT error!\n")));
        syslog(LOG_DEBUG, "setsockopt TCP_KEEPCNT error!\n");
    }

#endif

    return 0;

}

int CControlCmdHandler::VerifyRecvPacket(char *chRecvBuffer, char* chDest, int &nRecLen, int &nOffset, int &nLen)
{
    /*缓冲区长度小于最小帧长度*/
    if (nRecLen < 8)
    {
        return -1;
    }

    int nHeadPos = SysUtil::SearchHeadPos(chRecvBuffer, nRecLen);
    if (nHeadPos < 0)
    {
        return -1;
    }

    int nTailPos = SysUtil::SearchTailPos(chRecvBuffer, nRecLen);
    if (nTailPos < 0)
    {
        return -1;
    }

    memcpy(chDest, chRecvBuffer + nHeadPos, (nTailPos - nHeadPos));

    if (nRecLen - nTailPos > 0)
    {
        //接收的数据可能是连续的包
        memmove(chRecvBuffer, chRecvBuffer + nTailPos, nRecLen - nTailPos);
    }

    nOffset = nHeadPos = 0;
    nLen = nTailPos - nHeadPos;
    nRecLen = nRecLen - nTailPos;

    return 0;
}

int CControlCmdHandler::registerChannel(char* channel_id, int socket)
{
    if (!channel_id)
    {
        return -1;
    }

    if (socket > 0)
    {
        char chReq[1024] = {0};
        memset(chReq, 1024, 0);

        int sendLen = 0;
        memcpy(chReq, SYS_NET_MSGHEAD, 8); //包头数据
        sendLen += 8;

        NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chReq + sendLen);
        pHead->msg_type = SYS_MSG_EVENT_SUBSCRIBE;
        pHead->packet_len = sizeof (NET_EVENT_SUBSCRIBE);

        sendLen += sizeof (NET_PACKET_HEAD);


        NET_EVENT_SUBSCRIBE* pSubscribeReq = (NET_EVENT_SUBSCRIBE*) (chReq + sendLen);
        pSubscribeReq->client_type = EVENT_SERVICE_TYPE_CONTROLER;
        strcpy(pSubscribeReq->user_name, "");
        strcpy(pSubscribeReq->pass_word, "");
        strcpy(pSubscribeReq->event_serial, channel_id);


        sendLen += sizeof (NET_EVENT_SUBSCRIBE);


        memcpy(chReq + sendLen, SYS_NET_MSGTAIL, 8); //包头数据
        sendLen += 8;
        int nRet = SysUtil::SocketWrite(socket, chReq, sendLen, 100);

        if (nRet == sendLen)
        {
            syslog(LOG_DEBUG, "register channel %s succ...\n", channel_id);
        }

        return nRet;
    }

}

