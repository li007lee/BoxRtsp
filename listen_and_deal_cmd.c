/*
 * listen_and_deal_cmd.c
 *	Function: 用于监听客户端连接，并处理通信信令
 *  Created on: 2017年6月5日
 *      Author: root
 */

#include "my_include.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "cJSON.h"
#include "event2/event.h"

#include "listen_and_deal_cmd.h"
#include "dev_list.h"
#include "deal_dev_msg.h"
#include "sqlite3.h"

#define CMD_MAX_LEN	(512) //RTSP服务器发来命令的最大长度

HB_VOID deal_server_info_error_cb(struct bufferevent *client_bev, short events, void *args)
{
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)args;
	bufferevent_disable(client_bev, EV_READ|EV_WRITE);

	if (events & BEV_EVENT_EOF)//对端关闭
	{
		printf("\n22222####### RTSP CMD channel normal closed!\n");
	}
	else if (events & BEV_EVENT_ERROR)//错误事件
	{
		perror("22222Error from bufferevent");
	}
	else if (events & BEV_EVENT_TIMEOUT)//超时事件
	{
		printf("\n22222######RTSP CMD channel  timeout !\n");
	}
	//p_CommunicateTags->client_bev = NULL;
	if ((p_CommunicateTags->p_DevNode->st_ClientListHead.i_ClientNum<1)\
			&&(p_CommunicateTags->p_DevNode->st_ClientListHead.start_thread_flag == 0))
	{
		pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
		remove_one_from_dev_list(p_CommunicateTags->p_DevNode);
		pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
		//free(p_CommunicateTags->p_DevNode);
	}
	bufferevent_free(client_bev);
	free(p_CommunicateTags);
	p_CommunicateTags = NULL;

	return;
}

/**************************处理Open_video消息***************************/
/**************************处理Open_video消息***************************/
/**************************处理Open_video消息***************************/
///////////////////////////////////////////////
//	Function: 异常事件回调函数
//
//	@param: 无
//
//	Retrun: 无
///////////////////////////////////////////////
static HB_VOID deal_open_video_error_cb(struct bufferevent *bev, short events, void *args)
{
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)args;

	if (events & BEV_EVENT_EOF)//对端关闭
	{
		printf("\n####### RTSP CMD channel normal closed!\n");
	}
	else if (events & BEV_EVENT_ERROR)//错误事件
	{
		perror("Error from bufferevent");
	}
	else if (events & BEV_EVENT_TIMEOUT)//超时事件
	{
		printf("\n######RTSP CMD channel  timeout !\n");
	}

	free(p_CommunicateTags);
	p_CommunicateTags = NULL;
	bufferevent_free(bev);

	return;
}

static int analysis_json_dev_info(char *p_SrcJson, char *p_DevId, int *p_DevChnl, int *p_DevStreamType)
{
	cJSON *p_json = cJSON_Parse(p_SrcJson);
	if(NULL == p_json)
	{
		return -1;
	}
	cJSON *p_sub = cJSON_GetObjectItem(p_json, "CmdType");
	if(NULL == p_sub)
	{
		cJSON_Delete(p_json);
		return -1;
	}
	if(!strcmp(p_sub->valuestring, "open_video"))
	{
		p_sub = cJSON_GetObjectItem(p_json, "DevId");
		strcpy(p_DevId, p_sub->valuestring);
		p_sub = cJSON_GetObjectItem(p_json, "DevChnl");
		*p_DevChnl = p_sub->valueint;
		p_sub = cJSON_GetObjectItem(p_json, "DevStreamType");
		*p_DevStreamType = p_sub->valueint;
		cJSON_Delete(p_json);
	}
	else
	{
		cJSON_Delete(p_json);
		return -1;
	}
	return 0;
}



/****************************************数据库操作****************************************/
/****************************************数据库操作****************************************/
/****************************************数据库操作****************************************/

//获取数据条数回调函数
static HB_S32 LoadDeviceInfoCB( HB_VOID * para, HB_S32 n_column, HB_CHAR ** column_value, HB_CHAR ** column_name )
{
//	HB_CHAR *dev_num_buf = (HB_CHAR *)para;
	DEV_INFO_HANDLE dev_info = (DEV_INFO_HANDLE)para;

	strncpy(dev_info->dev_ip, column_value[0], strlen(column_value[0]));
	dev_info->dev_port = atoi(column_value[1]);

	return 0;
}


