#if !defined _PLAT_CMD_ACCEPTOR_H_
#define _PLAT_CMD_ACCEPTOR_H_


#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "PlatCmdHandler_T.h"
#include "MyLog.h"
#include <syslog.h>

#include <set>
#include <list>
using namespace std;

#define MAX_PLATCMD_HANDLER_NUM        30        // 5-30 2015-11-30

struct DispatchPlatCmdHandlerInfo {
    int nHandlerID;
    CPlatCmdHandler_T* pCmdHandler;
    int nUseFlag;
};

class CPlat_Cmd_Acceptor : public ACE_Acceptor<CPlatCmdHandler_T, ACE_SOCK_ACCEPTOR> {
public:
    CPlat_Cmd_Acceptor(void);
    ~CPlat_Cmd_Acceptor(void);
public:
    

    static int InitHandler() {

        for (int i = 0; i < MAX_PLATCMD_HANDLER_NUM; i++) {
            CPlatCmdHandler_T* handler = new CPlatCmdHandler_T();
            handler->SetOjbID(m_nObjNo);
            
            char szMsg[256]={0};
            sprintf(szMsg,"CCmd_Acceptor::InitHandler:new CCmdHandler_T\n");
            CMyLog::RegisterResourceInfo(handler, szMsg);

            DispatchPlatCmdHandlerInfo dispatchRealhandlerInfo;
            dispatchRealhandlerInfo.nHandlerID = m_nObjNo;
            dispatchRealhandlerInfo.nUseFlag = 0;
            dispatchRealhandlerInfo.pCmdHandler = handler;

            m_DispatchPlatCmdHandlerInfo[i] = dispatchRealhandlerInfo;

            m_nObjNo++;

/*
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1000;
            select(0, 0, NULL, NULL, &tv);
*/

        }

        m_bInitOK = true;

        return 0;

    }
    static int free_handler(int nID);

    static bool m_bInitOK;

protected:

    virtual int make_svc_handler(CPlatCmdHandler_T*& handler) {
        int nIndexOld = 0;
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handler_lock, -1);

        bool bFind = false;
        for (int i = m_nDispatchPlatCmdHandlerIndex; i < MAX_PLATCMD_HANDLER_NUM; i++) {
            if (m_DispatchPlatCmdHandlerInfo[i].nUseFlag == 0) {
                m_DispatchPlatCmdHandlerInfo[i].nUseFlag = 1;
                handler = m_DispatchPlatCmdHandlerInfo[i].pCmdHandler;
                nIndexOld = m_nDispatchPlatCmdHandlerIndex;
                m_nDispatchPlatCmdHandlerIndex++;
                if (m_nDispatchPlatCmdHandlerIndex > MAX_PLATCMD_HANDLER_NUM - 1) {
                    m_nDispatchPlatCmdHandlerIndex = 0;
                    
                }

                bFind = true;
                break;
            }
        }

        if (!bFind) {
            for (int i = 0; i < m_nDispatchPlatCmdHandlerIndex; i++) {
                if (m_DispatchPlatCmdHandlerInfo[i].nUseFlag == 0) {
                    m_DispatchPlatCmdHandlerInfo[i].nUseFlag = 1;
                    handler = m_DispatchPlatCmdHandlerInfo[i].pCmdHandler;
                    m_nDispatchPlatCmdHandlerIndex = i;
                    nIndexOld = m_nDispatchPlatCmdHandlerIndex;
                    bFind = true;
                    break;
                }
            }
        }

        if (!bFind) {
            if (!bFind) {
                handler = m_pFakeRealhandler;

                char szMsg[256] = {0};
                sprintf(szMsg, "make fake client handle %p! \n", handler);
                //CMyLog::m_pLog->_XGSysLog(szMsg);
                syslog(LOG_DEBUG, "%s", szMsg);
            }
        }


        char szMsg[256] = {0};
        sprintf(szMsg, "current cmd handler is %d,%p\n", nIndexOld, handler);
        //CMyLog::m_pLog->_XGSysLog(szMsg);
        syslog(LOG_DEBUG, "%s", szMsg);


        return 0;
    }

private:
    CPlatCmdHandler_T* m_pFakeRealhandler;
    static DispatchPlatCmdHandlerInfo m_DispatchPlatCmdHandlerInfo[MAX_PLATCMD_HANDLER_NUM];
    static int m_nDispatchPlatCmdHandlerIndex;


    static ACE_Thread_Mutex handler_lock;
    static int m_nObjNo;
    static int m_nCheckCount;
    static int m_nCheckIndex;


};

#endif

