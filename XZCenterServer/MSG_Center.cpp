// MSG_Center.cpp: implementation of the MSG_Task class.
//
//////////////////////////////////////////////////////////////////////

#include "ace/Select_Reactor.h"
#include "ace/SOCK_Stream.h"

#include "tinyXML/tinyxml.h"
#include "SimpleConfig.h"

#include "SysMessage.h"
#include "MSG_Center.h"
#include "Cmd_Acceptor.h"
#include "SysUtil.h"
#include "MySQLConnectionPool.h"
#include "ControlCmdHandler.h"

#include <boost/shared_array.hpp>

const char* DATABASENAME = "GatherGateSys";

ACE_Thread_Mutex CMSG_Center::msg_lock_;

std::vector<NET_PACKET_MSG*> CMSG_Center::msgVec_;
int CMSG_Center::m_nMsgBufferCount = 0;

ACE_Thread_Mutex CMSG_Center::channel_lock_;
std::map<string, T_ChannelStatus*> CMSG_Center::m_StaChannelStatusMap;

void RecvCtrlCmdCallback(char* szAreaID, char* szChannelNo, char* sIEType, char* szSequenceNo, char* szCtrlXML, int nLen)
{
    printf("recv sequence %s,ctrl cmd \n, %s \n", szSequenceNo, szCtrlXML);
    NET_PACKET_MSG* pMsg_ = NULL;
    CMSG_Center::get_message(pMsg_);
    if (!pMsg_)
    {
        return;
    }

    memset(pMsg_, 0, sizeof (NET_PACKET_MSG));

    pMsg_->msg_head.msg_type = SYS_MSG_SYSTEM_CTRL_CMD;

    T_CtrlCmd* pCtrlCmdData = (T_CtrlCmd*) pMsg_->msg_body;

    //拷贝初始报文
    strcpy(pCtrlCmdData->szAreaID, szAreaID);
    strcpy(pCtrlCmdData->szChannelNo, szChannelNo);
    strcpy(pCtrlCmdData->szIEType, sIEType);
    strcpy(pCtrlCmdData->szSequenceNo, szSequenceNo);
    pCtrlCmdData->nLen = nLen;
    memcpy(&pCtrlCmdData->szCtrlData, szCtrlXML, nLen);


    //记录连接句柄
    MSG_CENTER::instance()->put(pMsg_);

}

CMSG_Center::~CMSG_Center()
{
    ACE_Reactor::instance()->cancel_timer(this);
}

CMSG_Center::CMSG_Center()
{


}

static ACE_THR_FUNC_RETURN ThreadConnectToEventServerFunc(void *lParam)
{
    CMSG_Center* pTask = (CMSG_Center*) lParam;
    pTask->ConnectToEventServer();
    return 0;
}

static ACE_THR_FUNC_RETURN ThreadCheckUpdatedSeqFunc(void *lParam)
{
    CMSG_Center* pTask = (CMSG_Center*) lParam;
    pTask->CheckUpdatedSequence();
    return 0;
}

int CMSG_Center::open()
{

    event_handle = -1;
    //初始化信息数据缓冲区，此缓冲区用于CCtrlCmdReceiver_T向该类传输数据
    init_message_buffer();
    msg_semaphore_.acquire();


    m_bStartOk = false;

//    CMySQLConnectionPool::Init(CSimpleConfig::DATABASE_SERVER_IP, CSimpleConfig::DATABASE_USER, CSimpleConfig::DATABASE_PASSWORD
//            , DATABASENAME, CSimpleConfig::DATABASE_SERVER_PORT);


    {

        void* proxy_so = dlopen("/usr/lib/libXZCustomsProxy.so", RTLD_LAZY);
        if (!proxy_so)
        {

            CMyLog::m_pLog->_XGSysLog("********load library fail! dll name is  %s ,error %s %d \n", "libXZCustomsProxy.so", dlerror(), errno);
            return 1;
        } else
        {
            CMyLog::m_pLog->_XGSysLog("load library succ! dll name is  %s \n", "libXZCustomsProxy.so");
        }



        // reset errors
        dlerror();

        // load the symbols
        create_t* create_proxy = (create_t*) dlsym(proxy_so, "create");
        const char* dlsym_error = dlerror();
        if (dlsym_error)
        {
            CMyLog::m_pLog->_XGSysLog("Cannot load symbol create:  %s \n", dlsym_error);
            return 1;
        }

        if (create_proxy)
        {
            printf("get create function pointer %p succ !\n", create_proxy);
        }

        // create an instance of the class
        m_pPlatformProxy = create_proxy();

        if (m_pPlatformProxy)
        {
            m_pPlatformProxy->SetCtrlCmdCallback(RecvCtrlCmdCallback);
        }
    }

    ACE_Thread_Manager::instance()->spawn_n(MAX_HANDLE_THREADS, ThreadHandleMessage, this);
    //ACE_Thread_Manager::instance()->spawn(ThreadConnectToEventServerFunc, this);

    if (ACE_Reactor::instance()->schedule_timer(this, 0,
            ACE_Time_Value(2, 0),
            ACE_Time_Value(2, 0)) == -1)
    {
        return -1;
    }

    return 0;
}

ACE_THR_FUNC_RETURN CMSG_Center::ThreadHandleMessage(void *arg)
{
    CMSG_Center* pTask = (CMSG_Center*) arg;
    while (1)
    {
        pTask->msg_semaphore_.acquire();

        NET_PACKET_MSG* pMsg_ = NULL;
        {
            ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, pTask->msg_handle_lock_, 0);
            //printlog("Handle count : %d ****************************\n\n", CHA_Proactive_Acceptor::GetValidHandleCount());
            if (pTask->msgToHandleList_.size() == 0)
            {

                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000;
                select(0, 0, NULL, NULL, &tv);


                continue;
            }

            pMsg_ = pTask->msgToHandleList_.front();
            pTask->msgToHandleList_.pop_front();

        }

        if (!pMsg_)
        {
            continue;
        }

        //cast the pointer to Client_Handler*
        try
        {
            //printlog("+1\n");
            pTask->HandleNetMessage(pMsg_);
            //printlog("-1\n");
        } catch (...)
        {
            CMyLog::m_pLog->_XGSysLog("crash Message : %d ...\n", pMsg_->msg_head.msg_type);
            continue;
        }
    }
}