/***********************************
 * Function:数据库操作接口
 * args:
 * 		@sql: 需要执行的sql语句
 * 		@db_path_name: 需要操作的数据库
 * 		@callback:执行sql时的回调函数
 * 		@arg:回调函数的参数
 * return：成功返回0,失败返回负数
 ***********************************/
HB_S32 SqlOperation(HB_CHAR *sql, HB_CHAR *db_path_name, HB_S32 (*callback)(HB_VOID *, HB_S32, HB_CHAR **,HB_CHAR **), HB_VOID *arg)
{
	sqlite3 *db;
	HB_CHAR *errmsg = NULL;
	HB_S32 ret = 0;
	HB_S32 errcode = 0;

	//打开数据库
	ret = sqlite3_open(db_path_name, &db);
	if (ret != SQLITE_OK) {
		printf("open db [%s] failed(%d)!\n", db_path_name, ret);
		errcode = -1;
		goto Err;
	}

	ret = sqlite3_exec(db, sql, callback, arg, &errmsg);
	if (ret != SQLITE_OK) {
		printf("exec sql failed:[%s]\nerr message(%d):[%s]\n", sql, ret, errmsg);
		//设备id重复
		if(!strcmp(errmsg, "UNIQUE constraint failed: dev_add_web_data.dev_id"))
		{
			errcode = -2;
			goto Err;
		}
		errcode = -3;
		goto Err;
	}
Err:
	sqlite3_free(errmsg);
	sqlite3_close(db);

	return errcode;
}
/****************************************数据库操作END****************************************/
/****************************************数据库操作END****************************************/
/****************************************数据库操作END****************************************/





