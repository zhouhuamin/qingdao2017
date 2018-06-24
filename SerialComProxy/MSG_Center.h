// MSG_Center.h: interface for the MSG_Task class.
#if !defined _MSG_CENTER_
#define _MSG_CENTER_

#include <stdint.h>
#include <list>
#include <string>
#include <vector>
#include <map>

using namespace std;

#include "ace/Signal.h"
#include "ace/streams.h"
#include "ace/Task.h"
#include "ace/Semaphore.h"
#include "PlatProxy.h"


#define MAX_BUFFERD_MESSAGE_NUMBER             (100)
#define MAX_HANDLE_THREADS                        1

typedef unsigned char BYTE;

struct T_ChannelStatus
{
    char  channel_id[32];
    int   connect_handle;
    int   last_active;
    int   is_active;
};

struct T_SequenceStatus
{
    char  channel_id[32];
    int   connect_handle;
    int   last_active;
};

struct T_CtrlCmd
{
    char szAreaID[16];
    char szChannelNo[16];
    char szIEType[4];
    char szSequenceNo[64];
    int nLen;
    char szCtrlData[];
};


struct structCommandBean
{
    std::string AREA_ID;
    std::string CHNL_NO;
    std::string I_E_TYPE;
    std::string SEQ_NO;
    std::string BUSINESS_CODE;
    std::string CHECK_RESULT;
};



class CMSG_Center : public ACE_Task<ACE_SYNCH>
{
    // Instantiated with an MT synchronization trait.
public:

    enum
    {
        MAX_THREADS = 1
    };

    CMSG_Center();
    ~CMSG_Center(void);

    virtual int open();

    virtual int put(NET_PACKET_MSG* pMsg);

    virtual int handle_timeout(const ACE_Time_Value &tv, const void *arg);

public:
    
    int HandleNetMessage(NET_PACKET_MSG *pPacket);
    int handleChannelRegister(NET_PACKET_MSG* pPacket);
    int handleUploadCustomsData(NET_PACKET_MSG* pPacket);
    
    int parsePassInfo(NET_PACKET_MSG* pPacket, structCommandBean &command);
    int generatePassXml(const structCommandBean &command, std::string &strPassXml);
    int buildPassInfo(const structCommandBean &command, const std::string &strPassXml, char *pData, int &nSendLen);
    
    int handleCustomsDataAck(NET_PACKET_MSG* pPacket);
    int handleCtrlCmd(NET_PACKET_MSG* pPacket);
    int handleContaPicData(NET_PACKET_MSG* pPacket);
    int handleRegatherData(NET_PACKET_MSG* pMsg);
    int publish_gather_event(T_Upload_Customs_Data* pUploadDataReq);
    int handleDeviceCtrl(NET_PACKET_MSG* pMsg);

	// 2015-5-6
	int handleExceptionFree(NET_PACKET_MSG* pMsg);

    int RecordContaPicInfo(char* szSeqNo,int nPicType,char* szFileName, const std::string &strAreaNo, const std::string &strChannelNo);
    
    int handleChannelPassAck(T_CtrlCmd* pCtrlCmd);
    int publish_pass_ack(T_CtrlCmd* pCtrlCmd);
    
    
    int PackBasicCustomsData(char* szAraeID,char* szChannelNo,char* szIEType,char* szXML,int nXMLLen,char* szDestData,int& nPackedLen);
  
    
    int registerChannel(char* channel_id);
    int channel_exit(int handle);

    int ConnectToEventServer();

    int setEventhandle(int handle)
    {
        event_handle=handle;
    }
    
    int CheckUpdatedSequence();
  
    static int init_message_buffer();
    static int handleChannelKeeplive(NET_PACKET_MSG* pPacket);
    static int get_message(NET_PACKET_MSG*& pMsg);
    static ACE_THR_FUNC_RETURN ThreadHandleMessage(void *arg);
    static ACE_THR_FUNC_RETURN ThreadSuicide(void *arg);
    size_t	WriteAll(FILE *fd, void *buff, size_t len);
    std::list<NET_PACKET_MSG*> msgToHandleList_;
    ACE_Semaphore msg_semaphore_;
    ACE_Thread_Mutex msg_handle_lock_;

    bool m_bStartOk;

private:

public:
    int         event_handle;
    
    static bool m_bWork;
    static ACE_Thread_Mutex msg_lock_;
    static std::vector<NET_PACKET_MSG*> msgVec_;
    static int m_nMsgBufferCount;
    
    static ACE_Thread_Mutex channel_lock_;
    static std::map<string,T_ChannelStatus*> m_StaChannelStatusMap;


    CPlatformProxy*    m_pPlatformProxy;
   
    ACE_Thread_Mutex seq_lock_;
    std::map<string,T_SequenceStatus> m_StaChannelConnectMap;
  
};

typedef ACE_Singleton<CMSG_Center, ACE_Null_Mutex> MSG_CENTER;

#endif // !defined(AFX_MSG_TASK_H__69346340_854F_4162_801C_94B418746B85__INCLUDED_)