int CMSG_Center::ConnectToEventServer()
{
    CControlCmdHandler * handler = new CControlCmdHandler;

    while (1)
    {
        if (!CControlCmdHandler::m_bConnected)
        {
            if (handler->ConnectToAlarmServer(CSimpleConfig::EVENT_SERVER_PORT, CSimpleConfig::EVENT_SERVER_IP, this) == -1)
            {


            } else
            {
                CControlCmdHandler::m_bConnected = true;
            }
        }

        //select代替sleep,等待一段时间间隔5s
        SysUtil::SysSleep(2 * 1000);
    }

    delete handler;

}

int CMSG_Center::handle_timeout(const ACE_Time_Value &tv, const void *arg)
{
    ACE_UNUSED_ARG(tv);
    ACE_UNUSED_ARG(arg);

    NET_PACKET_MSG* pMsg = NULL;
    get_message(pMsg);
    if (!pMsg)
    {
        return -1;
    }


    //    put(pMsg);
    return 0;
}

int CMSG_Center::get_message(NET_PACKET_MSG *&pMsg)
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, msg_lock_, -1);

    pMsg = msgVec_[m_nMsgBufferCount];
    m_nMsgBufferCount++;
    if (m_nMsgBufferCount > MAX_BUFFERD_MESSAGE_NUMBER - 1)
    {
        m_nMsgBufferCount = 0;
    }

    return 0;
}

int CMSG_Center::put(NET_PACKET_MSG* pMsg)
{
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, msg_handle_lock_, -1);
    msgToHandleList_.push_back(pMsg);

    if (msgToHandleList_.size() > 10)
    {
        CMyLog::m_pLog->_XGSysLog("--------msg to handle %d! \n", msgToHandleList_.size());
    }

    msg_semaphore_.release();
    return 0;
}

int CMSG_Center::init_message_buffer()
{

    msgVec_.reserve(MAX_BUFFERD_MESSAGE_NUMBER);
    int nError = errno;
    for (int i = 0; i < MAX_BUFFERD_MESSAGE_NUMBER; i++)
    {

        NET_PACKET_MSG* pMsg = new NET_PACKET_MSG;
        msgVec_.push_back(pMsg);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100;
        select(0, 0, NULL, NULL, &tv);
    }

    return 0;
}

int CMSG_Center::HandleNetMessage(NET_PACKET_MSG *pPacket)
{
    if (!pPacket)
    {
        return -1;
    }

    switch (pPacket->msg_head.msg_type)
    {
        case SYS_MSG_SYSTEM_REGISTER_REQ:
            handleChannelRegister(pPacket);
            break;

        case SYS_MSG_SYSTEM_MSG_KEEPLIVE:
            handleChannelKeeplive(pPacket);
            break;


        case SYS_MSG_SYSTEM_MSG_UPLOADDATA_TO_CUSTOMS:
            handleUploadCustomsData(pPacket);
            break;

        case SYS_MSG_SYSTEM_MSG_UPLOADDATA_TO_CUSTOMS_ACK:
            handleCustomsDataAck(pPacket);
            break;

        case SYS_MSG_SYSTEM_CTRL_CMD:
            handleCtrlCmd(pPacket);
            break;

        case SYS_MSG_SEND_PICDATA:
            handleContaPicData(pPacket);
            break;


        case SYS_MSG_REGATHERDATA:
            handleRegatherData(pPacket);
            break;


        case SYS_MSG_DEVICE_CTRL:
            handleDeviceCtrl(pPacket);
            break;

		case CLIENT_EXCEPTIION_FREE:
			handleExceptionFree(pPacket);
			break;


    }

    return 0;
}

int CMSG_Center::handleChannelRegister(NET_PACKET_MSG* pPacket)
{
    if (!pPacket)
    {
        return -1;
    }

    int nProxyCount = pPacket->msg_head.proxy_count;
    if (nProxyCount < 1)
    {
        return -1;
    }

    int connect_handle = pPacket->msg_head.net_proxy[nProxyCount - 1];

    T_DeviceServerRegister_Req* pRegisterReq = (T_DeviceServerRegister_Req*) pPacket->msg_body;

    CMyLog::m_pLog->_XGSysLog("recv channel %s register...\n", pRegisterReq->device_id);
    
    
    {
        //2015-4-15
        //registerChannel(pRegisterReq->device_id);
        
        int nFlag = 0;
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, channel_lock_, -1);
        std::map<string, T_ChannelStatus*>::iterator iter;
        for (iter = m_StaChannelStatusMap.begin(); iter != m_StaChannelStatusMap.end(); ++iter)
        {
            T_ChannelStatus* pChannelStatus = iter->second;

            if (pChannelStatus && pChannelStatus->channel_id)
            {
                std::string strChannel1 = pChannelStatus->channel_id;
                std::string strChannel2 = pRegisterReq->device_id;
                if (strChannel1 == strChannel2)
                {
                    nFlag = 1;
                }
                    
            }
        }
        
        if (nFlag == 0)
        {
            registerChannel(pRegisterReq->device_id);
        }
        
    }
    


    {

        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, channel_lock_, -1);

        if (!m_StaChannelStatusMap[string(pRegisterReq->device_id)])
        {
            T_ChannelStatus* pChannelStatus = new T_ChannelStatus;
            pChannelStatus->connect_handle = connect_handle;
            pChannelStatus->is_active = 1;
            pChannelStatus->last_active = time(NULL);
            strcpy(pChannelStatus->channel_id, pRegisterReq->device_id);

            m_StaChannelStatusMap[string(pRegisterReq->device_id)] = pChannelStatus;

        } else
        {
            T_ChannelStatus* pChannelStatus = m_StaChannelStatusMap[string(pRegisterReq->device_id)];
            pChannelStatus->connect_handle = connect_handle;
            pChannelStatus->is_active = 1;
            pChannelStatus->last_active = time(NULL);
            strcpy(pChannelStatus->channel_id, pRegisterReq->device_id);

        }

    }

    //2015-4-15
    //registerChannel(pRegisterReq->device_id);




    //send ack
    ACE_SOCK_Stream server((ACE_HANDLE) connect_handle);
    char chMsgAck[1024] = {0};

    int sendLen = 0;
    memcpy(chMsgAck, SYS_NET_MSGHEAD, 8); //锟斤拷头锟斤拷锟?
    sendLen += 8;

    NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chMsgAck + sendLen);
    pHead->msg_type = SYS_MSG_SYSTEM_REGISTER_ACK;
    pHead->packet_len = sizeof (T_DeviceServerRegister_Ack);


    sendLen += sizeof (NET_PACKET_HEAD);

    T_DeviceServerRegister_Ack* pRegisterAck = (T_DeviceServerRegister_Ack*) (chMsgAck + sendLen);
    pRegisterAck->reg_result = 0;
    pRegisterAck->next_action = 1;

    sendLen += sizeof (T_DeviceServerRegister_Ack);

    pHead->packet_len = sizeof (T_DeviceServerRegister_Ack);
    memcpy(chMsgAck + sendLen, SYS_NET_MSGTAIL, 8);
    sendLen += 8;

    ACE_Time_Value expire_time = ACE_Time_Value(0, 100 * 1000);
    int nRet = server.send_n(chMsgAck, sendLen, &expire_time);


}

