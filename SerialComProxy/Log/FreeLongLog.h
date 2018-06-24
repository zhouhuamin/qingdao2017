// FreeLongLog.h: interface for the CFreeLongLog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined _FREELONG_LOG_H_
#define _FREELONG_LOG_H_
#include "MutexLock.h"
#include <vector>
using namespace std;



#define LOG_FILE_SIZE_MAX                         (1*1024*1024*1024)   //单个日志文件最大长度
#define LOG_ITEM_LENGTH_MAX                       (2*1024)             //单个LOG最大长度2K
#define LOG_FILE_CHANGE_NAME_PRE_SECONDS          (60*60*1 )           //一小时换一次名字
#define LOG_FILE_INFO_BUFFER_SIZE                 (256*1024)           //日志目录最大长度
#define LOG_FILE_DEFAULT_HOLD                     (12)                  //保存3天的日志文件
#define LOG_TIME_STRING_MAX                       (128)                //时间戳字符长度
#define FILENAME_STRING_LENGTH 256
#define APP_INFO_OIT_STRING_MAX                   (2*1024)


typedef void (*_APP_INFO_OUT_CALLBACK)(char* szInfo,void* pCallparam);
class CFreeLongLog  
{
public:
	static int MakeATimeString(char* szBuffer,int nBufferSize);  //定制时间戳字符串
public:
	CFreeLongLog(
		    char* szLogPath,
			char* szAppName,
			int nHoldFileMax=LOG_FILE_DEFAULT_HOLD,
			bool bSyslogFlag=true,
			bool bDebugFlag=true,
			bool bDebug2Flag=false,
			bool bDebug3Flag=false,
			bool bPrint2ScrFlag=true
			);

	 ~CFreeLongLog();
public:
	void _XGDebug4Bin(char* pBuffer,int nLength);
	int  _XGSysLog(char* szFormat,...);
	int  _XGDebug(char* szFormat,...);
	int  _XGDebug2(char* szFormat,...);
	int  _XGDebug3(char* szFormat,...);

public:
	int InitFileInfo();
	bool m_bSyslogFlag;
	bool m_bDebugFlag;
	bool m_bDebug2Flag;
	bool m_bDebug3Flag;

private:                                             //内部功能函数
	int _Printf(char* szFormat,...);                 //核心打印函数
	void DeleteFirstFile();                          //删除最老的文件
	void FixFileInfo();                              //修订文件名目录队列
	void MakeFileName();                             //根据时间和文件大小，定制文件名
	void GetFileName();                              //获得当前可用文件名
private:
	CMutexLock    m_Lock;
	char m_szFilePath[FILENAME_STRING_LENGTH];       //文件路径
	char m_szFileName[FILENAME_STRING_LENGTH];       //文件名
	unsigned long m_nFileSize;                       //当前文件大小
	time_t        m_tFileNameMake;                   //定制文件名的时间戳
	int           m_nHoleFileMax;                    //保留文件的数量，构造函数传递进来
	_APP_INFO_OUT_CALLBACK m_pInfoOutCallBack;       //输出回调函数
	void*         m_pInfoOutCallbackParam;
	bool          m_bPrint2ScrFlag;
	char          m_szFileInfoName[FILENAME_STRING_LENGTH];

	std::vector<char*> m_FileNameVec;
	char*         m_chFileName[LOG_FILE_DEFAULT_HOLD];
	int           m_nFileNameIndex;
};

#endif 
