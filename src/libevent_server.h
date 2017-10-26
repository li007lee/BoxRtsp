/*
 * libevent_server.h
 *
 *  Created on: 2017年9月21日
 *      Author: lijian
 */

#ifndef SRC_LIBEVENT_SERVER_H_
#define SRC_LIBEVENT_SERVER_H_

#include "my_include.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "cJSON.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/listener.h"
#include "event2/thread.h"

#include "dev_list.h"

#define CMD_MAX_LEN	(512) //RTSP服务器发来命令的最大长度

extern struct event_base *pEventBase;

typedef enum _tagDATA_TYPE
{
    I_FRAME=1,   //I帧
	BP_FRAME,    //BP帧
}DATA_TYPE_E;

typedef enum _tagCMD_TYPE
{
    BOX_PLAY_VIDEO=1,   //播放视频
	BOX_DEV_TEST,        //设备在线查询
	BOX_VIDEO_DATA,      //视频数据
	BOX_AUDIO_DATA,      //音频数据
}CMD_TYPE_E;

typedef enum _tagCMD_CODE
{
    CMD_OK=1,   //请求成功
	CMD_FAILED,//请求失败
}CMD_CODE_E;


//信令交互协议头
typedef struct _tagBOX_CTRL_CMD
{
	HB_CHAR header[8]; //固定为"hBzHbox@"
	HB_S32 cmd_length; //头消息后面的数据长度
	DATA_TYPE_E data_type;
	CMD_TYPE_E cmd_type;//信令类型
	CMD_CODE_E cmd_code;
	HB_S64	pts;
//	HB_U32 uiVideoSec;	//视频当前秒数
//	HB_U32 uiVideoUsec;	//视频当前微妙数
}BOX_CTRL_CMD_OBJ, *BOX_CTRL_CMD_HANDLE;


//libevent 通信参数结构体
typedef struct _LIBEVENT_ARGS_DEV
{
	DEV_LIST_HANDLE pDevNode; //当前设备节点
	CLIENT_LIST_HANDLE pClientNode;	//当前客户节点
//	struct event_base *pEventBase; //libevent 反应堆
//	struct bufferevent *pClientBev;//与客户端信令交互的事件句柄
}LIBEVENT_ARGS_DEV_OBJ, *LIBEVENT_ARGS_DEV_HANDLE;


HB_VOID libevent_server_main_listen();
HB_VOID deal_client_cmd(struct bufferevent *pClientBev, void *arg);

#endif /* SRC_LIBEVENT_SERVER_H_ */