int CMSG_Center::publish_gather_event(T_Upload_Customs_Data* pUploadDataReq)
{
    ACE_SOCK_Stream server((ACE_HANDLE) event_handle);

    char chReq[1024 * 10] = {0};
    memset(chReq, 1024 * 10, 0);

    int sendLen = 0;
    memcpy(chReq, SYS_NET_MSGHEAD, 8); //包头数据
    sendLen += 8;

    NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chReq + sendLen);
    pHead->msg_type = SYS_MSG_SYSEVENT_PUBLISH;
    pHead->packet_len = sizeof (NET_EVENT_UPLOAD_CUSTOMSDATA) + pUploadDataReq->nCustomsDataLen;
    sendLen += sizeof (NET_PACKET_HEAD);

    NET_EVENT_UPLOAD_CUSTOMSDATA* pEventToSend = (NET_EVENT_UPLOAD_CUSTOMSDATA*) (chReq + sendLen);
    strcpy(pEventToSend->main_type, pUploadDataReq->szChannelNo);
    pEventToSend->sub_type = SYS_EVENT_TYPE_UPLOAD_CUSTOMS_DATA;

    memcpy(&pEventToSend->customs_data, pUploadDataReq, sizeof (T_Upload_Customs_Data) + pUploadDataReq->nCustomsDataLen);


    sendLen += sizeof (NET_EVENT_UPLOAD_CUSTOMSDATA) + pUploadDataReq->nCustomsDataLen;

    memcpy(chReq + sendLen, SYS_NET_MSGTAIL, 8); //包头数据
    sendLen += 8;


    ACE_Time_Value expire_time = ACE_Time_Value(1, 0);
    int nRet = server.send_n(chReq, sendLen, &expire_time);
    if (nRet == sendLen)
    {
        CMyLog::m_pLog->_XGSysLog("publish event succ...\n");
    } else
    {
        CMyLog::m_pLog->_XGSysLog("publish event fail...\n");
    }

    return 0;
}

int CMSG_Center::handleUploadCustomsData(NET_PACKET_MSG* pPacket)
{
    if (!pPacket)
    {
        return -1;
    }

    int nProxyCount = pPacket->msg_head.proxy_count;
    if (nProxyCount < 1)
    {
        return -1;
    }

    int connect_handle = pPacket->msg_head.net_proxy[nProxyCount - 1];

    T_Upload_Customs_Data* pUploadDataReq = (T_Upload_Customs_Data*) pPacket->msg_body;


    {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, seq_lock_, -1);
        T_SequenceStatus seq_status;
        strcpy(seq_status.channel_id, pUploadDataReq->szSequenceNo);
        seq_status.connect_handle = connect_handle;
        seq_status.last_active = time(NULL);

        m_StaChannelConnectMap[string(pUploadDataReq->szSequenceNo)] = seq_status;
    }


    CMyLog::m_pLog->_XGSysLog("recv channel %s seq %s to upload ...\n", pUploadDataReq->szChannelNo, pUploadDataReq->szSequenceNo);


    if (event_handle > 0)
    {
        publish_gather_event(pUploadDataReq);

    }



    //写入数据库
    //       int nRet = m_pDataAccessObj->RecordPassVehicleInfo(pDatabase, pUploadDataReq->szCustomsData + 38);
    //        CMyLog::m_pLog->_XGSysLog("record to database ret %d...\n", nRet);

    char szPacketData[1024 * 10] = {0};
    int nPacketLen = 0;

    //send to platform,发送到上级平台，可能有多个上级平台
    if (CSimpleConfig::m_StaPlatformMap.size() > 0)
    {
        std::map<string, T_PlatformInfo*>::iterator iter;
        for (iter = CSimpleConfig::m_StaPlatformMap.begin(); iter != CSimpleConfig::m_StaPlatformMap.end(); iter++)
        {
            T_PlatformInfo* pPlatformInfo = iter->second;
            char szPlatformName[32] = {0};
            strcpy(szPlatformName, iter->first.c_str());

            {
                CppMySQL3DB* pDatabase = NULL;
//                CMySQLConnectionPool::GetDataAccess(pDatabase);
//
//                if (!pDatabase)
//                {
//                    return -1;
//                }
//
//                CSQLAutoLock autolock(pDatabase);

                if (pUploadDataReq->is_data_full == 1)
                {
                    m_pPlatformProxy->PackUploadData(pDatabase, szPlatformName, pUploadDataReq->szCustomsData, pUploadDataReq->nCustomsDataLen, szPacketData, nPacketLen, 1);
                } 
                else
                {
                    m_pPlatformProxy->PackUploadData(pDatabase, szPlatformName, pUploadDataReq->szCustomsData, pUploadDataReq->nCustomsDataLen, szPacketData, nPacketLen, 0);
                }
            }


            //数据不完整，等候远程监控端补采数据，不发送到上级平台
            if (pUploadDataReq->is_data_full == 1)
            {
                CMyLog::m_pLog->_XGSysLog("upload data is needing regather %s...\n", pUploadDataReq->szSequenceNo);
                return -1;
            }

            //按照不同的平台，对数据重新打包
            int send_socket = SysUtil::CreateSocket();
            if (SysUtil::ConnectSocket(send_socket, pPlatformInfo->platform_ip, pPlatformInfo->platform_port) != 0)
            {
                SysUtil::CloseSocket(send_socket);
                CMyLog::m_pLog->_XGSysLog("********connect to platform %s:%d fail...\n", pPlatformInfo->platform_ip, pPlatformInfo->platform_port);
                return -1;
            }

            int nRet = SysUtil::SocketWrite(send_socket, szPacketData, nPacketLen, 1000);

            if (nRet == nPacketLen)
            {
                CMyLog::m_pLog->_XGSysLog("send data to platform %s:%d succ %d...\n", pPlatformInfo->platform_ip, pPlatformInfo->platform_port, nRet);
            }

            SysUtil::CloseSocket(send_socket);
        }
    }


}

