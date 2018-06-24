
#if !defined _LELINK_MESSAGE_H_
#define _LELINK_MESSAGE_H_

#define LELINK_MSGHEAD_LEN                         8
#define LELINK_MSGTAIL_LEN                          8

#define MAX_XML_BODYLEN			          20*1024

//server load type
#define LELINK_SERVER_LOADTYPE_USER                 1 //online total users
#define LELINK_SERVER_LOADTYPE_DEVICE               2 //online total devices
#define LELINK_SERVER_LOADTYPE_PREVIEW              3 //total realtime video playing
#define LELINK_SERVER_LOADTYPE_VOD                  4 //total VOD playing
#define LELINK_SERVER_LOADTYPE_RECORD               5 //total recording devices

//server flag
#define LELINK_FLAG_CCS_SERVER        		        0x00 //Center Control Server
#define LELINK_FLAG_UMS_SERVER                      0x01 //User Management (Login) Server
#define LELINK_FLAG_MSS_SERVER                      0x02 //Media Streaming (Device) Server
#define LELINK_FLAG_RS_SERVER                       0x03 //Record Server
#define LELINK_FLAG_VOD_SERVER                      0x04 //VOD Server
#define LELINK_FLAG_RCMS_SERVER                     0x05 //Record Cluster Management Server
#define LELINK_FLAG_ACS_SERVER                      0x06 //Alarm Center (Event) Server

//version of message encryption
#define LELINK_CRYPTO_NOTUSE                   	    '0'
#define LELINK_CRYPTO_VERSION1                 	    '1'
#define LELINK_CRYPTO_VERSION2                 	    '2'

//type of terminal
#define LELINK_TERMINAL_DEVICE                      0x00
#define LELINK_TERMINAL_USER                        0x01

//message header and tail symbol
#define LELINK_NET_MSGHEAD							"00LELINK"
#define LELINK_NET_MSGTAIL							"LILINK11"

//common message
#define LELINK_MSG_TYPE_PUBLICKEY_REQ				0x101
#define LELINK_MSG_TYPE_PUBLICKEY_ACK				0x201

#define LELINK_MSG_TYPE_LOGIN_REQ					0x102
#define LELINK_MSG_TYPE_LOGIN_ACK					0x202

#define LELINK_MSG_TYPE_ONLINE_REQ					0x103
#define LELINK_MSG_TYPE_ONLINE_ACK					0x203

#define LELINK_MSG_TYPE_REALPLAY_REQ				0x104
#define LELINK_MSG_TYPE_REALPLAY_ACK				0x204

#define LELINK_MSG_TYPE_LOGOUT_REQ                  0x105
#define LELINK_MSG_TYPE_LOGOUT_ACK                  0x205

#define LELINK_MSG_TYPE_DIRECT_REALPLAY_REQ         0x106
#define LELINK_MSG_TYPE_DIRECT_REALPLAY_ACK         0x206

#define LELINK_MSG_TYPE_QUERY_NETWORK_REQ           0x107
#define LELINK_MSG_TYPE_QUERY_NETWORK_ACK           0x207

#define LELINK_MSG_TYPE_SET_NETWORK_REQ             0x108
#define LELINK_MSG_TYPE_SET_NETWORK_ACK             0x208

#define LELINK_MSG_TYPE_QUERY_WIFI_REQ              0x109
#define LELINK_MSG_TYPE_QUERY_WIFI_ACK              0x209

#define LELINK_MSG_TYPE_CHECK_WIFI_REQ              0x10A
#define LELINK_MSG_TYPE_CHECK_WIFI_ACK              0x20A

#define LELINK_MSG_TYPE_QUERY_DISPLAY_REQ           0x10B
#define LELINK_MSG_TYPE_QUERY_DISPLAY_ACK           0x20B

#define LELINK_MSG_TYPE_SET_DISPLAY_REQ             0x10C
#define LELINK_MSG_TYPE_SET_DISPLAY_ACK             0x20C

//user message
#define LELINK_MSG_USER_QUERY_DEVICE_REQ            0x141
#define LELINK_MSG_USER_QUERY_DEVICE_ACK            0x241

#define LELINK_MSG_USER_DEVICE_ASSOCIATE_REQ		0x142
#define LELINK_MSG_USER_DEVICE_ASSOCIATE_ACK		0x242

#define LELINK_MSG_USER_DEVICE_DEASSOCIATE_REQ      0x143
#define LELINK_MSG_USER_DEVICE_DEASSOCIATE_ACK      0x243

#define LELINK_MSG_USER_REGISTER_REQ				0x144
#define LELINK_MSG_USER_REGISTER_ACK				0x244

#define LELINK_MSG_USER_ENROL_DEVICE_REQ            0x145
#define LELINK_MSG_USER_ENROL_DEVICE_ACK            0x245

#define LELINK_MSG_USER_PTZ_CTRL_REQ                0x146
#define LELINK_MSG_USER_PTZ_CTRL_ACK                0x246

#define LELINK_MSG_USER_TALK_CTRL_REQ               0x147
#define LELINK_MSG_USER_TALK_CTRL_ACK               0x247

#define LELINK_MSG_USER_EDIT_DEVINFO_REQ            0x148
#define LELINK_MSG_USER_EDIT_DEVINFO_ACK            0x248

#define LELINK_MSG_USER_START_RECORD_REQ            0x149
#define LELINK_MSG_USER_START_RECORD_ACK            0x249

