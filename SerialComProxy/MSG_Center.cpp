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
#include "ControlCmdHandler.h"
#include "Encrypter.h"
#include "UtilTools.h"
#include <syslog.h>

#include <boost/shared_array.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/std/utility.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace boost;

ACE_Thread_Mutex CMSG_Center::msg_lock_;

std::vector<NET_PACKET_MSG*> CMSG_Center::msgVec_;
int CMSG_Center::m_nMsgBufferCount = 0;

ACE_Thread_Mutex CMSG_Center::channel_lock_;
std::map<string, T_ChannelStatus*> CMSG_Center::m_StaChannelStatusMap;

void RecvCtrlCmdCallback(char* szAreaID, char* szChannelNo, char* sIEType, char* szSequenceNo, char* szCtrlXML, int nLen)
{
    syslog(LOG_DEBUG, "recv sequence %s,ctrl cmd \n, %s \n", szSequenceNo, szCtrlXML);
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
    
    int nDATABASE_SERVER_PORT = atoi(CSimpleConfig::DATABASE_SERVER_PORT.c_str());

//    CMySQLConnectionPool::Init(CSimpleConfig::DATABASE_SERVER_IP.c_str(), CSimpleConfig::DATABASE_USER.c_str(), CSimpleConfig::DATABASE_PASSWORD.c_str()
//            , DATABASENAME, nDATABASE_SERVER_PORT);
    
    {
        void* proxy_so = dlopen("/usr/lib/libSerialComProxyDll.so", RTLD_LAZY);
        if (!proxy_so)
        {
            syslog(LOG_DEBUG, "********load library fail! dll name is  %s ,error %s %d \n", "libSerialComProxyDll.so", dlerror(), errno);
            return 1;
        } 
        else
        {
            syslog(LOG_DEBUG, "load library succ! dll name is  %s \n", "libSerialComProxyDll.so");
        }



        // reset errors
        dlerror();

        // load the symbols
        create_t* create_proxy = (create_t*) dlsym(proxy_so, "create");
        const char* dlsym_error = dlerror();
        if (dlsym_error)
        {
            syslog(LOG_DEBUG, "Cannot load symbol create:  %s \n", dlsym_error);
            return 1;
        }

        if (create_proxy)
        {
            syslog(LOG_DEBUG, "get create function pointer %p succ !\n", create_proxy);
        }

        // create an instance of the class
        m_pPlatformProxy = create_proxy();

        if (m_pPlatformProxy)
        {
            m_pPlatformProxy->SetCtrlCmdCallback(RecvCtrlCmdCallback);
        }
    }

    ACE_Thread_Manager::instance()->spawn_n(MAX_HANDLE_THREADS, ThreadHandleMessage, this);
    ACE_Thread_Manager::instance()->spawn(ThreadConnectToEventServerFunc, this);

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
        } 
        catch (...)
        {
            syslog(LOG_DEBUG, "crash Message : %d ...\n", pMsg_->msg_head.msg_type);
            continue;
        }
    }
}