int CMSG_Center::handleCustomsDataAck(NET_PACKET_MSG* pPacket)
{
    T_Upload_Customs_Data* pCustomsDataAck = (T_Upload_Customs_Data*) pPacket->msg_body;

    CppMySQL3DB* pDatabase = NULL;
//    CMySQLConnectionPool::GetDataAccess(pDatabase);
//
//    if (!pDatabase)
//    {
//        return -1;
//    }
//
//    CSQLAutoLock autolock(pDatabase);
//    pDatabase->SetCharsetName("GB2312");


    m_pPlatformProxy->UnpackCtrlData(pDatabase, pCustomsDataAck->szCustomsData, pCustomsDataAck->nCustomsDataLen);

    return 0;
}

int CMSG_Center::channel_exit(int handle)
{
    {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, channel_lock_, -1);
        std::map<string, T_ChannelStatus*>::iterator iter;
        for (iter = m_StaChannelStatusMap.begin(); iter != m_StaChannelStatusMap.end(); iter++)
        {
            T_ChannelStatus* pChannelStatus = iter->second;

            if (pChannelStatus && pChannelStatus->connect_handle == handle)
            {
                pChannelStatus->is_active = 0;

                CMyLog::m_pLog->_XGSysLog("********channel %s offline......\n", pChannelStatus->channel_id);
            }

        }
    }
}

int CMSG_Center::CheckUpdatedSequence()
{

    int nTimeNow = time(NULL);


    while (1)
    {
        {
            ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, seq_lock_, -1);
            std::map<string, T_SequenceStatus>::iterator iter;
            for (iter = m_StaChannelConnectMap.begin(); iter != m_StaChannelConnectMap.end(); iter++)
            {
                T_SequenceStatus status = iter->second;

                if (nTimeNow - status.last_active > 300)
                {
                    m_StaChannelConnectMap.erase(iter);
                    break;
                }

            }
        }

        SysUtil::SysSleep(5000);

    }

    return 0;

}

int CMSG_Center::handleChannelKeeplive(NET_PACKET_MSG* pPacket)
{
    int nProxyCount = pPacket->msg_head.proxy_count;
    if (nProxyCount < 1)
    {
        return -1;
    }

    int connect_handle = pPacket->msg_head.net_proxy[nProxyCount - 1];

    {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, channel_lock_, -1);
        std::map<string, T_ChannelStatus*>::iterator iter;
        for (iter = m_StaChannelStatusMap.begin(); iter != m_StaChannelStatusMap.end(); iter++)
        {
            T_ChannelStatus* pChannelStatus = iter->second;

            if (pChannelStatus && pChannelStatus->connect_handle == connect_handle)
            {
                pChannelStatus->is_active = 1;
                pChannelStatus->last_active = time(NULL);
            }

        }
    }


    ACE_SOCK_Stream server((ACE_HANDLE) connect_handle);
    char chMsgAck[1024] = {0};

    int sendLen = 0;
    memcpy(chMsgAck, SYS_NET_MSGHEAD, 8); //锟斤拷头锟斤拷锟?
    sendLen += 8;

    NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chMsgAck + sendLen);
    pHead->msg_type = SYS_MSG_SYSTEM_MSG_KEEPLIVE_ACK;
    pHead->packet_len = 0;


    sendLen += sizeof (NET_PACKET_HEAD);

    memcpy(chMsgAck + sendLen, SYS_NET_MSGTAIL, 8);
    sendLen += 8;

    ACE_Time_Value expire_time = ACE_Time_Value(0, 100 * 1000);
    int nRet = server.send_n(chMsgAck, sendLen, &expire_time);

    //    CMyLog::m_pLog->_XGSysLog("recv connect %d keeplive...\n", connect_handle);


}

int CMSG_Center::registerChannel(char* channel_id)
{
    if (!channel_id)
    {
        return -1;
    }

    if (event_handle > 0)
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
        int nRet = SysUtil::SocketWrite(event_handle, chReq, sendLen, 100);

        if (nRet == sendLen)
        {
            CMyLog::m_pLog->_XGSysLog("register channel %s succ...\n", channel_id);
        }

        return nRet;
    }

}

int CMSG_Center::handleCtrlCmd(NET_PACKET_MSG* pPacket)
{
    if (!pPacket)
    {
        return -1;
    }

    T_CtrlCmd* pCtrlCmdData = (T_CtrlCmd*) pPacket->msg_body;


    CMyLog::m_pLog->_XGSysLog("handle ctrl  %s.. \n", pCtrlCmdData->szSequenceNo);

    handleChannelPassAck(pCtrlCmdData);
    publish_pass_ack(pCtrlCmdData);

    return 0;

}

int CMSG_Center::publish_pass_ack(T_CtrlCmd* pCtrlCmd)
{

    ACE_SOCK_Stream server((ACE_HANDLE) event_handle);

    char chReq[1024 * 10] = {0};
    memset(chReq, 1024 * 10, 0);

    int sendLen = 0;
    memcpy(chReq, SYS_NET_MSGHEAD, 8); //包头数据
    sendLen += 8;

    NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chReq + sendLen);
    pHead->msg_type = SYS_MSG_SYSEVENT_PUBLISH;
    pHead->packet_len = sizeof (NET_EVENT_UPLOAD_CUSTOMSDATA) + pCtrlCmd->nLen;
    sendLen += sizeof (NET_PACKET_HEAD);

    NET_EVENT_UPLOAD_CUSTOMSDATA* pEventToSend = (NET_EVENT_UPLOAD_CUSTOMSDATA*) (chReq + sendLen);
    sprintf(pEventToSend->main_type, "%s%s%s", pCtrlCmd->szAreaID, pCtrlCmd->szChannelNo, pCtrlCmd->szIEType);
    pEventToSend->sub_type = SYS_EVENT_TYPE_CUSTOMS_DATA_ACK;


    strcpy(pEventToSend->customs_data.szChannelNo, pCtrlCmd->szChannelNo);
    strcpy(pEventToSend->customs_data.szSequenceNo, pCtrlCmd->szSequenceNo);
    pEventToSend->customs_data.nCustomsDataLen = pCtrlCmd->nLen;
    strcpy(pEventToSend->customs_data.szCustomsData, pCtrlCmd->szCtrlData);


    sendLen += sizeof (NET_EVENT_UPLOAD_CUSTOMSDATA) + pCtrlCmd->nLen;

    memcpy(chReq + sendLen, SYS_NET_MSGTAIL, 8); //包头数据
    sendLen += 8;


    ACE_Time_Value expire_time = ACE_Time_Value(1, 0);
    int nRet = server.send_n(chReq, sendLen, &expire_time);
    if (nRet == sendLen)
    {
        CMyLog::m_pLog->_XGSysLog("publish pass cmd event succ...\n");
    } else
    {
        CMyLog::m_pLog->_XGSysLog("publish pass cmd event fail...\n");
    }


    return 0;
}

