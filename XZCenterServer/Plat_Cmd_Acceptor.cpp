#include "Plat_Cmd_Acceptor.h"

DispatchPlatCmdHandlerInfo CPlat_Cmd_Acceptor::m_DispatchPlatCmdHandlerInfo[MAX_PLATCMD_HANDLER_NUM];
int CPlat_Cmd_Acceptor::m_nDispatchPlatCmdHandlerIndex = 0;
int CPlat_Cmd_Acceptor::m_nCheckIndex = 0;

ACE_Thread_Mutex CPlat_Cmd_Acceptor::handler_lock;
int CPlat_Cmd_Acceptor::m_nCheckCount = 0;
int CPlat_Cmd_Acceptor::m_nObjNo = 0;
bool CPlat_Cmd_Acceptor::m_bInitOK = false;

CPlat_Cmd_Acceptor::CPlat_Cmd_Acceptor(void) {
}

CPlat_Cmd_Acceptor::~CPlat_Cmd_Acceptor(void) {
}

int CPlat_Cmd_Acceptor::free_handler(int nID) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handler_lock, -1);
    m_DispatchPlatCmdHandlerInfo[nID].nUseFlag = 0;
    return 0;
}




