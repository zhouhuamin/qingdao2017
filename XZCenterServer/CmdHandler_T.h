// CmdHandler_T.h: interface for the CCmdHandler_T class.
//
//////////////////////////////////////////////////////////////////////

#if !defined _C_CMD_HANDLER_T_H_
#define _C_CMD_HANDLER_T_H_

#include "ace/Svc_Handler.h"
#include "ace/SOCK_Stream.h"
#include "SysMessage.h"



class CCmdHandler_T : public ACE_Svc_Handler <ACE_SOCK_STREAM, ACE_NULL_SYNCH> {
public:
    int VerifyRecvPacket(char *chRecvBuffer, char* chDest, int &nRecLen, int &nOffset, int &nLen);
    int SearchTailPos(char *chBuffer, int nDataLen);
    int SearchHeadPos(char *chBuffer, int nDataLen);
    typedef ACE_Svc_Handler <ACE_SOCK_STREAM, ACE_NULL_SYNCH> inherited;
    CCmdHandler_T();


    void destroy(void);

    int open(void *acceptor);

    int close(u_long flags = 0);

    virtual int handle_input(ACE_HANDLE handle);
    virtual int handle_close(ACE_HANDLE handle = ACE_INVALID_HANDLE,
            ACE_Reactor_Mask mask = ACE_Event_Handler::ALL_EVENTS_MASK);

    virtual int svc(void);
    int stop();

    int SetOjbID(int nID) {
        m_nObjID = nID;
        return 0;
    };

public:
    int m_nObjID;
protected:
    int process();
    virtual ~CCmdHandler_T();

private:
    //recv buffer to hold recv data
    char* chRecvBuffer;
    char* chDest;
    //recv data pointer,maybe one recv can't recv total packet or over a packet 
    int dwRecvBuffLen;
    bool bStopFlag_; //Õ£÷π±Í÷æ




};

#endif 