int CMSG_Center::handleChannelPassAck(T_CtrlCmd* pCtrlCmd)
{
    int channel_connect_handle = -1;

    {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, seq_lock_, -1);
        if (m_StaChannelConnectMap.find(string(pCtrlCmd->szSequenceNo)) != m_StaChannelConnectMap.end())
        {
            channel_connect_handle = m_StaChannelConnectMap[string(pCtrlCmd->szSequenceNo)].connect_handle;

        }
    }

    if (channel_connect_handle == -1)
    {
        printf("channel connect handle is %d\n", channel_connect_handle);
        return -1;
    }



    ACE_SOCK_Stream server((ACE_HANDLE) channel_connect_handle);

    char chReq[1024 * 10] = {0};
    memset(chReq, 1024 * 10, 0);

    int sendLen = 0;
    memcpy(chReq, SYS_NET_MSGHEAD, 8); //包头数据
    sendLen += 8;

    NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chReq + sendLen);
    pHead->msg_type = SYS_MSG_SYSTEM_MSG_UPLOADDATA_TO_CUSTOMS_ACK;
    pHead->packet_len = sizeof (T_Upload_Customs_Data) + pCtrlCmd->nLen;
    sendLen += sizeof (NET_PACKET_HEAD);

    T_Upload_Customs_Data* pCustomsDataAck = (T_Upload_Customs_Data*) (chReq + sendLen);
    memcpy(pCustomsDataAck->szCustomsData, pCtrlCmd->szCtrlData, pCtrlCmd->nLen);
    strcpy(pCustomsDataAck->szChannelNo, "");
    strcpy(pCustomsDataAck->szSequenceNo, pCtrlCmd->szSequenceNo);



    sendLen += sizeof (T_Upload_Customs_Data) + pCtrlCmd->nLen;

    memcpy(chReq + sendLen, SYS_NET_MSGTAIL, 8); //包头数据
    sendLen += 8;


    ACE_Time_Value expire_time = ACE_Time_Value(1, 0);
    int nRet = server.send_n(chReq, sendLen, &expire_time);
    if (nRet == sendLen)
    {
        CMyLog::m_pLog->_XGSysLog("publish event succ...\n");
    } else
    {
        CMyLog::m_pLog->_XGSysLog("publish event fail...\n");
    }

    return 0;

}

