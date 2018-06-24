#include "Cmd_Acceptor.h"

DispatchCmdHandlerInfo CCmd_Acceptor::m_DispatchRealStreamHandlerInfo[MAX_CMD_HANDLER_NUM];
int CCmd_Acceptor::m_nDispatchRealStreamHandlerIndex = 0;
int CCmd_Acceptor::m_nCheckIndex = 0;

ACE_Thread_Mutex CCmd_Acceptor::handler_lock;
int CCmd_Acceptor::m_nCheckCount = 0;
int CCmd_Acceptor::m_nObjNo = 0;
bool CCmd_Acceptor::m_bInitOK = false;

CCmd_Acceptor::CCmd_Acceptor(void) {
}

CCmd_Acceptor::~CCmd_Acceptor(void) {
}

int CCmd_Acceptor::free_handler(int nID) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handler_lock, -1);
    m_DispatchRealStreamHandlerInfo[nID].nUseFlag = 0;
    return 0;
}

int CCmd_Acceptor::GetCurLoad() {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handler_lock, -1);
    int nCount = 0;
    for (int i = 0; i < MAX_CMD_HANDLER_NUM; i++) {
        if(m_DispatchRealStreamHandlerInfo[i].nUseFlag == 1){
            nCount++;
        }
    }

    return nCount;
}



