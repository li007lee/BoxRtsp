/*
 * listen_and_deal_cmd.h
 *
 *  Created on: 2017年6月5日
 *      Author: root
 */

#ifndef LISTEN_AND_DEAL_CMD_H_
#define LISTEN_AND_DEAL_CMD_H_

#include "event.h"
#include "event2/listener.h"
#include "event2/thread.h"

#include "dev_list.h"

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

typedef struct _tagBOX_CTRL_CMD
{
	HB_CHAR header[8]; //固定为"hBzHbox@"
	HB_S32 cmd_length; //头消息后面的数据长度
	DATA_TYPE_E data_type;
	CMD_TYPE_E cmd_type;//信令类型
	CMD_CODE_E cmd_code;
}BOX_CTRL_CMD_OBJ, *BOX_CTRL_CMD_HANDLE;


typedef struct _tagDEV_INFO
{
	HB_CHAR dev_ip[16]; //固定为"hBzHbox@"
	HB_S32 dev_port; //头消息后面的数据长度
}DEV_INFO_OBJ, *DEV_INFO_HANDLE;

typedef enum cmd_type
{
	CMD_HEAD = 1,
	COMMAND,
	READ_ALL,
}CMD_TYPE;

typedef enum cmd_type_ctx
{
	OPEN_VIDEO = 1,
	SERVER_INFO,
	NONE,
}CMD_TYPE_CTX;

typedef struct _tagCLIENT_INFO
{
	DEV_LIST_HANDLE p_DevNode;	//设备节点
	CLIENT_LIST_HANDLE p_ClientNode;//客户节点
	CMD_TYPE enum_DataType;
	struct event_base *base;	//整体的base反应堆
	struct bufferevent *client_bev; //接收到的客户端连接的事件
}CLIENT_INFO_OBJ, *CLIENT_INFO_HANDLE;

HB_S32 start_listening();
///////////////////////////////////////////////
//	Function: 读取回调函数,用于处理接从客户端接收到的信令，并根据信令能容做相应处理
//
//	@param client_bev: 与客户端信令交互的bufferevent事件。
//	@param arg: 事件触发时传入的参数。
//
//	Retrun: 无
///////////////////////////////////////////////
HB_VOID deal_client_request(struct bufferevent *client_bev, void *arg);
HB_VOID deal_server_info_error_cb(struct bufferevent *client_bev, short events, void *args);

#endif /* LISTEN_AND_DEAL_CMD_H_ */