static HB_S32 deal_open_video_cmd(HB_CHAR *p_CmdBuf, CLIENT_INFO_HANDLE p_CommunicateTags)
{
	HB_CHAR acc_DevId[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_S32 i_DevChnl = -1;		//设备通道号
	HB_S32 i_DevStreamType = -1;//主子码流
	DEV_LIST_HANDLE p_DevNode = NULL;

	analysis_json_dev_info(p_CmdBuf, acc_DevId, &i_DevChnl, &i_DevStreamType);
	printf("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", acc_DevId, i_DevChnl, i_DevStreamType);

	pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
	p_DevNode = find_in_dev_list(acc_DevId, i_DevChnl, i_DevStreamType);
	//未找到该设备，创建该设备并加入到设备链表。
	if (p_DevNode == NULL)
	{
		DEV_INFO_OBJ dev_info;
		HB_CHAR sql[512] = {0};
		memset(&dev_info, 0, sizeof(DEV_INFO_OBJ));
		snprintf(sql, sizeof(sql), "select dev_ip,dev_port from device_info where dev_id='%s'", acc_DevId);
		printf("sql:[%s]\n", sql);
		if (SqlOperation(sql, DEV_DATA_BASE_NAME, LoadDeviceInfoCB, (void *)&dev_info) < 0)
		{
			//数据库操作失败
			bufferevent_free(p_CommunicateTags->client_bev);
			p_CommunicateTags->client_bev = NULL;
			free(p_CommunicateTags);
			p_CommunicateTags = NULL;
			pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
			printf("data_base[%s] opt failed!\n", DEV_DATA_BASE_NAME);
			return HB_FAILURE;
		}
		printf("dev_ip:[%s], dev_port:[%d]\n", dev_info.dev_ip, dev_info.dev_port);
		if (dev_info.dev_ip == NULL)
		{
			//测设备不存在
			bufferevent_free(p_CommunicateTags->client_bev);
			p_CommunicateTags->client_bev = NULL;
			free(p_CommunicateTags);
			p_CommunicateTags = NULL;
			pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
			return HB_FAILURE;
		}
		p_DevNode = create_new_dev_node(acc_DevId, i_DevChnl, i_DevStreamType);
		strncpy(p_DevNode->arr_DevIp, dev_info.dev_ip, sizeof(p_DevNode->arr_DevIp));
		p_DevNode->i_DevRtspPort = dev_info.dev_port;
		p_DevNode->connecet_status = CONNECTING;

//		snprintf(p_DevNode->arr_DevRtspUrl, sizeof(p_DevNode->arr_DevRtspUrl), \
//				"rtsp://admin:a1234567@%s:%d/h264/ch1/sub/av_stream", \
//				p_DevNode->arr_DevIp, p_DevNode->i_DevRtspPort);
//		printf("rtsp url:[%s]\n", p_DevNode->arr_DevRtspUrl);
//			strncpy(p_DevNode->arr_DevRtspUrl, "rtsp://admin:a1234567@%s:9036/h264/ch1/sub/av_stream", sizeof(p_DevNode->arr_DevRtspUrl));
//			strncpy(p_DevNode->arr_DevIp, "192.168.8.21", sizeof(p_DevNode->arr_DevIp));
//			p_DevNode->i_DevRtspPort = 8554;
//			strncpy(p_DevNode->arr_DevIp, "10.7.126.242", sizeof(p_DevNode->arr_DevIp));
		//海康
//			strncpy(p_DevNode->arr_DevIp, "10.6.209.93", sizeof(p_DevNode->arr_DevIp));
//			p_DevNode->i_DevRtspPort = 9020;
		p_CommunicateTags->p_DevNode = p_DevNode;
		memset(&(p_CommunicateTags->p_DevNode->st_ClientListHead), 0, sizeof(CLIENT_LIST_HEAD_OBJ));
		add_node_to_dev_list(p_DevNode);
		pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
		test_dev_connection(p_CommunicateTags);
//		strncpy(p_DevNode->arr_DevRtspUrl, "rtsp://admin:a1234567@10.6.209.93:9036/h264/ch1/sub/av_stream", sizeof(p_DevNode->arr_DevRtspUrl));
//		strncpy(p_DevNode->arr_DevRtspUrl, "rtsp://admin:a1234567@10.6.209.93:9036/h264/ch1/main/av_stream", sizeof(p_DevNode->arr_DevRtspUrl));

//		read_dev_sdp_cb(NULL, p_CommunicateTags);
	}
	else
	{
		if (p_DevNode->st_ClientListHead.i_ClientNum == 0)
		{
			//如果此设备还未开始传输视频流
			bufferevent_free(p_CommunicateTags->client_bev);
			p_CommunicateTags->client_bev = NULL;
			free(p_CommunicateTags);
			p_CommunicateTags = NULL;
			pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
			return -1;
		}

//		while (p_DevNode->connecet_status != CONNECTED)
//		{
//			usleep(500*1000);
//			if (p_DevNode->connecet_status != UN_CONNECTED)
//			{
//				//如果此设备还未开始传输视频流
//				bufferevent_free(p_CommunicateTags->client_bev);
//				p_CommunicateTags->client_bev = NULL;
//				free(p_CommunicateTags);
//				p_CommunicateTags = NULL;
//				pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
//				return -1;
//			}
//		}
		pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
		TRACE_DBG("Already have this device, add rtsp client node!\n");
		struct bufferevent *p_AcceptClient_bev = p_CommunicateTags->client_bev;
		BOX_CTRL_CMD_OBJ st_MsgHead;
		HB_CHAR arr_SendBuf[2048] = {0};

		memset(&st_MsgHead, 0, sizeof(st_MsgHead));
		memcpy(st_MsgHead.header, "hBzHbox@", 8);
		st_MsgHead.cmd_code = CMD_OK;

		memcpy(arr_SendBuf, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ));

		snprintf(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ), 2048-sizeof(BOX_CTRL_CMD_OBJ), \
				"{\"CmdType\":\"sdp_info\",\"m_video\":\"%s\",\"a_rtpmap_video\":\"%s\",\"a_fmtp_video\":\"%s\",\"m_audio\":\"%s\",\"a_rtpmap_audio\":\"%s\"}", \
				p_DevNode->m_video, p_DevNode->a_rtpmap_video, p_DevNode->a_fmtp_video, p_DevNode->m_audio, p_DevNode->a_rtpmap_audio);

		st_MsgHead.cmd_length = strlen(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ));

		printf("arr_SendBuf2:[%s], body_len:[%d]\n", arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ), st_MsgHead.cmd_length);

		p_CommunicateTags->p_DevNode = p_DevNode;
		p_CommunicateTags->enum_DataType = CMD_HEAD;
		bufferevent_write(p_AcceptClient_bev, arr_SendBuf, st_MsgHead.cmd_length+sizeof(BOX_CTRL_CMD_OBJ));
		//bufferevent_setcb(p_AcceptClient_bev, cmd_task_read_cb, NULL, NULL, (HB_VOID *)p_CommunicateTags);
		bufferevent_setcb(p_AcceptClient_bev, deal_client_request, NULL, deal_server_info_error_cb, (void*)p_CommunicateTags);
		bufferevent_enable(p_AcceptClient_bev, EV_READ);
		printf("connect rtsp connect rtsp connect rtsp connect rtsp connect rtsp connect rtsp\n");
	}

	return 0;
}
/***************************处理Open_video消息End****************************/
/***************************处理Open_video消息End****************************/
/***************************处理Open_video消息End****************************/


