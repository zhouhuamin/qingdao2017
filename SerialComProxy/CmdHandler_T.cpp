// CmdHandler_T.cpp: implementation of the CCmdHandler_T class.
//
//////////////////////////////////////////////////////////////////////
#include "CmdHandler_T.h"
#include "Cmd_Acceptor.h"

#include "SysMessage.h"
#include "MSG_Center.h"
#include "SysUtil.h"
#include <syslog.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CCmdHandler_T::CCmdHandler_T()
{
    m_nObjID = 0;
    dwRecvBuffLen = 0;

    chRecvBuffer = new char[MAX_MSG_BODYLEN];
    chDest = new char[MAX_MSG_BODYLEN];


}

CCmdHandler_T::~CCmdHandler_T()
{
    delete [] chRecvBuffer;
    delete [] chDest;

}

void CCmdHandler_T::destroy()
{
   
    MSG_CENTER::instance()->channel_exit(this->peer().get_handle());
    
    this->reactor()->remove_handler(this,
            ACE_Event_Handler::READ_MASK
            | ACE_Event_Handler::DONT_CALL);

    this->peer().close();
    CCmd_Acceptor::free_handler(m_nObjID);
    //	inherited::destroy();
}

int CCmdHandler_T::open(void *void_acceptor)
{
    
    
     int rcvbuf = 2 * 1024 * 1024;
    int rcvbufsize = sizeof (int);
    
 
    setsockopt(this->peer().get_handle(), SOL_SOCKET, SO_RCVBUF, (char*) &rcvbuf,rcvbufsize);
    
    
    dwRecvBuffLen = 0;
    //	this->activate (THR_DETACHED);
    if (this->reactor()->register_handler(this,
            ACE_Event_Handler::READ_MASK) == -1)
    {
//        ACE_ERROR_RETURN((LM_ERROR,
//                "(%P|%t) can't register with reactor\n"),
//                -1);
        
        syslog(LOG_DEBUG, "can't register with reactor\n");
    }

    syslog(LOG_DEBUG, "recv socket is %d\n", (int) this->peer().get_handle());

    return 0;
}

int CCmdHandler_T::close(u_long flags)
{
    ACE_UNUSED_ARG(flags);
    this->destroy();
    return 0;
}

int CCmdHandler_T::handle_input(ACE_HANDLE handle)
{
    ACE_UNUSED_ARG(handle);

    return this->process();
}

int CCmdHandler_T::handle_close(ACE_HANDLE handle,
        ACE_Reactor_Mask mask)
{
    ACE_UNUSED_ARG(handle);
    ACE_UNUSED_ARG(mask);

    this->destroy();
    return 0;
}

int CCmdHandler_T::svc(void)
{
    while (!bStopFlag_)
    {
        if (this->process() == -1)
            return -1;
    }

    bStopFlag_ = true;
    return 0;
}

int CCmdHandler_T::stop(void)
{
    bStopFlag_ = true;
    return 0;
}

