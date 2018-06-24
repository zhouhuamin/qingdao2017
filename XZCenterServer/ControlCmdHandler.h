// ControlCmdHandler.h: interface for the CControlCmdHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined _CONTROL_CMD_HANDLER_H_
#define _CONTROL_CMD_HANDLER_H_

#include "SysMessage.h"
#include <vector>
#include <string>

using namespace std;

class CMSG_Center;
class CControlCmdHandler
{
public:
    int VerifyRecvPacket(char *chRecvBuffer, char* chDest, int &nRecLen, int &nOffset, int &nLen);
    int set_keep_live(int keep_alive_times, int keep_alive_interval);
    int SendHeartbeatMsg();
    int ReceiveTimer(int fd); //检测是否有网络数据
    CControlCmdHandler(); //构造函数
    virtual ~CControlCmdHandler(); //析构函数


    int open(void*); //初始化连接
    int ConnectToAlarmServer(short port, char* chIP, CMSG_Center *pCenter); //连接到ICS
    int svc(); //线程体
    int recvData(); //接收数据处理
    
    int registerChannel(char* channel_id, int socket);

    static bool m_bConnected; //是否处于连接状态
private:
    //recv buffer to hole recv data
    char* chRecvBuffer;
    char* chDest;
    
    //recv data pointer,maybe one recv can't recv total packet or over a packet
    unsigned long dwRecvBuffLen;
    int socket_; //连接句柄
    bool bGoWork_; //继续运行标志
    bool bThread_flag_; //线程启动标志

};

#endif 