int CMSG_Center::handleContaPicData(NET_PACKET_MSG* pPacket)
{
        FILE* fPath = NULL;
        BYTE chOrientation = ' ';
        int nPartLen       = 0;
        int nTotalPicLen   = 0;
        int nPerPicLen     = 0;
        const char packetHead[4] = {(char)0xE2, (char)0x5C, (char)0x4B, (char)0x89};
        const char packetEnd[2]  = {(char)0xFF, (char)0xFF};
        char cpYearMonDay[120] = {0};
        char szSavePath[120] = {0};
        char szCmd[120] = {0};
        char szTodayPath[120] = {0};
        char szHourPath[120] = {0};
        char szPicFileName[120] = {0};
        char szCommandInfo[200] = {0};
        char szDataType[2] = {0x26, 0x00};
        char szPlatformName[32] = {0};
        string strCommandInfoXml = "";
        int i = 0; 
        int nPicType = 0;
        FILE* pImageFile = NULL;
        int nXmlLen = 0;
        int PackDataLen = 0;
        BYTE *p = NULL;
        BYTE *pPic = NULL;
        boost::shared_array<BYTE> spPacketData;
         std::map<string, T_PlatformInfo*>::iterator iter;
         int send_socket = 0;
         int nRet = 0;
         T_PlatformInfo* pPlatformInfo = NULL;
         std::string strAreaNo      = "";
         std::string strChannelNo   = "";
        
    struct tm* tmNow;
    time_t tmTime = time(NULL);
             
    if (!pPacket)
    {
        return -1;
    }
    
    boost::shared_array<BYTE> spPicData;
    spPicData.reset(new BYTE[pPacket->msg_head.packet_len + 1]);
    bzero(spPicData.get(), pPacket->msg_head.packet_len + 1);
    pPic = spPicData.get();
    memcpy(pPic, pPacket->msg_body, pPacket->msg_head.packet_len);
    T_Upload_Pic_Data* pPicData = (T_Upload_Pic_Data*) pPic;
    
    tmNow = localtime(&tmTime);
    sprintf(cpYearMonDay, "%04d%02d%02d", tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);       
    

    //T_Upload_Pic_Data* pPicData = (T_Upload_Pic_Data*) pPacket->msg_body;
        sprintf(szCommandInfo, "<COMMAND_INFO AREA_ID=\"%s\" CHNL_NO=\"%s\" I_E_TYPE=\"%s\" SEQ_NO=\"%s\"></COMMAND_INFO>\r\n", 
            pPicData->szAreaNo, pPicData->szChannelNo, pPicData->szIEFlag, pPicData->szSequenceNo);
        strAreaNo       = pPicData->szAreaNo;
        strChannelNo    = pPicData->szChannelNo;
//    printf("COMMAND_INFO=%s\n", szCommandInfo);

//    {
//        std::string strFileNameNew = "/root/zhouhm/";
//        strFileNameNew += "ce_";
//        strFileNameNew += pPicData->szSequenceNo;
//        strFileNameNew += ".dat";
//        FILE* pImageFile = fopen(strFileNameNew.c_str(), "wb");
//        if (!pImageFile)
//        {
//            return -1;
//        }     
//        WriteAll(pImageFile, &(pPacket->msg_head), pPacket->msg_head.packet_len + sizeof(NET_PACKET_HEAD));
//        fclose(pImageFile);
//    }


    sprintf(szSavePath, "%s/%s", CSimpleConfig::PIC_SAVE_PATH, pPicData->szChannelNo);


    CMyLog::m_pLog->_XGSysLog("publish event succ...%s\n", szSavePath);

    fPath = fopen(szSavePath, "rb");
    if (!fPath)
    {
        sprintf(szCmd, "mkdir %s", szSavePath);
        system(szCmd);
    } else
    {
        fclose(fPath);
    }




    sprintf(szTodayPath, "%s/%s", szSavePath, cpYearMonDay);

    fPath = fopen(szTodayPath, "rb");
    if (!fPath)
    {
        sprintf(szCmd, "mkdir %s", szTodayPath);
        system(szCmd);
    } else
    {
        fclose(fPath);
    }


    sprintf(szHourPath, "%s/%02d", szTodayPath, tmNow->tm_hour);

//    printf("current hour path %s\n", szHourPath);

    fPath = fopen(szHourPath, "rb");
    if (!fPath)
    {
        sprintf(szCmd, "mkdir %s", szHourPath);
        system(szCmd);
    } else
    {
        fclose(fPath);
    }


    for (i = 0; i < pPicData->nPicNum; i++)
    {
        memset(szPicFileName, 0x00, sizeof(szPicFileName));
        nPicType = pPicData->picInfo[i].nPicType;

        if (nPicType == 0)
        {
            sprintf(szPicFileName, "%s/%s_%s_F.jpeg", szHourPath, pPicData->szChannelNo, pPicData->szSequenceNo);
        } else if (nPicType == 1)
        {
            sprintf(szPicFileName, "%s/%s_%s_LF.jpeg", szHourPath, pPicData->szChannelNo, pPicData->szSequenceNo);
        } else if (nPicType == 2)
        {
            sprintf(szPicFileName, "%s/%s_%s_RF.jpeg", szHourPath, pPicData->szChannelNo, pPicData->szSequenceNo);
        } else if (nPicType == 3)
        {
            sprintf(szPicFileName, "%s/%s_%s_B.jpeg", szHourPath, pPicData->szChannelNo, pPicData->szSequenceNo);
        } else if (nPicType == 4)
        {
            sprintf(szPicFileName, "%s/%s_%s_LB.jpeg", szHourPath, pPicData->szChannelNo, pPicData->szSequenceNo);
        } else if (nPicType == 5)
        {
            sprintf(szPicFileName, "%s/%s_%s_RB.jpeg", szHourPath, pPicData->szChannelNo, pPicData->szSequenceNo);
        }
        else if (nPicType == 6)
        {
            sprintf(szPicFileName, "%s/%s_%s_vehicle.jpeg", szHourPath, pPicData->szChannelNo, pPicData->szSequenceNo);
        }

        if (szPicFileName[0] == '\0')
            continue;

        pImageFile = fopen(szPicFileName, "wb");
        if (!pImageFile)
        {

            continue;
        }

        //fwrite(pPicData->szPicData + pPicData->picInfo[i].nOffSet, 1, pPicData->picInfo[i].nPicLen, pImageFile);
        // size_t CMSG_Center::WriteAll(FILE *fd, void *buff, size_t len)
//        printf("==========i:%d===========%d\n", i, pPicData->picInfo[i].nOffSet);
//        if (nPicType == 0)
//        {
//            WriteAll(pImageFile, pPic + sizeof(T_Upload_Pic_Data), pPicData->picInfo[i].nPicLen);
//        }
//        else
        {
            WriteAll(pImageFile, pPicData->szPicData + pPicData->picInfo[i].nOffSet, pPicData->picInfo[i].nPicLen);
        }
        fclose(pImageFile);

        if (nPicType != 6)
            RecordContaPicInfo(pPicData->szSequenceNo, nPicType, szPicFileName, strAreaNo, strChannelNo);
    }
    
    

    
    // send pic data
    //char szPacketData[1024 * 10] = {0};
    //int nPacketLen = 0;
    //=========================================================

        strCommandInfoXml = szCommandInfo;
        

        if (pPicData->nPicNum > 0)
        {
            for (i = 0; i < pPicData->nPicNum; i++)
            {
                nPerPicLen = pPicData->picInfo[i].nPicLen;  
                if (nPerPicLen <= 0)
                    break;
                
                nTotalPicLen += nPerPicLen;
                
                nPartLen += 5;
                nPartLen += nPerPicLen;
            }
        }                
        
        
        nXmlLen = strCommandInfoXml.size(); // strSendXml.size();
        if (pPicData->nPicNum > 0)
            PackDataLen = 34 + 4 + nXmlLen + 2 + 4 + nPartLen;
        else
            PackDataLen = 34 + 4 + nXmlLen + 2 + nPartLen;

        
        spPacketData.reset(new BYTE[PackDataLen + 1]);
        bzero(spPacketData.get(), PackDataLen + 1);

        p = spPacketData.get();
        memcpy(p, packetHead ,4);
        p += 4;

        memcpy(p, (BYTE*)&PackDataLen, sizeof(int));
        p += 4;

        
        memcpy(p, szDataType, 1);
        p += 1;	

        memcpy(p, pPicData->szAreaNo, 10);
        p += 10;

        memcpy(p, pPicData->szChannelNo, 10);
        p += 10;

        memcpy(p, pPicData->szIEFlag, 1);
        p += 1;							

        memset(p, 0xFF, 4);
        p += 4;	

        memcpy(p, &nXmlLen, 4);
        p+=4;

        memcpy(p, strCommandInfoXml.c_str(), nXmlLen);
        p += nXmlLen;
        
        // pic stream
        if (pPicData->nPicNum > 0)
        {     
            memcpy(p, &nTotalPicLen, 4);
            p += 4;
        }
        
        for (i = 0; i < pPicData->nPicNum; i++)
        {
            nPicType = pPicData->picInfo[i].nPicType;

            if (nPicType == 0)
            {
                chOrientation = 0x30;
            } 
            else if (nPicType == 1)
            {
                chOrientation = 0x31;
            } 
            else if (nPicType == 2)
            {
                chOrientation = 0x32;
            } 
            else if (nPicType == 3)
            {
                chOrientation = 0x33;
            } 
            else if (nPicType == 4)
            {
                chOrientation = 0x34;
            } 
            else if (nPicType == 5)
            {
                chOrientation = 0x35;
            }
            else if (nPicType == 6)
            {
                chOrientation = 0x36;
            }
            else
            {
                break;
            }

            memcpy(p, &chOrientation, 1);
            p += 1;
            
            nPerPicLen = pPicData->picInfo[i].nPicLen;            
            memcpy(p, &nPerPicLen, 4);
            p += 4;
            
            memcpy(p, pPicData->szPicData + pPicData->picInfo[i].nOffSet, nPerPicLen);
            p += nPerPicLen;        

            nTotalPicLen += nPerPicLen;
            // fwrite(pPicData->szPicData + pPicData->picInfo[i].nOffSet, 1, pPicData->picInfo[i].nPicLen);
        }
        
        memcpy(p, packetEnd ,2);
        // end
    
    
    
    

    //send to platform,发送到上级平台，可能有多个上级平台
    if (CSimpleConfig::m_StaPlatformMap.size() > 0)
    {
       
        for (iter = CSimpleConfig::m_StaPlatformMap.begin(); iter != CSimpleConfig::m_StaPlatformMap.end(); iter++)
        {
            pPlatformInfo = iter->second;
            
            strcpy(szPlatformName, iter->first.c_str());
            
            
            
//    {
//        std::string strFileNameNew = "/root/zhouhm/testpic/";
//        strFileNameNew += "ce_";
//        strFileNameNew += pPicData->szSequenceNo;
//        strFileNameNew += ".dat";
//        FILE* pImageFile = fopen(strFileNameNew.c_str(), "wb");
//        if (!pImageFile)
//        {
//            return -1;
//        }     
//        WriteAll(pImageFile, (char*)spPacketData.get(), PackDataLen);
//        fclose(pImageFile);
//    }
            
            

            //按照不同的平台，对数据重新打包
            send_socket = SysUtil::CreateSocket();
            if (SysUtil::ConnectSocket(send_socket, pPlatformInfo->platform_ip, pPlatformInfo->platform_port) != 0)
            {
                SysUtil::CloseSocket(send_socket);
                CMyLog::m_pLog->_XGSysLog("********connect to platform %s:%d fail...\n", pPlatformInfo->platform_ip, pPlatformInfo->platform_port);
                return -1;
            }

            nRet = SysUtil::SocketWrite(send_socket,  (char*)spPacketData.get(), PackDataLen, 1000);

            if (nRet == PackDataLen)
            {
                CMyLog::m_pLog->_XGSysLog("send pic data to platform %s:%d succ %d...\n", pPlatformInfo->platform_ip, pPlatformInfo->platform_port, nRet);
            }

            SysUtil::CloseSocket(send_socket);
        }
    }
    
}