#define LELINK_MSG_USER_STOP_RECORD_REQ             0x14A
#define LELINK_MSG_USER_STOP_RECORD_ACK             0x24A

#define LELINK_MSG_USER_QUERY_RECCLIP_REQ           0x14B
#define LELINK_MSG_USER_QUERY_RECCLIP_ACK           0x24B

#define LELINK_MSG_USER_PLAY_RECCLIP_REQ            0x14C
#define LELINK_MSG_USER_PLAY_RECCLIP_ACK            0x24C

#define LELINK_MSG_USER_PLAY_RECORD_CONNECT_REQ     0x14D
#define LELINK_MSG_USER_PLAY_RECORD_CONNECT_ACK     0x24D

#define LELINK_MSG_USER_QUERY_CLIPID_REQ            0x14E
#define LELINK_MSG_USER_QUERY_CLIPID_ACK            0x24E

#define LELINK_MSG_USER_PLAY_VODCTRL_REQ            0x14F
#define LELINK_MSG_USER_PLAY_VODCTRL_ACK            0x24F

//device message
#define LELINK_MSG_DEVICE_ISASSOCIATE_REQ           0x181
#define LELINK_MSG_DEVICE_ISASSOCIATE_ACK           0x281

#define LELINK_MSG_DEVICE_PTZ_CTRL_REQ              0x182
#define LELINK_MSG_DEVICE_PTZ_CTRL_ACK              0x282

#define LELINK_MSG_DEVICE_TALK_CTRL_REQ             0x183
#define LELINK_MSG_DEVICE_TALK_CTRL_ACK             0x283

#define LELINK_MSG_DEVICE_SIGN_REQ                  0x184
#define LELINK_MSG_DEVICE_SIGN_ACK                  0x284

#define LELINK_MSG_DEVICE_PLAYSTOP_REQ              0x185
#define LELINK_MSG_DEVICE_PLAYSTOP_ACK              0x285

#define LELINK_MSG_DEVICE_ALARM_REQ                 0x186
#define LELINK_MSG_DEVICE_ALARM_ACK                 0x286

//udp p2p message between user and device within LAN
#define LELINK_MSG_DEVICE_P2P_USER_ASSOCIATE_REQ    0x1C1
#define LELINK_MSG_USER_P2P_DEVICE_ASSOCIATE_ACK    0x2C1

#define LELINK_MSG_USER_P2P_DEVICE_PASSWORD_REQ     0x2C1
#define LELINK_MSG_DEVICE_P2P_USER_PASSWORD_ACK     0x1C2

/********************************************************/
//ONLY FOR RESOLVING COMPILATION ISSUES, IT SHOULD NOT BE USED IN ANY FORMAL RELEASE!!!
//deprecated udp p2p messages
#define LELINK_MSG_USER_P2P_GET_DEVICE_PASSWORD_REQ 0x171
#define LELINK_MSG_USER_P2P_GET_DEVICE_PASSWORD_ACK 0x271

#define LELINK_MSG_USER_P2P_DETECT_DEVICE_REQ       0x172
#define LELINK_MSG_USER_P2P_DETECT_DEVICE_ACK       0x272

#define LELINK_MSG_DEVICE_P2P_ASSOCIATE_REQ         0x1B1
#define LELINK_MSG_DEVICE_P2P_ASSOCIATE_ACK         0x2B1
/********************************************************/

//server message
#define LELINK_MSG_SERVER_REGISTER_REQ				0x301
#define LELINK_MSG_SERVER_REGISTER_ACK				0x401
#define MAX_SESSION_IDLEN						    32

struct LelinkMessageHead
{
    char szMsgType[16];
    char szXMLLen[16];
    char chReserved1[16];
    char chReserved2[16];
};

#define LELINK_VIDEO_QVGA_128K                0x00  //320x240
#define LELINK_VIDEO_CIF_160K                 0x01  //352x288
#define LELINK_VIDEO_VGA_1024K                0x02  //640x480
#define LELINK_VIDEO_D1_1536K                 0x03  //702x576
#define LELINK_VIDEO_720P_4M                  0x04  //1280x720
#define LELINK_VIDEO_1080P_8M                 0x05  //1920x1080

#define LELINK_AUDIO_G711                     0x00   //G.711
#define LELINK_AUDIO_G726                     0x01   //G.722
#define LELINK_AUDIO_AMRNB                    0x02   //AMRNB
#define LELINK_AUDIO_AMRWB                    0x03   //AMRWB

#define LELINK_FOURCC_3GPP                    0x00   //3GPP
#define LELINK_FOURCC_HXAV                    0x01   //the proprietary streaming container defined by HICHIP 

#define LELINK_PTZ_CTRL_RESET                 0
#define LELINK_PTZ_CTRL_LEFT                  1
#define LELINK_PTZ_CTRL_RIGHT                 2
#define LELINK_PTZ_CTRL_UP                    3
#define LELINK_PTZ_CTRL_DOWN                  4
#define LELINK_PTZ_CTRL_PRESET                5

#define LELINK_TALK_CTRL_CLOSE                0
#define LELINK_TALK_CTRL_OPEN                 1


/*
struct LelinkMessage
{
        LelinkMessageHead*    pMsgHead;
        char*                 szXMLBody;	//[MAX_XML_BODYLEN]
        char*                 szXMLDigest;	//[MAX_DIGEST_BODYLEN]
};
 */

#endif //_LELINK_MESSAGE_H_
