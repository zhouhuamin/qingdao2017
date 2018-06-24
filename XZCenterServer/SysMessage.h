#if !defined _SYS_MESSAGE_H_
#define _SYS_MESSAGE_H_

#define MAX_MSG_BODYLEN  6*1024*1024

#define SYS_NET_MSGHEAD                          "0XJZTECH"
#define SYS_NET_MSGTAIL                           "NJTECHJZ"


#define SYS_SYSTEM                             0X00
#define SYS_SUB_SYSTEM_ICCARDREADER         1000000
#define SYS_SUB_SYSTEM_LEVERCONTROLER      2000000
#define SYS_SUB_SYSTEM_LCDCONTROLER        3000000


#define SYS_MSG_SYSTEM_REGISTER_REQ            SYS_SYSTEM+1
#define SYS_MSG_SYSTEM_REGISTER_ACK           SYS_SYSTEM+2

#define SYS_MSG_SYSTEM_MSG_KEEPLIVE            SYS_SYSTEM+3
#define SYS_MSG_SYSTEM_MSG_KEEPLIVE_ACK        SYS_SYSTEM+4

#define SYS_MSG_SYSTEM_MSG_SEQUENCE_DISPATCH  SYS_SYSTEM+5


#define SYS_MSG_SYSTEM_MSG_UPLOADDATA_TO_CUSTOMS            SYS_SYSTEM+7
#define SYS_MSG_SYSTEM_MSG_UPLOADDATA_TO_CUSTOMS_ACK        SYS_SYSTEM+8


#define SYS_MSG_EVENT_SUBSCRIBE               SYS_SYSTEM+0X11                                        //事件订阅
#define SYS_MSG_EVENT_SUBSCRIBE_ACK           SYS_SYSTEM+0X12                                        //事件订阅返回

#define SYS_MSG_EVENT_EXIT                    SYS_SYSTEM+0X16


#define SYS_MSG_SYSEVENT_PUBLISH                  SYS_SYSTEM+0X20                                        //事件发布
#define SYS_EVENT_TYPE_UPLOAD_CUSTOMS_DATA     SYS_SYSTEM+0X01
#define SYS_EVENT_TYPE_CUSTOMS_DATA_ACK        SYS_SYSTEM+0X02

#define SYS_MSG_ICCARDREADER_READ            SYS_SUB_SYSTEM_ICCARDREADER+3
#define SYS_MSG_ICCARDREADER_READ_ACK       SYS_SUB_SYSTEM_ICCARDREADER+4


#define SYS_MSG_LEVERCONTROLER_ON            SYS_SUB_SYSTEM_ICCARDREADER+5
#define SYS_MSG_LEVERCONTROLER_ON_ACK       SYS_SUB_SYSTEM_ICCARDREADER+6

#define SYS_MSG_LEVERCONTROLER_OFF         SYS_SUB_SYSTEM_ICCARDREADER+7
#define SYS_MSG_LEVERCONTROLER_OFF_ACK    SYS_SUB_SYSTEM_ICCARDREADER+8


#define SYS_MSG_LCD_DISPLAY              SYS_SUB_SYSTEM_ICCARDREADER+9
#define SYS_MSG_LCD_DISPLAY_ACK              SYS_SUB_SYSTEM_ICCARDREADER+10

#define SYS_MSG_REGATHERDATA                 0X13

#define SYS_MSG_DEVICE_CTRL                 0X17
#define CLIENT_EXCEPTIION_FREE				0X18

#define SYS_MSG_SEND_PICDATA              0X0A
#define SYS_MSG_SEND_PICDATA_ACK          0X0B



#define SYS_EVENT_ICREADER_READ_COMPLETE   "1000000000"      //ic读卡器读完成
#define SYS_EVENT_PACKET_DATA                "C000000001"     //打包数据


#define EVENT_SERVICE_TYPE_CLIENT                 0X00
#define EVENT_SERVICE_TYPE_CONTROLER            0X01


#define SYS_MSG_SYSTEM_CTRL_CMD                       SYS_SYSTEM+30



struct NET_PACKET_HEAD {
    int msg_type;
    int packet_len;
    char version_no[16];
    int proxy_count;
    int net_proxy[10];
};