int CCmdHandler_T::process()
{

    ssize_t bytes_read;
    switch ((bytes_read = this->peer().recv(chRecvBuffer + dwRecvBuffLen, MAX_MSG_BODYLEN - dwRecvBuffLen)))
    {
        case -1:
            return -1;
        case 0:
            if (dwRecvBuffLen > 0)
            {
                int nOffset = 0;
                int nLen = 0;


                syslog(LOG_DEBUG, "yyyyyyrecv data len %d\n",dwRecvBuffLen);
                if(dwRecvBuffLen==936474)
                {
                    int nnnn=0;
                }

                while (VerifyRecvPacket(chRecvBuffer, chDest, dwRecvBuffLen, nOffset, nLen) == 0)
                {
                    NET_PACKET_HEAD* pPacketHead = (NET_PACKET_HEAD*) (chDest + nOffset);
                    ;

                    if (nLen == pPacketHead->packet_len + sizeof (NET_PACKET_HEAD))
                    {
                        NET_PACKET_MSG* pMsg_ = NULL;
                        CMSG_Center::get_message(pMsg_);
                        if (!pMsg_)
                        {
                            return -1;
                        }

                        memset(pMsg_, 0, sizeof (NET_PACKET_MSG));

                        //拷贝初始报文
                        syslog(LOG_DEBUG, "yyyyyrecv packet type %d\n",pPacketHead->msg_type);

                        memcpy(pMsg_, chDest + nOffset, pPacketHead->packet_len + sizeof (NET_PACKET_HEAD));

                        //T_Upload_Pic_Data* pPicData = (T_Upload_Pic_Data*) pMsg_->msg_body;

                        //记录连接句柄
                        pMsg_->msg_head.net_proxy[pPacketHead->proxy_count] = this->peer().get_handle();
                        pMsg_->msg_head.proxy_count++;


                        MSG_CENTER::instance()->put(pMsg_);
                    }
                }                
            }
            return -1;
        default:
            dwRecvBuffLen += bytes_read;

            int nOffset = 0;
            int nLen = 0;


            syslog(LOG_DEBUG, "xxxxxxrecv data len %d\n",dwRecvBuffLen);
            if(dwRecvBuffLen==936474)
            {
                int nnnn=0;
            }
            
            while (VerifyRecvPacket(chRecvBuffer, chDest, dwRecvBuffLen, nOffset, nLen) == 0)
            {
                NET_PACKET_HEAD* pPacketHead = (NET_PACKET_HEAD*) (chDest + nOffset);
                ;

                if (nLen == pPacketHead->packet_len + sizeof (NET_PACKET_HEAD))
                {
                    NET_PACKET_MSG* pMsg_ = NULL;
                    CMSG_Center::get_message(pMsg_);
                    if (!pMsg_)
                    {
                        return -1;
                    }

                    memset(pMsg_, 0, sizeof (NET_PACKET_MSG));

                    //拷贝初始报文
                    syslog(LOG_DEBUG, "xxxxxrecv packet type %d\n",pPacketHead->msg_type);
                    
                    memcpy(pMsg_, chDest + nOffset, pPacketHead->packet_len + sizeof (NET_PACKET_HEAD));

                    //T_Upload_Pic_Data* pPicData = (T_Upload_Pic_Data*) pMsg_->msg_body;
                    
                    //记录连接句柄
                    pMsg_->msg_head.net_proxy[pPacketHead->proxy_count] = this->peer().get_handle();
                    pMsg_->msg_head.proxy_count++;


                    MSG_CENTER::instance()->put(pMsg_);
                }
            }
    }

    return 0;
}

int CCmdHandler_T::VerifyRecvPacket(char *chRecvBuffer, char* chDest, int &nRecLen, int &nOffset, int &nLen)
{

    /*缓冲区长度小于最小帧长度*/
    if (nRecLen < sizeof (NET_PACKET_HEAD))
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

    memcpy(chDest, chRecvBuffer + nHeadPos, (nTailPos - nHeadPos) + 8);

    if (nRecLen - nTailPos - 8 > 0)
    {
        //接收的数据可能是连续的包
        //memcpy(chRecvBuffer, chRecvBuffer + nTailPos + 8, nRecLen - nTailPos - 8);
        memmove(chRecvBuffer, chRecvBuffer + nTailPos + 8, nRecLen - nTailPos - 8);
    }

    nOffset = nHeadPos = 8;
    nLen = nTailPos - nHeadPos;
    nRecLen = nRecLen - nTailPos - 8;

    return 0;
}

int CCmdHandler_T::SearchHeadPos(char *chBuffer, int nDataLen)
{
    int end, i, j;
    end = nDataLen - 8; /* 计算结束位置*/

    if (end > 0)
    {
        for (i = 0; i <= end; i++)
        {
            //循环比较
            for (j = i; chBuffer[j] == SYS_NET_MSGHEAD[j - i]; j++)
            {
                if (SYS_NET_MSGHEAD[j - i + 1] == '\0')
                {
                    return i; /* 找到了子字符串   */
                }
            }
        }
    }

    return -1;
}

int CCmdHandler_T::SearchTailPos(char *chBuffer, int nDataLen)
{
    int end, i, j;
    end = nDataLen - 8; /* 计算结束位置*/

    if (end > 0)
    {
        for (i = 0; i <= end; i++)
        {
            //循环比较
            for (j = i; chBuffer[j] == SYS_NET_MSGTAIL[j - i]; j++)
            {
                if (SYS_NET_MSGTAIL[j - i + 1] == '\0')
                {
                    return i; /* 找到了子字符串   */
                }
            }
        }
    }

    return -1;
}