int CMSG_Center::ConnectToEventServer()
{
    int nPASS_PORT = atoi(CSimpleConfig::PASS_PORT.c_str());
    CControlCmdHandler * handler = new CControlCmdHandler;

    while (1)
    {
        if (!CControlCmdHandler::m_bConnected)
        {
            if (handler->ConnectToAlarmServer(nPASS_PORT, CSimpleConfig::SERIAL_SERVER_IP2.c_str(), this) == -1)
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
        syslog(LOG_DEBUG, "--------msg to handle %d! \n", msgToHandleList_.size());
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

    syslog(LOG_DEBUG, "recv channel %s register...\n", pRegisterReq->device_id);
    
    
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
        syslog(LOG_DEBUG, "publish event succ...\n");
    } 
    else
    {
        syslog(LOG_DEBUG, "publish event fail...\n");
    }

    return 0;
}


int CMSG_Center::sendGatherInfoToEsealPlatform(const std::string& strSendData)
{
    int nEsealPort = atoi(CSimpleConfig::ESEAL_SERVER_PORT.c_str());
    if (nEsealPort <= 0)
        return 1;
    
    if (CSimpleConfig::ESEAL_SERVER_IP.empty())
        return 1;
    
    
    
    
    
    int send_socket = SysUtil::CreateSocket();

    struct timeval timeo = {10, 0};
    socklen_t len = sizeof(timeo);
    timeo.tv_sec = 10;
    setsockopt(send_socket, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);

    struct timeval timeo2 = {3 * 60, 0};
    socklen_t len2 = sizeof(timeo2);
    timeo2.tv_sec = 3 * 60 ;            
    setsockopt(send_socket, SOL_SOCKET, SO_RCVTIMEO, &timeo2, len2);            


    int canSend = -1; // 操作返回值
    if (SysUtil::ConnectSocket(send_socket, CSimpleConfig::ESEAL_SERVER_IP.c_str(), nEsealPort) != 0)
    {
        canSend = 1;  // 网络连接异常
        SysUtil::CloseSocket(send_socket);
        syslog(LOG_DEBUG, "********connect to eseal-server %s:%d fail...\n", CSimpleConfig::ESEAL_SERVER_IP.c_str(), nEsealPort);
        return -1;
    }

    int nRet = SysUtil::SocketWrite(send_socket, (char*)strSendData.c_str(), strSendData.size(), 1000);

    if (nRet == strSendData.size())
    {
        syslog(LOG_DEBUG, "send gather message to eseal-server %s:%d succ %d...\n", CSimpleConfig::ESEAL_SERVER_IP.c_str(), nEsealPort, nRet);
    }


    char szRecvBuffer[2048] = {0};
    int nFeedbackLen = SysUtil::SocketRead(send_socket, szRecvBuffer, 2047);
    if (nFeedbackLen > 0)
    {
        syslog(LOG_DEBUG, "recv feedback message from eseal-server:%d\n", nFeedbackLen);
        std::string rece_str = szRecvBuffer;
        // parse feedback message
        if (nFeedbackLen > 0)
        {
            if (rece_str != "" && rece_str.size() > 0)
            {
                std::string source          = rece_str;
                std::vector<std::string>    rece_StrArray;
                std::string division        = "&";
                SplitString(source, rece_StrArray, division);



                //string[] rece_StrArray = rece_str.Split('&');

                if (!rece_StrArray.empty() && rece_StrArray.size() > 3)
                {
                    // 根据返回值判定 是否都执行成功
                    if (rece_StrArray[3] == "success" && rece_StrArray[5] == "success")
                    {
                        canSend = 0;
                    }
                    else
                    {
                        if (rece_StrArray[3] == "noSearch" || rece_StrArray[5] == "noSearch")
                        {
                            canSend = 4;   // 没有搜索到电子封制
                        }
                        else if (rece_StrArray[3] == "noOper" || rece_StrArray[5] == "noOper")
                        {
                            canSend = 5;   // 电子封制操作不成功
                        }
                        else if (rece_StrArray[3] == "noNetGetData" || rece_StrArray[5] == "noNetGetData")
                        {
                            canSend = 6;   // 根据箱号获取封号网络异常
                        }
                        else if (rece_StrArray[3] == "checkSealError" || rece_StrArray[5] == "checkSealError")
                        {
                            canSend = 7;   // 验封状态异常
                        }
                        else if (rece_StrArray[3] == "noStatus" || rece_StrArray[5] == "noStatus")
                        {
                            canSend = 8;  // 验封信息失败
                        }
                        else if (rece_StrArray[3] == "noLine" || rece_StrArray[5] == "noLine")
                        {
                            canSend = 9;   // 锁杆异常  
                        }
                        else if (rece_StrArray[3] == "noMatchReader" || rece_StrArray[5] == "noMatchReader")
                        {
                            canSend = 10;   //  获取的封志个数与实际读取的电子封志个数不匹配
                        }
                        else if (rece_StrArray[3] == "DCNoMatchLock" || rece_StrArray[5] == "DCNoMatchLock")
                        {
                            canSend = 11;  // 数据中心返回的箱号和封号个数不匹配
                        }
                        else if (rece_StrArray[3] == "DCReturnError" || rece_StrArray[5] == "DCReturnError")
                        {
                            canSend = 12;  // 核注返回信息异常
                        }
                        else if (rece_StrArray[3] == "noNetSendInfo" || rece_StrArray[5] == "noNetSendInfo")
                        {
                            canSend = 13;  // 核注网络异常
                        }
                        else if (rece_StrArray[3] == "noBoxNum" || rece_StrArray[5] == "noBoxNum")
                        {
                            canSend = 14;  // 箱号获取为空 
                        }
                        else if (rece_StrArray[3] == "insertDataFail" || rece_StrArray[5] == "insertDataFail")
                        {
                            canSend = 15;  // 插入数据库信息失败
                        }
                        else if (rece_StrArray[3] == "DCJudgeFail" || rece_StrArray[5] == "DCJudgeFail")
                        {
                            canSend = 16;  // 信息比对失败
                        }
                        else
                        {
                            canSend = 17;  // 其他异常  
                        }
                    }
                }
                else
                {
                    canSend = 3;
                }
            }
            else
            {
                canSend = 3;
            }
        }
        //
    }  
    else
    {
        canSend = 2;    // 接收返回值超时
        syslog(LOG_DEBUG, "recv feedback message from eseal-server failed timeout:%d\n", nFeedbackLen);
    }

    SysUtil::CloseSocket(send_socket);    
    return 0;
}




int CMSG_Center::handleUploadCustomsData(NET_PACKET_MSG* pPacket)
{
    if (!pPacket)
    {
        return -1;
    }
    syslog(LOG_DEBUG, "process gather message  ...\n");

//    int nProxyCount = pPacket->msg_head.proxy_count;
//    if (nProxyCount < 1)
//    {
//        return -1;
//    }
//
//    int connect_handle = pPacket->msg_head.net_proxy[nProxyCount - 1];
    T_Upload_Customs_Data* pUploadDataReq = (T_Upload_Customs_Data*) pPacket->msg_body;
    
    char szPacketData[1024 * 4] = {0};
    int nPacketLen = 0;

    //send to platform,发送到上级平台，可能有多个上级平台
    {
        std::string strEsealData = "";        
        {
            char szPlatformName[7 + 1] = {0};
            strcpy(szPlatformName, "CUSTOMS");
            {
                m_pPlatformProxy->PackUploadData(szPlatformName, pUploadDataReq->szCustomsData, pUploadDataReq->nCustomsDataLen, szPacketData, nPacketLen, 0, strEsealData);
            }

            // here serial com send
            //按照不同的平台，对数据重新打包
            int nGATHER_PORT = atoi(CSimpleConfig::GATHER_PORT.c_str());
            int send_socket = SysUtil::CreateSocket();
            
            struct timeval timeo = {5, 0};
            socklen_t len = sizeof(timeo);
            timeo.tv_sec = 5;
            setsockopt(send_socket, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);
            
            struct timeval timeo2 = {5, 0};
            socklen_t len2 = sizeof(timeo2);
            timeo2.tv_sec = 5;            
            setsockopt(send_socket, SOL_SOCKET, SO_RCVTIMEO, &timeo2, len2);            

            if (SysUtil::ConnectSocket(send_socket, CSimpleConfig::SERIAL_SERVER_IP.c_str(), nGATHER_PORT) != 0)
            {
                SysUtil::CloseSocket(send_socket);
                syslog(LOG_DEBUG, "********connect to serial-server %s:%d fail...\n", CSimpleConfig::SERIAL_SERVER_IP.c_str(), nGATHER_PORT);
            }
            else
            {
                int nRet = SysUtil::SocketWrite(send_socket, szPacketData, nPacketLen, 1000);

                if (nRet == nPacketLen)
                {
                    syslog(LOG_DEBUG, "send gather message to serial-server %s:%d succ %d...\n", CSimpleConfig::SERIAL_SERVER_IP.c_str(), nGATHER_PORT, nRet);
                }

                char szRecvBuffer[100 + 1] = {0};
                int nFeedbackLen = SysUtil::SocketRead(send_socket, szRecvBuffer, 100);
                //int nFeedbackLen = SysUtil::SocketRead_n(send_socket, szRecvBuffer, 100);
                if (nFeedbackLen == 100)
                {
                    syslog(LOG_DEBUG, "recv feedback message from serial-server:%d\n", nFeedbackLen);

                    // parse feedback message
                    structFeedback_Info *pFeedback = (structFeedback_Info *)szRecvBuffer;
                    if (pFeedback->szBody[0] == '\0')
                    {
                        syslog(LOG_DEBUG, "recv feedback message from serial-server succ:%d\n", nFeedbackLen);
                    }
                    else
                    {
                        syslog(LOG_DEBUG, "recv feedback message from serial-server failed:%d\n", nFeedbackLen);
                    }
                    //
                }  
                else
                {
                    syslog(LOG_DEBUG, "recv feedback message from serial-server succ2:%d\n", nFeedbackLen);
                }

                SysUtil::CloseSocket(send_socket);                
            }
        }
        sendGatherInfoToEsealPlatform(strEsealData);
    }


}


int CMSG_Center::parsePassInfo(NET_PACKET_MSG* pPacket, structCommandBean &command)
{
    T_Upload_Customs_Data* pCustomsDataAck = (T_Upload_Customs_Data*) pPacket->msg_body;
    
    if (pCustomsDataAck == NULL)
        return -1;
    
    if (pCustomsDataAck->szCustomsData == NULL)
        return -1;
    
    if (pCustomsDataAck->nCustomsDataLen < 8)
        return -1;
    
    char szRecvData[4096] = {0};
    memcpy(szRecvData, pCustomsDataAck->szCustomsData, pCustomsDataAck->nCustomsDataLen);
    
    unsigned char szXmlLen[4] = {0};
    szXmlLen[0] = szRecvData[37];
    szXmlLen[1] = szRecvData[38];
    szXmlLen[2] = szRecvData[39];
    szXmlLen[3] = szRecvData[40];
    int nXmlLen2 = 0;
    nXmlLen2 = (unsigned int)szXmlLen[0] + (unsigned int)(szXmlLen[1] << 8) + (unsigned int)(szXmlLen[2] << 16) + (unsigned int)(szXmlLen[3] << 24);

    if (nXmlLen2 <= 0 || nXmlLen2 > 4096)
    {
            return -1;
    }

    boost::shared_array<char> spRecvXmlData;
    spRecvXmlData.reset(new char[nXmlLen2 + 1]);
    char *pRecvXmlData = spRecvXmlData.get();
    memcpy(pRecvXmlData, szRecvData + 41, nXmlLen2);
    pRecvXmlData[nXmlLen2] = '\0';

    //syslog(LOG_DEBUG, "%s\n", pRecvXmlData);

    std::string AREA_ID		= "";
    std::string CHNL_NO		= "";
    std::string I_E_TYPE        = "";
    std::string SEQ_NO		= "";
    std::string BUSINESS_CODE   = "";
    std::string CHECK_RESULT    = "";
    std::string MESSAGE         = "";

    TiXmlDocument doc;
    doc.Parse(pRecvXmlData);
    TiXmlHandle docHandle(&doc);
    TiXmlHandle COMMAND_INFOHandle		= docHandle.FirstChildElement("COMMAND_INFO");
    if (COMMAND_INFOHandle.Node() != NULL)
            AREA_ID 		= COMMAND_INFOHandle.Element()->Attribute("AREA_ID");
    if (COMMAND_INFOHandle.Node() != NULL)
            CHNL_NO 		= COMMAND_INFOHandle.Element()->Attribute("CHNL_NO");
    if (COMMAND_INFOHandle.Node() != NULL)
            I_E_TYPE 		= COMMAND_INFOHandle.Element()->Attribute("I_E_TYPE");
    if (COMMAND_INFOHandle.Node() != NULL)
            SEQ_NO 		= COMMAND_INFOHandle.Element()->Attribute("SEQ_NO");
    if (COMMAND_INFOHandle.Node() != NULL)
            BUSINESS_CODE 		= COMMAND_INFOHandle.Element()->Attribute("BUSINESS_CODE");  
    // start
    TiXmlHandle CHECK_RESULTHandle  = docHandle.FirstChildElement("COMMAND_INFO").ChildElement("CHECK_RESULT", 0).FirstChild();
    if (CHECK_RESULTHandle.Node() != NULL)
            CHECK_RESULT	= CHECK_RESULTHandle.Text()->Value();   
    
    TiXmlHandle MESSAGEHandle  = docHandle.FirstChildElement("COMMAND_INFO").ChildElement("MESSAGE", 0).FirstChild();
    if (MESSAGEHandle.Node() != NULL)
            MESSAGE	= MESSAGEHandle.Text()->Value();      
    
    
    char szNOTE[256]   = {0};
    CUtilTools::Utf8ToGb2312(szNOTE, 255, MESSAGE.c_str(), MESSAGE.size());
    szNOTE[255] = '\0';
    MESSAGE = szNOTE;    
    
    boost::algorithm::trim(AREA_ID);
    boost::algorithm::trim(CHNL_NO);     
    
    
    std::string str1 = "0000";
    str1 += AREA_ID;
    
    
    std::string str2 = "00000000";
    str2 += CHNL_NO;
    
    command.AREA_ID  = str1;
    command.CHNL_NO  = str2;
    command.I_E_TYPE = I_E_TYPE;
    command.SEQ_NO   = SEQ_NO;
    command.BUSINESS_CODE = BUSINESS_CODE;
    command.CHECK_RESULT  = CHECK_RESULT;
    command.MESSAGE       = MESSAGE;
    
//    //===================================
//    //
//    //10	标识符	0xFF 0xFF 0xFF 0xFF	取固定值，为方便识别后文数字签名  + 4
//    //11	数字签名长度		数字签名的长度（4 bytes）               + 4
//    //12	数字签名	长度等于RSA密钥长度	数字签名                      + 128
//    //13	包尾	0xFF 0xFF	取固定值
//
//    
//    unsigned char szSignLen[4] = {0};
//    szSignLen[0] = szRecvData[45 + nXmlLen2];
//    szSignLen[1] = szRecvData[46 + nXmlLen2];
//    szSignLen[2] = szRecvData[47 + nXmlLen2];
//    szSignLen[3] = szRecvData[48 + nXmlLen2];
//    int nSignedLen = 0;
//    nSignedLen = (unsigned int)szSignLen[0] + (unsigned int)(szSignLen[1] << 8) + (unsigned int)(szSignLen[2] << 16) + (unsigned int)(szSignLen[3] << 24);
//
//    if (nSignedLen <= 0)
//    {
//            return -1;
//    }
//    
//    
//    
//    char szSignedData[128 + 1] = {0};
//    memcpy(szSignedData, szRecvData + 49 + nXmlLen2, nSignedLen);
//    
//    
//    std::string strPassXml = pRecvXmlData;
//    Encrypter crypt;
//    std::string strMessageHash = crypt.GetMessageHash(strPassXml);
//    std::string strPubFile = "/root/zhouhm/myopenssl/test/b-to-pub.pem";
//    std::string strDecodeMessageHash = crypt.DecodeRSAKeyFile(strPubFile, szSignedData, nSignedLen);
//
//    syslog(LOG_DEBUG, "recv pass xml messsage:%s\n", strPassXml.c_str());
//    syslog(LOG_DEBUG, "public key file:%s\n", strPubFile.c_str());
//    syslog(LOG_DEBUG, "generate digust messsage:%s\n", strMessageHash.c_str());
//    syslog(LOG_DEBUG, "parse digust messsage:%s\n", strDecodeMessageHash.c_str());
//    //      

    return 0;
}

int CMSG_Center::generatePassXml(const structCommandBean &command, std::string &strPassXml)
{
// generate xml
    std::string AREA_ID     = command.AREA_ID;
    std::string CHNL_NO     = command.CHNL_NO;
    std::string I_E_TYPE    = command.I_E_TYPE;
    std::string SEQ_NO      = command.SEQ_NO; 
    
    
    std::string strStationNo = command.AREA_ID;
    std::string strChnlNo    = command.CHNL_NO;  
    boost::algorithm::trim(strStationNo);
    boost::algorithm::trim(strChnlNo);    
    char szAreaNo[10 + 1] = {0};
    char szChnlNo[10 + 1] = {0};
    sprintf(szAreaNo, "%010s", strStationNo.c_str());
    sprintf(szChnlNo, "%010s", strChnlNo.c_str());    
    

    using boost::property_tree::ptree;
    ptree pt;
    ptree pattr1;


    pattr1.add<std::string>("<xmlattr>.AREA_ID", szAreaNo);
    pattr1.add<std::string>("<xmlattr>.CHNL_NO", szChnlNo);
    pattr1.add<std::string>("<xmlattr>.I_E_TYPE", I_E_TYPE);
    pattr1.add<std::string>("<xmlattr>.SEQ_NO", SEQ_NO);

    pt.add_child("COMMAND_INFO", pattr1);

    pt.put("COMMAND_INFO.CHECK_RESULT", command.CHECK_RESULT);

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
    pt.put("COMMAND_INFO.OP_HINT", command.MESSAGE);  


    // 格式化输出，指定编码（默认utf-8）
    boost::property_tree::xml_writer_settings<char> settings('\t', 1,  "GB2312");
    //write_xml(filename, pt, std::locale(), settings);

    ostringstream oss;
    write_xml(oss, pt, settings);

    strPassXml = oss.str();   
    
    syslog(LOG_DEBUG, "pass xml:%s\n", strPassXml.c_str());
    return 0;
}

int CMSG_Center::buildPassInfo(const structCommandBean &command, const std::string& strPassXml, char* pData, int& nSendLen)
{
    if (strPassXml == "")
        return -1;
    
    char chFlag = command.I_E_TYPE.size() > 0 ? command.I_E_TYPE[0] : '\0';
    char szIEFlag[2]         = {chFlag, '\0'}; 
    std::string strStationNo = command.AREA_ID;
    std::string strChnlNo    = command.CHNL_NO;      
    std::string strSendXml   = strPassXml;

    boost::algorithm::trim(strStationNo);
    boost::algorithm::trim(strChnlNo);       
    
    const char packetHead[4]    = {(char)0xE2, (char)0x5C, (char)0x4B, (char)0x89};
    const char packetEnd[2]     = {(char)0xFF, (char)0xFF};
    char szAreaNo[10 + 1] = {0};
    char szChnlNo[10 + 1] = {0};
    sprintf(szAreaNo, "%010s", strStationNo.c_str());
    sprintf(szChnlNo, "%010s", strChnlNo.c_str());
  
    int nXmlLen = strSendXml.size();
    int PackDataLen = 34 + 4 + nXmlLen + 2;

    boost::shared_array<BYTE> spPacketData;
    spPacketData.reset(new BYTE[PackDataLen + 1]);
    bzero(spPacketData.get(), PackDataLen + 1);

    BYTE *p = spPacketData.get();
    memcpy(p, packetHead ,4);
    p += 4;

    memcpy(p, (BYTE*)&PackDataLen, sizeof(int));
    p += 4;

    char szDataType[1 + 1] = {0x22, 0x00};
    memcpy(p, szDataType, 1);
    p += 1;	

    memcpy(p, szAreaNo, 10);
    p += 10;

    memcpy(p, szChnlNo, 10);
    p += 10;

    memcpy(p, szIEFlag, 1);
    p += 1;							

    memset(p, 0x00, 4);
    p += 4;	

    memcpy(p, &nXmlLen, 4);
    
    p+=4;
    memcpy(p, strSendXml.data(), nXmlLen);
    
    p += nXmlLen;
    
    memcpy(p, packetEnd ,2);
    p += 2;
    // end

    memcpy(pData, spPacketData.get(), PackDataLen);  
    
    nSendLen = PackDataLen;
    return 0;
}


int CMSG_Center::handleCustomsDataAck(NET_PACKET_MSG* pPacket)
{
    structCommandBean command;
    int nRet = parsePassInfo(pPacket, command);
    if (nRet == -1)
        return -1;

    std::string strPassXml = "";
    generatePassXml(command, strPassXml);
    
    // send
    char szPacketData[1024 * 2] = {0};
    int nPacketLen = 0;  
    nRet = buildPassInfo(command, strPassXml, szPacketData, nPacketLen);
    if (nRet == -1)
        return -1;

    //send to platform,发送到上级平台，可能有多个上级平台
    {
        {
            // here serial com send
            //按照不同的平台，对数据重新打包
            int nCENTER_SERVER_PORT = atoi(CSimpleConfig::CENTER_SERVER_PORT.c_str());
            int send_socket = SysUtil::CreateSocket(); 
            
            if (SysUtil::ConnectSocket(send_socket, CSimpleConfig::CENTER_SERVER_IP.c_str(), nCENTER_SERVER_PORT) != 0)
            {
                SysUtil::CloseSocket(send_socket);
                syslog(LOG_DEBUG, "********connect to center server %s:%d fail...\n", CSimpleConfig::CENTER_SERVER_IP.c_str(), nCENTER_SERVER_PORT);
                return -1;
            }

            int nRet = SysUtil::SocketWrite(send_socket, szPacketData, nPacketLen, 1000);

            if (nRet == nPacketLen)
            {
                syslog(LOG_DEBUG, "send pass message to center server %s:%d succ %d...\n", CSimpleConfig::CENTER_SERVER_IP.c_str(), nCENTER_SERVER_PORT, nRet);
            }
            else
            {
                syslog(LOG_DEBUG, "send pass message to center server %s:%d failed %d...\n", CSimpleConfig::CENTER_SERVER_IP.c_str(), nCENTER_SERVER_PORT, nRet);
            }
            
            SysUtil::CloseSocket(send_socket);
        }
    }
    
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

                syslog(LOG_DEBUG, "********channel %s offline......\n", pChannelStatus->channel_id);
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

    //    syslog(LOG_DEBUG, "recv connect %d keeplive...\n", connect_handle);


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
            syslog(LOG_DEBUG, "register channel %s succ...\n", channel_id);
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


    syslog(LOG_DEBUG, "handle ctrl  %s.. \n", pCtrlCmdData->szSequenceNo);

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
        syslog(LOG_DEBUG, "publish pass cmd event succ...\n");
    } 
    else
    {
        syslog(LOG_DEBUG, "publish pass cmd event fail...\n");
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
        syslog(LOG_DEBUG, "channel connect handle is %d\n", channel_connect_handle);
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
        syslog(LOG_DEBUG, "publish event succ...\n");
    } 
    else
    {
        syslog(LOG_DEBUG, "publish event fail...\n");
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


    sprintf(szSavePath, "%s/%s", CSimpleConfig::PIC_PATH.c_str(), pPicData->szChannelNo);


    syslog(LOG_DEBUG, "publish event succ...%s\n", szSavePath);

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
    
    
    
    
//
//    //send to platform,发送到上级平台，可能有多个上级平台
//    //if (CSimpleConfig::m_StaPlatformMap.size() > 0)
//    {
//       
//        //for (iter = CSimpleConfig::m_StaPlatformMap.begin(); iter != CSimpleConfig::m_StaPlatformMap.end(); iter++)
//        {
//        //    pPlatformInfo = iter->second;
//            
//        //    strcpy(szPlatformName, iter->first.c_str());
//            
//            
//            
////    {
////        std::string strFileNameNew = "/root/zhouhm/testpic/";
////        strFileNameNew += "ce_";
////        strFileNameNew += pPicData->szSequenceNo;
////        strFileNameNew += ".dat";
////        FILE* pImageFile = fopen(strFileNameNew.c_str(), "wb");
////        if (!pImageFile)
////        {
////            return -1;
////        }     
////        WriteAll(pImageFile, (char*)spPacketData.get(), PackDataLen);
////        fclose(pImageFile);
////    }
//            
//            
//
//            //按照不同的平台，对数据重新打包
//            send_socket = SysUtil::CreateSocket();
//            if (SysUtil::ConnectSocket(send_socket, pPlatformInfo->platform_ip, pPlatformInfo->platform_port) != 0)
//            {
//                SysUtil::CloseSocket(send_socket);
//                CMyLog::m_pLog->_XGSysLog("********connect to platform %s:%d fail...\n", pPlatformInfo->platform_ip, pPlatformInfo->platform_port);
//                return -1;
//            }
//
//            nRet = SysUtil::SocketWrite(send_socket,  (char*)spPacketData.get(), PackDataLen, 1000);
//
//            if (nRet == PackDataLen)
//            {
//                CMyLog::m_pLog->_XGSysLog("send pic data to platform %s:%d succ %d...\n", pPlatformInfo->platform_ip, pPlatformInfo->platform_port, nRet);
//            }
//
//            SysUtil::CloseSocket(send_socket);
//        }
//    }
    
}

int CMSG_Center::handleRegatherData(NET_PACKET_MSG* pMsg)
{
    syslog(LOG_DEBUG, "recv regather req!\n");
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
    } 
    else //设备方式,把报文发送给通道控制器处理
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
            syslog(LOG_DEBUG, "send device regather cmd to channelcontrolserver succ...\n");
        } else
        {
            syslog(LOG_DEBUG, "send device regather cmd to channelcontrolserver fail...\n");
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
    return 0;
}

int CMSG_Center::handleExceptionFree(NET_PACKET_MSG* pMsg)
{
	return 0;
}

int CMSG_Center::RecordContaPicInfo(char* szSeqNo, int nPicType, char* szFileName, const std::string &strAreaNo, const std::string &strChannelNo)
{
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



void CMSG_Center::SplitString(std::string source, std::vector<std::string>& dest, const std::string& division)
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