struct NET_PACKET_MSG {
    NET_PACKET_HEAD msg_head;
    char msg_body[MAX_MSG_BODYLEN];
};

struct T_DeviceServerRegister_Req {
    char device_id[32]; //服务模块ID，唯一值
};

struct T_DeviceServerRegister_Ack {
    int reg_result; //注册结果，成功还是失败
    int next_action; //后续动作，是立即开始工作还是等待启动指令
};

struct T_KeepliveAck {
    int nResult;
};

struct T_ICReader_Data {
    char event_id[32];
    char reader_data[64];
};

struct T_Sequence_ControlData {
    char event_id[32];
    char ic_card_data[32];
    int has_ic;
    int xml_len;
    char xml_data[];

};

struct T_Len {
    int length;
};

struct T_Upload_Customs_Data {
    char szChannelNo[32];
    char szSequenceNo[32];
    int is_data_full; //数据是否完整
    int nCustomsDataLen;
    char szCustomsData[];
};



struct NET_PIC_INFO
{
    int  nPicType;                 //区分图片对应的箱子的位置，前后左右等
    int  nPicLen;                  //图片长度
    int  nOffSet;                  //在缓冲区的位置 
};

struct T_Upload_Pic_Data {
    char szSequenceNo[32];
    char szChannelNo[32];
    char szAreaNo[32];
    char szIEFlag[32];
    int  nPicNum;
    NET_PIC_INFO  picInfo[6 + 1];       // 2015-9-19 NET_PIC_INFO  picInfo[6];
    char szPicData[];              //图片数据
    
};


struct NET_EVENT_SUBSCRIBE {
    char client_type;
    char user_name[32];
    char pass_word[32];
    char event_serial[32];

};

struct NET_EVENT_SUBSCRIBE_ACK {
    int result;
};

struct NET_EVENT {
    char main_type[32];
    int sub_type;
};

struct NET_EVENT_UPLOAD_CUSTOMSDATA {
    char main_type[32];
    int sub_type;
    T_Upload_Customs_Data customs_data;
};


struct NET_REGATHERDATA_REQ
{
    int   nRegatherType; //1,手工录入  2，启动前端设备方式
    char  channel_no[32];
    char  device_tag[16];
    T_Upload_Customs_Data customs_data;  //手工方式有效，其他无效
    
};



struct NET_REGATHER//补采结构体
{
    int iArrange_Type; //补采类别：1-手工录入数据方式补采；2-启动前端设备方式补采; 
    char szChnnl_No[32]; //场站号+通道号+进出标志（0为进、1为出）
    char szDeviceTag[16]; // 启动前端设备补采时传递的设备标识号
    T_Upload_Customs_Data customs_data; //手工方式补采所存放的报文；否则为空

};


struct CLIENT_COMMAND_INFO//控制类报文
{
    int iArrange_Type; //处理类别：0-放行报文；1-人工处理;2-设备控制
    char szChnnl_No[32]; // 场站号+通道号+进出标志（0为进、1为出）
    char szSequence_No[32]; // 场站号+通道号+进出标志（0为进、1为出）
    char szDEAL_TYPE[4]; //人工处理类控制类型（可以为空）00：抬杆01：落杆02：启动ＩＣ卡读写	03：启动地磅04：启动电子车牌05: 启动车间器
    char szMEM[256]; //备注说明（备案到数据库中供后期查询）
    int nCustomsDataLen; //报文XML大小
    char szCustomsData[]; //存放XML采集报文和放行报文	
};

struct CLIENT_EXCEPTION_FREE//异常放行控制报文
{
	int iArrange_Type; //处理类别：3-异常放行控制
	char szChnnl_No[32]; // 场站号+通道号+进出标志（I为进、E为出）
	char szSequence_No[32]; // 场站号+通道号+进出标志（0为进、1为出）
	char szDEAL_TYPE[4]; //异常放行标记，值为00
	char szMEM[256]; //备注说明（备案到数据库中供后期查询）
	int nExceptionFreeDataLen; //异常放行报文XML大小
	char szExceptionFreeData[]; //存放异常放行XML控制报文，其中存放control_info	
};

#endif