/**************************处理Server_info消息***************************/
/**************************处理Server_info消息***************************/
/**************************处理Server_info消息***************************/


static HB_VOID error_close_cb(struct bufferevent *connect_rtsp_server_bev, HB_S16 what, HB_VOID *arg)
{
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)arg;
	CLIENT_LIST_HANDLE p_ClientNode = p_CommunicateTags->p_ClientNode;
	CLIENT_LIST_HEAD_HANDLE p_ClientHead = &(p_CommunicateTags->p_DevNode->st_ClientListHead);

	if (what & BEV_EVENT_ERROR)
	{
		//printf("disable client node addr [%p]!\n", p_ClientNode);
		printf("disable bev [%p]--->[%p]!\n", connect_rtsp_server_bev, p_ClientNode->p_SendVideoToServerEvent);
		if (p_ClientHead->start_thread_flag == 1) //如果发送线程已经启动，此处只置位
		{
//			pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
//			bufferevent_disable(p_ClientNode->p_SendVideoToServerEvent, EV_READ|EV_WRITE);
			p_ClientNode->del_flag = 1;
//			pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
		}
		else //若没有启动，从此处释放
		{
			printf("free form call back0 !!!!!!!!!!!!!!!!!!!!!!!!!!!");
		}
	}
	else
	{
		printf("1111disable bev [%p]--->[%p]!\n", connect_rtsp_server_bev, p_ClientNode->p_SendVideoToServerEvent);
		if (p_ClientHead->start_thread_flag == 1) //如果发送线程已经启动，此处只置位
		{
//			pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
//			bufferevent_disable(p_ClientNode->p_SendVideoToServerEvent, EV_READ|EV_WRITE);
			p_ClientNode->del_flag = 1;
//			pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
		}
		else //若没有启动，从此处释放
		{
			printf("free form call back1 !!!!!!!!!!!!!!!!!!!!!!!!!!!");
		}
	}
#if 0
	if (what & BEV_EVENT_TIMEOUT)//盒子connect设备超时
	{
		printf("\n###########  connect dev get SDP  timeout !\n");
	}
	else if (what & BEV_EVENT_ERROR)  //盒子connect 设备失败
	{
		printf("\n###########  box connect dev  failed !\n");
	}
	else if (what & BEV_EVENT_EOF) //设备主动发送close给盒子
	{
		printf("\n###########  dev send close to box !\n");
	}
#endif
}


