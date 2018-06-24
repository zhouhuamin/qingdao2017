#if !defined _C_CMD_ACCEPTOR_H_
#define _C_CMD_ACCEPTOR_H_


#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"
#include "CmdHandler_T.h"
#include "MyLog.h"

#include <set>
#include <list>
using namespace std;

#define MAX_CMD_HANDLER_NUM   15     

struct DispatchCmdHandlerInfo {
    int nHandlerID;
    CCmdHandler_T* pCmdHandler;
    int nUseFlag;
};

class CCmd_Acceptor : public ACE_Acceptor<CCmdHandler_T, ACE_SOCK_ACCEPTOR> {
public:
    CCmd_Acceptor(void);
    ~CCmd_Acceptor(void);
public:
    static int GetCurLoad();

    /*
     *	�ڳ���������ʱ���ȴ���������Ҫ���̴߳������?
     */
    static int InitHandler() {

        for (int i = 0; i < MAX_CMD_HANDLER_NUM; i++) {
            CCmdHandler_T* handler = new CCmdHandler_T();
            handler->SetOjbID(m_nObjNo);
            
            char szMsg[256]={0};
            sprintf(szMsg,"CCmd_Acceptor::InitHandler:new CCmdHandler_T\n");
            CMyLog::RegisterResourceInfo(handler, szMsg);

            DispatchCmdHandlerInfo dispatchRealhandlerInfo;
            dispatchRealhandlerInfo.nHandlerID = m_nObjNo;
            dispatchRealhandlerInfo.nUseFlag = 0;
            dispatchRealhandlerInfo.pCmdHandler = handler;

            m_DispatchRealStreamHandlerInfo[i] = dispatchRealhandlerInfo;

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

    virtual int make_svc_handler(CCmdHandler_T*& handler) {
        int nIndexOld = 0;
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, handler_lock, -1);

        bool bFind = false;
        for (int i = m_nDispatchRealStreamHandlerIndex; i < MAX_CMD_HANDLER_NUM; i++) {
            if (m_DispatchRealStreamHandlerInfo[i].nUseFlag == 0) {
                m_DispatchRealStreamHandlerInfo[i].nUseFlag = 1;
                handler = m_DispatchRealStreamHandlerInfo[i].pCmdHandler;
                nIndexOld = m_nDispatchRealStreamHandlerIndex;
                m_nDispatchRealStreamHandlerIndex++;
                if (m_nDispatchRealStreamHandlerIndex > MAX_CMD_HANDLER_NUM - 1) {
                    m_nDispatchRealStreamHandlerIndex = 0;
                }

                bFind = true;
                break;
            }
        }

        if (!bFind) {
            for (int i = 0; i < m_nDispatchRealStreamHandlerIndex; i++) {
                if (m_DispatchRealStreamHandlerInfo[i].nUseFlag == 0) {
                    m_DispatchRealStreamHandlerInfo[i].nUseFlag = 1;
                    handler = m_DispatchRealStreamHandlerInfo[i].pCmdHandler;
                    m_nDispatchRealStreamHandlerIndex = i;
                    nIndexOld = m_nDispatchRealStreamHandlerIndex;
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
                CMyLog::m_pLog->_XGSysLog(szMsg);
            }
        }


        char szMsg[256] = {0};
        sprintf(szMsg, "current cmd handler is %d,%p\n", nIndexOld, handler);
        CMyLog::m_pLog->_XGSysLog(szMsg);


        return 0;
    }

private:
    CCmdHandler_T* m_pFakeRealhandler;
    static DispatchCmdHandlerInfo m_DispatchRealStreamHandlerInfo[MAX_CMD_HANDLER_NUM];
    static int m_nDispatchRealStreamHandlerIndex;


    static ACE_Thread_Mutex handler_lock;
    static int m_nObjNo;
    static int m_nCheckCount;
    static int m_nCheckIndex;


};

#endif