int CMSG_Center::handleRegatherData(NET_PACKET_MSG* pMsg)
{


    CMyLog::m_pLog->_XGSysLog("recv regather req!\n");
    int nProxyCount = pMsg->msg_head.proxy_count;
    if (nProxyCount < 2)
    {
        return -1;
    }

    CCmdHandler_T* pPublishHandler = (CCmdHandler_T*) pMsg->msg_head.net_proxy[nProxyCount - 1];

    NET_REGATHERDATA_REQ* pReGatherReq = (NET_REGATHERDATA_REQ*) pMsg->msg_body;



    if (pReGatherReq->nRegatherType == 1) //手工方式
    {


        string strAreaNo;
        string strChannelNo;
        string strIEType;

        string strChannelInfo(pReGatherReq->channel_no);
        strAreaNo = strChannelInfo.substr(0, 10);
        strChannelNo = strChannelInfo.substr(10, 10);
        strIEType = strChannelInfo.substr(20, 1);


        NET_PACKET_MSG* pPacket = NULL;
        CMSG_Center::get_message(pPacket);
        if (!pPacket)
        {
            return -1;
        }

        memset(pPacket, 0, sizeof (NET_PACKET_MSG));

        T_Upload_Customs_Data* pUploadCustomsData = (T_Upload_Customs_Data*) pPacket->msg_body;
        pUploadCustomsData->is_data_full = 1;
        strcpy(pUploadCustomsData->szChannelNo, pReGatherReq->channel_no);

        PackBasicCustomsData((char*) strAreaNo.c_str(), (char*) strChannelNo.c_str(), (char*) strIEType.c_str(), pReGatherReq->customs_data.szCustomsData, strlen((char*) pReGatherReq->customs_data.szCustomsData), pUploadCustomsData->szCustomsData, pUploadCustomsData->nCustomsDataLen);


        char szPacketData[1024 * 10] = {0};
        int nPacketLen = 0;

        //send to platform,发送到上级平台，可能有多个上级平台
        if (CSimpleConfig::m_StaPlatformMap.size() > 0)
        {
            std::map<string, T_PlatformInfo*>::iterator iter;
            for (iter = CSimpleConfig::m_StaPlatformMap.begin(); iter != CSimpleConfig::m_StaPlatformMap.end(); iter++)
            {
                T_PlatformInfo* pPlatformInfo = iter->second;
                char szPlatformName[32] = {0};
                strcpy(szPlatformName, iter->first.c_str());

                {
                    CppMySQL3DB* pDatabase = NULL;
//                    CMySQLConnectionPool::GetDataAccess(pDatabase);
//
//                    if (!pDatabase)
//                    {
//                        return -1;
//                    }
//
//                    CSQLAutoLock autolock(pDatabase);

                    m_pPlatformProxy->PackUploadData(pDatabase, szPlatformName, pUploadCustomsData->szCustomsData, pUploadCustomsData->nCustomsDataLen, szPacketData, nPacketLen);
                }

                //按照不同的平台，对数据重新打包
                int send_socket = SysUtil::CreateSocket();
                if (SysUtil::ConnectSocket(send_socket, pPlatformInfo->platform_ip, pPlatformInfo->platform_port) != 0)
                {
                    SysUtil::CloseSocket(send_socket);
                    CMyLog::m_pLog->_XGSysLog("********connect to platform %s:%d fail...\n", pPlatformInfo->platform_ip, pPlatformInfo->platform_port);
                    return -1;
                }

                int nRet = SysUtil::SocketWrite(send_socket, szPacketData, nPacketLen, 1000);

                if (nRet == nPacketLen)
                {
                    CMyLog::m_pLog->_XGSysLog("send data to platform %s:%d succ %d...\n", pPlatformInfo->platform_ip, pPlatformInfo->platform_port, nRet);
                }

                SysUtil::CloseSocket(send_socket);
            }
        }
    } else //设备方式,把报文发送给通道控制器处理
    {
        int channel_connect_handle = -1;
        {

            ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, channel_lock_, -1);

            if (m_StaChannelStatusMap[string(pReGatherReq->channel_no)])
            {
                channel_connect_handle = m_StaChannelStatusMap[string(pReGatherReq->channel_no)]->connect_handle;
            }
        }

        if (channel_connect_handle == -1)
        {
            return -1;
        }


        ACE_SOCK_Stream server((ACE_HANDLE) channel_connect_handle);

        char chReq[1024 * 10] = {0};
        memset(chReq, 1024 * 10, 0);

        int sendLen = 0;
        memcpy(chReq, SYS_NET_MSGHEAD, 8); //包头数据
        sendLen += 8;

        NET_PACKET_HEAD* pHead = (NET_PACKET_HEAD*) (chReq + sendLen);
        pHead->msg_type = SYS_MSG_REGATHERDATA;
        pHead->packet_len = sizeof (NET_REGATHER);
        sendLen += sizeof (NET_PACKET_HEAD);

        NET_REGATHER* pRegahterData = (NET_REGATHER*) (chReq + sendLen);

        strcpy(pRegahterData->szChnnl_No, pReGatherReq->channel_no);
        strcpy(pRegahterData->szDeviceTag, pReGatherReq->device_tag);

        sendLen += sizeof (NET_REGATHER);

        memcpy(chReq + sendLen, SYS_NET_MSGTAIL, 8); //包头数据
        sendLen += 8;

        ACE_Time_Value expire_time = ACE_Time_Value(1, 0);
        int nRet = server.send_n(chReq, sendLen, &expire_time);
        if (nRet == sendLen)
        {
            CMyLog::m_pLog->_XGSysLog("send device regather cmd to channelcontrolserver succ...\n");
        } else
        {
            CMyLog::m_pLog->_XGSysLog("send device regather cmd to channelcontrolserver fail...\n");
        }
    }

    return 0;

}