static HB_VOID active_connect_to_rtsp_server_eventcb(struct bufferevent *connect_rtsp_server_bev, HB_S16 what, HB_VOID *arg)
{
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)arg;
	CLIENT_LIST_HEAD_HANDLE p_ClientHead = &(p_CommunicateTags->p_DevNode->st_ClientListHead);

	if (what & BEV_EVENT_CONNECTED)//盒子主动连接rtsp服务器成功
	{
		//连接成功创建客户节点
		pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
		CLIENT_LIST_HANDLE p_NewClientNode = NULL;
		p_NewClientNode = new_client_node();
		p_NewClientNode->p_SendVideoToServerEvent = connect_rtsp_server_bev;
		p_CommunicateTags->p_ClientNode = p_NewClientNode;
		p_NewClientNode->del_flag = 0;

		//插入链表
		add_client_in_tail(p_ClientHead, p_NewClientNode);
		printf("add client add client add client!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		//启动读取视频流线程
		if (p_ClientHead->start_thread_flag == 0)
		{
			pthread_create(&(p_ClientHead->thread_ReadVideo_id), NULL, read_video_data_from_dev_task, (HB_VOID*)p_CommunicateTags);
		}
		printf("create bev[%p]\n", connect_rtsp_server_bev);
		bufferevent_setcb(connect_rtsp_server_bev, NULL, NULL, error_close_cb, (HB_VOID *)p_CommunicateTags);
//		bufferevent_enable(connect_rtsp_server_bev, EV_READ|EV_WRITE);
		bufferevent_enable(connect_rtsp_server_bev, EV_WRITE);
		pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
	}
	else if (what & BEV_EVENT_ERROR)  //盒子connect 设备失败
	{
		printf("\n###########  connect rtsp server failed !\n");
		p_CommunicateTags->p_ClientNode = NULL;

		bufferevent_free(connect_rtsp_server_bev);
	}
#if 0
	else if (what & BEV_EVENT_TIMEOUT)//盒子connect设备超时
	{
	}
	else if (what & BEV_EVENT_ERROR)  //盒子connect 设备失败
	{
	}
	else if (what & BEV_EVENT_EOF) //设备主动发送close给盒子
	{
	}
#endif

}


static int analysis_json_server_info(char *p_SrcJson, char *p_ServerIp, int *p_ServerPort)
{
	cJSON *p_json = cJSON_Parse(p_SrcJson);
	if(NULL == p_json)
	{
		return -1;
	}
	cJSON *p_sub = cJSON_GetObjectItem(p_json, "CmdType");
	if(NULL == p_sub)
	{
		cJSON_Delete(p_json);
		return -1;
	}
	if(!strcmp(p_sub->valuestring, "server_info"))
	{
		p_sub = cJSON_GetObjectItem(p_json, "ServerIp");
		strcpy(p_ServerIp, p_sub->valuestring);
		p_sub = cJSON_GetObjectItem(p_json, "ServerPort");
		*p_ServerPort = p_sub->valueint;
		cJSON_Delete(p_json);
	}
	else
	{
		cJSON_Delete(p_json);
		return -1;
	}
	return 0;
}


static HB_S32 start_send_video(HB_CHAR *p_CmdBuf, CLIENT_INFO_HANDLE p_CommunicateTags)
{
	HB_CHAR server_ip[16] = {0};
	HB_S32 server_port = 0;

	analysis_json_server_info(p_CmdBuf, server_ip, &server_port);
	printf("rtsp_server_ip=[%s]\nrtsp_server_port=[%d]\n", \
					server_ip, server_port);

	HB_S32 connect_to_addrlen;
	struct sockaddr_in connect_to_addr;
	struct bufferevent *p_SendVideoToServerEvent;//主动连接服务器事件

	p_SendVideoToServerEvent = bufferevent_socket_new(p_CommunicateTags->base, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);
	bzero(&connect_to_addr, sizeof(connect_to_addr));
	connect_to_addr.sin_family = AF_INET;
	connect_to_addr.sin_port = htons(server_port);
	inet_pton(AF_INET, server_ip, &connect_to_addr.sin_addr);
	connect_to_addrlen = sizeof(struct sockaddr_in);

	bufferevent_setcb(p_SendVideoToServerEvent, NULL, NULL, active_connect_to_rtsp_server_eventcb, (HB_VOID *)p_CommunicateTags);

	if (bufferevent_socket_connect(p_SendVideoToServerEvent, (struct sockaddr*)&connect_to_addr, connect_to_addrlen) < 0)//非阻塞连接设备端
	{
		bufferevent_free(p_SendVideoToServerEvent);
		return HB_FAILURE;
	}

	return HB_SUCCESS;
}
/***************************处理Server_info消息End****************************/
/***************************处理Server_info消息End****************************/
/***************************处理Server_info消息End****************************/