int CMSG_Center::PackBasicCustomsData(char* szAraeID, char* szChannelNo, char* szIEType, char* szXML, int nXMLLen, char* szDestData, int& nPackedLen)
{
    char* szSendPacket = szDestData;

    szSendPacket[0] = 0XE2;
    szSendPacket[1] = 0X5C;
    szSendPacket[2] = 0X4B;
    szSendPacket[3] = 0X89;
    int nLen = 4;

    T_Len* pTotalLen = (T_Len*) (szSendPacket + nLen);
    nLen += sizeof (int);

    szSendPacket[nLen] = 0X21;
    nLen += sizeof (char);

    memcpy(szSendPacket + nLen, szAraeID, 10);
    nLen += 10;

    memcpy(szSendPacket + nLen, szChannelNo, 10);
    nLen += 10;

    if (strcmp(szIEType, "I") == 0)
    {
        szSendPacket[nLen] = 'I';
    } else
    {
        szSendPacket[nLen] = 'E';
    }
    nLen += 1;


    szSendPacket[nLen] = 0X00;
    nLen += 1;

    szSendPacket[nLen] = 0X00;
    nLen += 1;

    szSendPacket[nLen] = 0X00;
    nLen += 1;

    szSendPacket[nLen] = 0X00;
    nLen += 1;


    int nRealXMLLen = strlen(szXML) + 1;
    T_Len* pXMLLen = (T_Len*) (szSendPacket + nLen);
    pXMLLen->length = nRealXMLLen;
    pTotalLen->length = 40 + nRealXMLLen;

    nLen += 4;

    strcpy(szSendPacket + nLen, szXML);
    nLen += nRealXMLLen;

    szSendPacket[nLen] = 0XFF;
    nLen += 1;
    szSendPacket[nLen] = 0XFF;
    nLen += 1;

    nPackedLen = nLen;

    return 0;

}

int CMSG_Center::handleDeviceCtrl(NET_PACKET_MSG* pMsg)
{

    if (!pMsg)
    {
        return -1;
    }

    CLIENT_COMMAND_INFO* pCtrlCmdData = (CLIENT_COMMAND_INFO*) pMsg->msg_body;


    CMyLog::m_pLog->_XGSysLog("handle ctrl  %s.. \n", pCtrlCmdData->szChnnl_No);


    CppMySQL3DB* pDatabase = NULL;
//    CMySQLConnectionPool::GetDataAccess(pDatabase);
//
//    if (!pDatabase)
//    {
//        return -1;
//    }
//
//    CSQLAutoLock autolock(pDatabase);
//    pDatabase->SetCharsetName("GB2312");

    m_pPlatformProxy->BuildCtrlData(pDatabase, pCtrlCmdData->szChnnl_No, pCtrlCmdData->szSequence_No, pCtrlCmdData->szCustomsData, pCtrlCmdData->szMEM);

    return 0;
}

int CMSG_Center::handleExceptionFree(NET_PACKET_MSG* pMsg)
{

	if (!pMsg)
	{
		return -1;
	}

	CLIENT_EXCEPTION_FREE* pCtrlCmdData = (CLIENT_EXCEPTION_FREE*) pMsg->msg_body;


	CMyLog::m_pLog->_XGSysLog("handle exception free  %s.. \n", pCtrlCmdData->szChnnl_No);


	CppMySQL3DB* pDatabase = NULL;
//	CMySQLConnectionPool::GetDataAccess(pDatabase);
//
//	if (!pDatabase)
//	{
//		return -1;
//	}
//
//	CSQLAutoLock autolock(pDatabase);
//	pDatabase->SetCharsetName("GB2312");

	m_pPlatformProxy->BuildExceptionFreeData(pDatabase, pCtrlCmdData->szChnnl_No, pCtrlCmdData->szSequence_No, pCtrlCmdData->szExceptionFreeData,pCtrlCmdData->nExceptionFreeDataLen, pCtrlCmdData->szMEM);

	return 0;
}

int CMSG_Center::RecordContaPicInfo(char* szSeqNo, int nPicType, char* szFileName, const std::string &strAreaNo, const std::string &strChannelNo)
{


    CppMySQL3DB* pDatabase = NULL;
//    CMySQLConnectionPool::GetDataAccess(pDatabase);
//
//    if (!pDatabase)
//    {
//        return -1;
//    }
//
//    CSQLAutoLock autolock(pDatabase);
//    pDatabase->SetCharsetName("GB2312");
//
//
//
//    char cpYearMonDay[256] = {0};
//    struct tm* tmNow;
//    time_t tmTime = time(NULL);
//    tmNow = localtime(&tmTime);
//    sprintf(cpYearMonDay, "%04d-%02d-%02d %02d:%02d:%02d", tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec);
//
//
//    char chQuerySQL[1024] = {0};
//    sprintf(chQuerySQL, "INSERT INTO T_GATHERINFO_CONTAPIC VALUES('%s','%s','%s','%s',%d,'%s')"
//            , szSeqNo
//            , strAreaNo.c_str()
//            , strChannelNo.c_str()
//            , cpYearMonDay
//            , nPicType
//            , szFileName);
//
//
//
//    int nRet = pDatabase->execSQL(chQuerySQL);


    return 0;
}

size_t CMSG_Center::WriteAll(FILE *fd, void *buff, size_t len)
{
	size_t n	= 0;
	size_t sum	= 0;
	do 
	{
		n = fwrite((char*)buff + sum, 1, len - sum, fd);
		sum += n;
	} while (sum < len && n != 0);
	if (n == 0 && ferror(fd))
		return 0;
	if (n == 0)
		return 1;
	return 1;
}