///////////////////////////////////////////////
//	Function: 读取回调函数,用于处理接从客户端接收到的信令，并根据信令能容做相应处理
//
//	@param client_bev: 与客户端信令交互的bufferevent事件。
//	@param arg: 事件触发时传入的参数。
//
//	Retrun: 无
///////////////////////////////////////////////
HB_VOID deal_client_request(struct bufferevent *client_bev, void *arg)
{
	struct timeval tv_w;
	BOX_CTRL_CMD_OBJ st_MsgHead;
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)arg;

	if (p_CommunicateTags == NULL)
	{
		printf("p_CommunicateTags is NULL!\n");
		return;
	}

	//bufferevent_disable(client_bev, EV_READ);
	CMD_TYPE enum_DataType = p_CommunicateTags->enum_DataType;
	HB_CHAR arrc_RecvCmdBuf[CMD_MAX_LEN] = {0};

	struct evbuffer *src = bufferevent_get_input(client_bev);//获取输入缓冲区
	HB_S32 len = evbuffer_get_length(src);//获取输入缓冲区中数据的长度，也就是可以读取的长度。

	//读取缓冲区数据长度
	if (len > sizeof(BOX_CTRL_CMD_OBJ))
	{
		//如果命令头和命令体是一条命令发过来的则统一处理
		if (p_CommunicateTags->enum_DataType != COMMAND)
		{
			enum_DataType = READ_ALL;
		}
	}

	switch(enum_DataType)
	{
		case CMD_HEAD: //头信息
		{
			memset(&st_MsgHead, 0, sizeof(BOX_CTRL_CMD_OBJ));
			if(evbuffer_remove(src, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ)))
			{
				printf("recv_header = [%s]\n", st_MsgHead.header);
				if (strncmp(st_MsgHead.header, "hBzHbox@", 8))
				{
					//头消息错误，直接关闭返回
					bufferevent_free(client_bev);
					return ;
				}
				p_CommunicateTags->enum_DataType = COMMAND;
			}
			break;
		}
		case COMMAND: //命令信息
		{
			//读取命令信息
			if (evbuffer_remove(src, arrc_RecvCmdBuf, CMD_MAX_LEN) > 0)
			{
				if (strstr(arrc_RecvCmdBuf, "open_video") != NULL)
				{
					printf("recv recv recv recv open_video:[%s]\n", arrc_RecvCmdBuf);
					p_CommunicateTags->client_bev = client_bev;
					//收到rtsp打开请求
					deal_open_video_cmd(arrc_RecvCmdBuf, p_CommunicateTags);
				}
				else if (strstr(arrc_RecvCmdBuf, "server_info") != NULL)
				{
					printf("recv recv recv recv server_info:[%s]\n", arrc_RecvCmdBuf);
					//拿到了rtsp服务器的ip和端口，需要将视频流发到这里
					//创建线程并发送数据
					bufferevent_free(client_bev);
					p_CommunicateTags->client_bev = NULL;
					start_send_video(arrc_RecvCmdBuf, p_CommunicateTags);
				}
			}
			break;
		}
		case READ_ALL://头信息和命令信息一起发过来的
		{
			memset(&st_MsgHead, 0, sizeof(BOX_CTRL_CMD_OBJ));
			if(evbuffer_remove(src, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ)))
			{
				printf("recv_header = [%s]\n", st_MsgHead.header);
				if (strncmp(st_MsgHead.header, "hBzHbox@", 8))
				{
					//头消息错误，直接关闭返回
					bufferevent_free(client_bev);
					return ;
				}
				if (evbuffer_remove(src, arrc_RecvCmdBuf, CMD_MAX_LEN) > 0)
				{
					if (strstr(arrc_RecvCmdBuf, "open_video") != NULL)
					{
						printf("recv recv recv recv open_video:[%s]\n", arrc_RecvCmdBuf);
						p_CommunicateTags->client_bev = client_bev;
						//收到rtsp打开请求
						deal_open_video_cmd(arrc_RecvCmdBuf, p_CommunicateTags);
					}
					else if (strstr(arrc_RecvCmdBuf, "server_info") != NULL)
					{
						printf("recv recv recv recv server_info:[%s]\n", arrc_RecvCmdBuf);
						//拿到了rtsp服务器的ip和端口，需要将视频流发到这里
						//创建线程并发送数据
						bufferevent_free(client_bev);
						p_CommunicateTags->client_bev = NULL;
						start_send_video(arrc_RecvCmdBuf, p_CommunicateTags);
					}
				}
			}
			break;
		}
		default:
			break;
	}
}


///////////////////////////////////////////////
//	Function: 接收到客户端连接的回调函数
//
//	@param: 无
//
//	Retrun: 无
///////////////////////////////////////////////
static void accept_client_connect_cb(struct evconnlistener *p_Listener, evutil_socket_t i_AcceptSockfd,
	    struct sockaddr *p_ClientAddr, int slen, void *arg)
{
	struct timeval tv_w;
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)malloc(sizeof(CLIENT_INFO_OBJ));
	memset(p_CommunicateTags, 0, sizeof(CLIENT_INFO_OBJ));

	//p_CommunicateTags->p_DevNode = NULL;
	p_CommunicateTags->enum_DataType = CMD_HEAD;
	//p_CommunicateTags->enum_DataTypeCtx = OPEN_VIDEO;//第一次连接过来接收open_video命令
	p_CommunicateTags->base = evconnlistener_get_base(p_Listener);

#if 1
	//打印对端ip
	struct sockaddr_in *p_PeerAddr = (struct sockaddr_in *)p_ClientAddr;
	char arrch_PeerIp[16] = {0};
	inet_ntop(AF_INET, &(p_PeerAddr->sin_addr), arrch_PeerIp, sizeof(arrch_PeerIp));
	printf("\nAccept a new connect ! sockfd=%d accept ip=%s : %d\n", \
			i_AcceptSockfd,arrch_PeerIp, ntohs(p_PeerAddr->sin_port));
#endif

    // 为新的连接分配并设置 bufferevent,设置BEV_OPT_CLOSE_ON_FREE宏后，当连接断开时也会通知客户端关闭套接字
    struct bufferevent *accept_sockfd_bev = bufferevent_socket_new(p_CommunicateTags->base, i_AcceptSockfd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
    //printf("accept_sockfd_bev addr [%p]\n", accept_sockfd_bev);
    //设置超时，5秒内未收到对端发来数据则断开连接
	tv_w.tv_sec  = 5;
	tv_w.tv_usec = 0;
	bufferevent_set_timeouts(accept_sockfd_bev, &tv_w, NULL);
    bufferevent_setcb(accept_sockfd_bev, deal_client_request, NULL, deal_open_video_error_cb, (void*)p_CommunicateTags);
    bufferevent_enable(accept_sockfd_bev, EV_READ);

    return;
}


///////////////////////////////////////////////
//	Function: 启动服务程序并监听客户端连接
//
//	@param: 无
//
//	Retrun: 无
///////////////////////////////////////////////
HB_S32 start_listening()
{
	struct event_base *p_EventBase;
	struct evconnlistener *p_Listener;
	struct sockaddr_in  st_ListenAddr;

	if(evthread_use_pthreads() != 0)
	{
		TRACE_ERR("########## evthread_use_pthreads() err !");
		return HB_FAILURE;
	}

	bzero(&st_ListenAddr, sizeof(st_ListenAddr));
	st_ListenAddr.sin_family = AF_INET;
	st_ListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	st_ListenAddr.sin_port = htons(BOX_SERVER_PORT);

	p_EventBase = event_base_new();
	if (!p_EventBase)
	{
		perror("cmd_base event_base_new()");
		return 1;
	}
	//用于线程安全
	if(evthread_make_base_notifiable(p_EventBase) != 0)
	{
		TRACE_ERR("###### evthread_make_base_notifiable() err!");
		return HB_FAILURE;
	}

	//绑定端口并接收连接
	p_Listener = evconnlistener_new_bind(p_EventBase, accept_client_connect_cb, NULL,
			LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE|LEV_OPT_THREADSAFE,
			1024, (struct sockaddr*)&st_ListenAddr, sizeof(struct sockaddr_in));
	//printf("total_event: [%d]\n", p_EventBase->event_count);
	event_base_dispatch(p_EventBase);
	evconnlistener_free(p_Listener);
	event_base_free(p_EventBase);

	return 0;
}
