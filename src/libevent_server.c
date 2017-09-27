/*
 * libevent_server.c
 *
 *  Created on: 2017年9月21日
 *      Author: lijian
 */

#include "libevent_server.h"
#include "sqlite3.h"
#include "dev_opt.h"
#include "client_list.h"


/****************************************数据库操作****************************************/
/****************************************数据库操作****************************************/
/****************************************数据库操作****************************************/

//获取数据条数回调函数
static HB_S32 load_device_ip_port_cb( HB_VOID * para, HB_S32 n_column, HB_CHAR ** column_value, HB_CHAR ** column_name )
{
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)para;

	strncpy(pDevNode->arrDevIp, column_value[0], strlen(column_value[0]));
	pDevNode->iDevRtspPort = atoi(column_value[1]);

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
HB_S32 sql_operation(HB_CHAR *sql, HB_CHAR *db_path_name, HB_S32 (*callback)(HB_VOID *, HB_S32, HB_CHAR **,HB_CHAR **), HB_VOID *arg)
{
	sqlite3 *db;
	HB_CHAR *errmsg = NULL;
	HB_S32 ret = 0;
	HB_S32 errcode = 0;

	//打开数据库
	ret = sqlite3_open(db_path_name, &db);
	if (ret != SQLITE_OK) {
		TRACE_ERR("open db [%s] failed(%d)!\n", db_path_name, ret);
		errcode = -1;
		goto Err;
	}

	ret = sqlite3_exec(db, sql, callback, arg, &errmsg);
	if (ret != SQLITE_OK)
	{
		TRACE_ERR("exec sql failed:[%s]\nerr message(%d):[%s]\n", sql, ret, errmsg);
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




/**************************处理Server_info消息***************************/
/**************************处理Server_info消息***************************/
/**************************处理Server_info消息***************************/
static HB_VOID send_rtsp_to_server_event_error_cb(struct bufferevent *connect_rtsp_server_bev, HB_S16 what, HB_VOID *arg)
{
	LIBEVENT_ARGS_HANDLE pMessengerArgs = (LIBEVENT_ARGS_HANDLE)arg;
	CLIENT_LIST_HANDLE pClientNode = pMessengerArgs->pClientNode;
	CLIENT_LIST_HEAD_HANDLE pRtspClientHead = &(pMessengerArgs->pDevNode->stRtspClientHead);

	if (pRtspClientHead->iStartThreadFlag == 1) //如果发送线程已经启动，此处只置位
	{
		bufferevent_disable(pClientNode->pSendVideoToServerEvent, EV_READ|EV_WRITE);
		pClientNode->iDelFlag = 1;
	}
	else //若没有启动，从此处释放
	{
		pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
		bufferevent_disable(pClientNode->pSendVideoToServerEvent, EV_READ|EV_WRITE);
		if (pMessengerArgs != NULL)
		{
			if (pClientNode != NULL)
			{
				printf("del one dev from dev_list!\n");
				pthread_mutex_lock(&(pRtspClientHead->mutexClientListMutex));
				del_one_client(pRtspClientHead, pClientNode);
				pMessengerArgs->pClientNode = NULL;
				pthread_mutex_unlock(&(pRtspClientHead->mutexClientListMutex));
			}

			if ((pMessengerArgs->pDevNode != NULL) && (pMessengerArgs->pDevNode->stRtspClientHead.iClientNum < 1))
			{
				//当前设备下如果已经没有用户了，那么需要把设备从设备链表摘除
				printf("del one dev from dev_list!\n");
				del_one_from_dev_list(pMessengerArgs->pDevNode);
				pMessengerArgs->pDevNode = NULL;
			}
			printf("send_rtsp_to_server_event_error_cb free pMessengerArgs\n");
			free(pMessengerArgs);
			pMessengerArgs = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
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


/*
 *	Function: 向rtsp服务器发起连接的事件处理回调函数
 *
 *	@param pConnectRtspServerBev: [IN]连接rtsp服务器的事件句柄
 *  @parmm what: [IN]事件触发状态的类型
 *  @parmm arg： [IN]libevent 参数结构体，类型为LIBEVENT_ARGS_HANDLE
 *
 *	Retrun: 无
 */
static HB_VOID connect_to_rtsp_server_event_cb(struct bufferevent *pConnectRtspServerBev, HB_S16 what, HB_VOID *arg)
{
	LIBEVENT_ARGS_HANDLE pMessengerArgs = (LIBEVENT_ARGS_HANDLE)arg;
	DEV_LIST_HANDLE pDevNode = pMessengerArgs->pDevNode;
	CLIENT_LIST_HEAD_HANDLE pRtspClientHead = &(pDevNode->stRtspClientHead);

	if (what & BEV_EVENT_CONNECTED)//盒子主动连接rtsp服务器成功
	{
		TRACE_GREEN("连接rtsp服务器成功！！！！！！！！！！！");
		//连接成功创建客户节点
		CLIENT_LIST_HANDLE pClientNode = (CLIENT_LIST_HANDLE)malloc(sizeof(CLIENT_LIST_OBJ));
		pClientNode->pSendVideoToServerEvent = pConnectRtspServerBev;
		pMessengerArgs->pClientNode = pClientNode;

		pthread_mutex_lock(&(pRtspClientHead->mutexClientListMutex));
//		pMessengerArgs->del_flag = 0;

		//插入链表
		add_client_in_tail(pRtspClientHead, pClientNode);
		TRACE_GREEN("add client add client add client!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		//启动读取视频流线程
		if (pRtspClientHead->iStartThreadFlag == 0)
		{
			//如果当前设备还没有启动推流线程，则启动
			pthread_create(&(pRtspClientHead->threadReadVideoId), NULL, read_video_data_from_dev_task, (HB_VOID*)pMessengerArgs);
		}
		//设置连接出错后的回调函数
		bufferevent_setcb(pConnectRtspServerBev, NULL, NULL, send_rtsp_to_server_event_error_cb, (HB_VOID *)pMessengerArgs);
		bufferevent_enable(pConnectRtspServerBev, EV_WRITE);
		pthread_mutex_unlock(&(pRtspClientHead->mutexClientListMutex));
	}
	else  //盒子connect rtsp服务器失败
	{
		TRACE_ERR("\n###########  connect rtsp server failed !\n");
		bufferevent_disable(pConnectRtspServerBev, EV_READ|EV_WRITE);

		if (pMessengerArgs != NULL)
		{
			if (pRtspClientHead->iClientNum < 1)
			{
				//连接rtsp服务器失败，如果当前用户数为0，那么就将设备从链表中删除
				pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
				del_one_from_dev_list(pDevNode);
				pDevNode = NULL;
				pMessengerArgs->pDevNode = NULL;
				pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			}

			printf("free pMessengerArgs\n");
			free(pMessengerArgs);
			pMessengerArgs = NULL;
		}

		bufferevent_free(pConnectRtspServerBev);
		pConnectRtspServerBev = NULL;
	}
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


/*
 *	Function: 收到server_info信息后，连接rtsp服务器
 *
 *	@param pCmdBuf: [IN]存储rtsp服务器地址和端口的json串
 *  @parmm pMessengerArgs: [IN]libevent_server的参数结构体
 *
 *	Retrun: 无
 */
static HB_VOID connect_to_rtsp_server(HB_CHAR *pCmdBuf, LIBEVENT_ARGS_HANDLE pMessengerArgs)
{
	HB_CHAR arrServerIp[16] = {0};
	HB_S32 iServerPort = 0;
	struct sockaddr_in stServeraddr;
	HB_S32 iServeraddrLen;
	struct bufferevent *pSendVideoToServerEvent;//主动连接服务器事件

	analysis_json_server_info(pCmdBuf, arrServerIp, &iServerPort);
	TRACE_GREEN("rtsp_server_ip=[%s]\nrtsp_server_port=[%d]\n", arrServerIp, iServerPort);

	pSendVideoToServerEvent = bufferevent_socket_new(pMessengerArgs->pEventBase, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);
	bzero(&stServeraddr, sizeof(stServeraddr));
	stServeraddr.sin_family = AF_INET;
	stServeraddr.sin_port = htons(iServerPort);
	inet_pton(AF_INET, arrServerIp, &stServeraddr.sin_addr);
	iServeraddrLen = sizeof(struct sockaddr_in);

	bufferevent_setcb(pSendVideoToServerEvent, NULL, NULL, connect_to_rtsp_server_event_cb, (HB_VOID *)pMessengerArgs);
	bufferevent_socket_connect(pSendVideoToServerEvent, (struct sockaddr*)&stServeraddr, iServeraddrLen);
}
/***************************处理Server_info消息End****************************/
/***************************处理Server_info消息End****************************/
/***************************处理Server_info消息End****************************/


/***************************处理open_video消息End****************************/
/***************************处理open_video消息End****************************/
/***************************处理open_video消息End****************************/
/*
 *	Function: 处理与客户端信令交互时产生的异常和错误(第一次信令交互)
 *
 *	@param bev: 异常产生的事件句柄
 *	@param events: 异常事件类型
 *  @parmm args	: 实际为LIBEVENT_ARGS_HANDLE类型的参数结构体
 *
 *	Retrun: 无
 */
static HB_VOID deal_client_cmd_error_cb1(struct bufferevent *bev, short events, void *args)
{
	LIBEVENT_ARGS_HANDLE pMessengerArgs = (LIBEVENT_ARGS_HANDLE)args;

	if (events & BEV_EVENT_EOF)//对端关闭
	{
		TRACE_ERR("RTSP CMD peer client closed!");
	}
	else if (events & BEV_EVENT_ERROR)//错误事件
	{
		TRACE_ERR("Error from bufferevent deal_client_request_error_cb");
	}
	else if (events & BEV_EVENT_TIMEOUT)//超时事件
	{
		TRACE_ERR("RTSP CMD deal_client_request_error_cb  timeout !");
	}


	if (pMessengerArgs != NULL)
	{
		if (pMessengerArgs->pDevNode != NULL)
		{
			printf("del one dev from dev_list!\n");
			pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
			del_one_from_dev_list(pMessengerArgs->pDevNode);
			pMessengerArgs->pDevNode = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
		}
		if (pMessengerArgs->pClientBev != NULL)
		{
			printf("bufferevent_free pClientBev!\n");
			bufferevent_free(pMessengerArgs->pClientBev);
			pMessengerArgs->pClientBev = NULL;
		}
		printf("free pMessengerArgs\n");
		free(pMessengerArgs);
		pMessengerArgs = NULL;
	}
	else
	{
		//伪代码
		bufferevent_free(bev);
	}

}

/*
 *	Function: json字符串解析接口
 *
 *	@param p_SrcJson: [IN]需要解析的json字符串
 *	@param p_DevId: [OUT]解析出来的设备ID
 *  @parmm p_DevChnl : [OUT]解析出来的设备通道号
 *  @parmm p_DevChnl : [OUT]解析出来的设备主子码流
 *
 *	Retrun: 成功返回0，失败返回-1
 */
static HB_S32 analysis_json_dev_info(char *p_SrcJson, char *pDevId, int *pDevChnl, int *pDevStreamType)
{
	cJSON *pJson = cJSON_Parse(p_SrcJson);
	if(NULL == pJson)
	{
		return HB_FAILURE;
	}
	cJSON *p_sub = cJSON_GetObjectItem(pJson, "CmdType");
	if(NULL == p_sub)
	{
		cJSON_Delete(pJson);
		return HB_FAILURE;
	}
	if(!strcmp(p_sub->valuestring, "open_video"))
	{
		p_sub = cJSON_GetObjectItem(pJson, "DevId");
		strcpy(pDevId, p_sub->valuestring);
		p_sub = cJSON_GetObjectItem(pJson, "DevChnl");
		*pDevChnl = p_sub->valueint;
		p_sub = cJSON_GetObjectItem(pJson, "DevStreamType");
		*pDevStreamType = p_sub->valueint;
		cJSON_Delete(pJson);
	}
	else
	{
		cJSON_Delete(pJson);
		return HB_FAILURE;
	}
	return HB_SUCCESS;
}


/*
 *	Function: 收到打开视频流命令时的处理函数
 *
 *	@param pCmdBuf: [IN]命令字符串（json格式）
 *	@param pMessengerArgs: [IN]libevent参数结构体
 *
 *	Retrun: 成功返回0，失败返回-1
 */
static HB_S32 deal_open_video_cmd(HB_CHAR *pCmdBuf, LIBEVENT_ARGS_HANDLE pMessengerArgs)
{
	HB_S32 iDevChnl = -1;		//设备通道号
	HB_S32 iDevStreamType = -1;//主子码流
	DEV_LIST_HANDLE pDevNode = NULL;
	HB_CHAR accDevId[MAX_DEV_ID_LEN] = {0}; //设备ID

	analysis_json_dev_info(pCmdBuf, accDevId, &iDevChnl, &iDevStreamType);
	TRACE_YELLOW("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", accDevId, iDevChnl, iDevStreamType);

	pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
	pDevNode = find_in_dev_list(accDevId, iDevChnl, iDevStreamType);
	if (pDevNode == NULL)
	{
		//未找到设备
		HB_CHAR sql[512] = {0};
		pDevNode = (DEV_LIST_HANDLE)malloc(sizeof(DEV_LIST_OBJ));
		memset(pDevNode, 0, sizeof(DEV_LIST_OBJ));

		snprintf(sql, sizeof(sql), "select dev_ip,dev_port from device_info where dev_id='%s'", accDevId);
		if (sql_operation(sql, DEV_DATA_BASE_NAME, load_device_ip_port_cb, (HB_VOID *)pDevNode) < 0)
		{
			//数据库操作失败
			free(pDevNode);
			pDevNode = NULL;
			if (pMessengerArgs->pClientBev != NULL)
			{
				bufferevent_free(pMessengerArgs->pClientBev);
				pMessengerArgs->pClientBev = NULL;
			}
			free(pMessengerArgs);
			pMessengerArgs = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			TRACE_ERR("data_base[%s] exec[%s] failed!\n", DEV_DATA_BASE_NAME, sql);
			return HB_FAILURE;
		}
		if ((pDevNode->arrDevIp == NULL) || (strlen(pDevNode->arrDevIp) < 7))//ip长度至少为7位(0.0.0.0)
		{
			//设备不存在在本地数据库
			free(pDevNode);
			pDevNode = NULL;
			if (pMessengerArgs->pClientBev != NULL)
			{
				bufferevent_free(pMessengerArgs->pClientBev);
				pMessengerArgs->pClientBev = NULL;
			}
			free(pMessengerArgs);
			pMessengerArgs = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			TRACE_ERR("The device [%s] do not in the database!\n", accDevId);
			return HB_FAILURE;
		}


		strncpy(pDevNode->pDevId, accDevId, sizeof(pDevNode->pDevId));
		pDevNode->iDevChnl = iDevChnl;
		pDevNode->iDevStreamType = iDevStreamType;

		pthread_mutex_init(&(pDevNode->stRtspClientHead.mutexClientListMutex), NULL);

		TRACE_GREEN("dev_id:[%s], dev_chnl:[%d], dev_streadm:[%d], dev_ip:[%s], dev_port:[%d]\n", \
				pDevNode->pDevId, pDevNode->iDevChnl, \
				pDevNode->iDevStreamType, pDevNode->arrDevIp, \
				pDevNode->iDevRtspPort);

		pMessengerArgs->pDevNode = pDevNode;//livevent参数结构体中记录当前设备的地址
//		pDevNode->stClientListHead.i_ClientNum = 0;
//		memset(&(pDevNode->stClientListHead), 0, sizeof(CLIENT_LIST_HEAD_OBJ));
		add_to_dev_list(pDevNode);
		pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
		test_dev_connection(pMessengerArgs);

	}
	else
	{
		//找到了设备

	}
	pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));

	return 0;
}
/***************************处理open_video消息End****************************/
/***************************处理open_video消息End****************************/
/***************************处理open_video消息End****************************/


/*
 *	Function: 读取回调函数,用于处理接从客户端接收到的信令，并根据信令能容做相应处理
 *
 *	@param client_bev: 与客户端信令交互的bufferevent事件。
 *	@param arg: 事件触发时传入的参数。
 *
 *	Retrun: 无
 */
HB_VOID deal_client_cmd(struct bufferevent *pClientBev, void *arg)
{
	BOX_CTRL_CMD_OBJ st_MsgHead;
	LIBEVENT_ARGS_HANDLE pMessengerArgs = (LIBEVENT_ARGS_HANDLE)arg;

	if (pMessengerArgs == NULL)
	{
		printf("pMessengerArgs is NULL!\n");
		return;
	}

	CMD_TYPE enum_DataType = pMessengerArgs->enumCmdType;
	HB_CHAR arrc_RecvCmdBuf[CMD_MAX_LEN] = {0};

	struct evbuffer *src = bufferevent_get_input(pClientBev);//获取输入缓冲区
	HB_S32 len = evbuffer_get_length(src);//获取输入缓冲区中数据的长度，也就是可以读取的长度。

	//读取缓冲区数据长度
	if (len > sizeof(BOX_CTRL_CMD_OBJ))
	{
		//如果命令头和命令体是一条命令发过来的则统一处理
		if (pMessengerArgs->enumCmdType != CMD_BODY)
		{
			enum_DataType = CMD_ALL;
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
					free(pMessengerArgs);
					//头消息错误，直接关闭返回
					bufferevent_free(pClientBev);
					return ;
				}
				pMessengerArgs->enumCmdType = CMD_BODY;
			}
			break;
		}
		case CMD_BODY: //命令信息
		{
			printf("cmd body!\n");
#if 0
			//读取命令信息
			if (evbuffer_remove(src, arrc_RecvCmdBuf, CMD_MAX_LEN) > 0)
			{
				if (strstr(arrc_RecvCmdBuf, "open_video") != NULL)
				{
					printf("recv recv recv recv open_video:[%s]\n", arrc_RecvCmdBuf);
//					p_CommunicateTags->client_bev = client_bev;
					//收到rtsp打开请求
					deal_open_video_cmd(arrc_RecvCmdBuf, pMessengerArgs);
				}
				else if (strstr(arrc_RecvCmdBuf, "server_info") != NULL)
				{
					printf("recv recv recv recv server_info:[%s]\n", arrc_RecvCmdBuf);
					//拿到了rtsp服务器的ip和端口，需要将视频流发到这里
					//创建线程并发送数据
					bufferevent_free(client_bev);
//					p_CommunicateTags->client_bev = NULL;
					start_send_video(arrc_RecvCmdBuf, pMessengerArgs);
				}
			}
#endif
			break;
		}
		case CMD_ALL://头信息和命令信息一起发过来的
		{
			memset(&st_MsgHead, 0, sizeof(BOX_CTRL_CMD_OBJ));
			if(evbuffer_remove(src, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ)))
			{
				printf("read all recv_header = [%s]\n", st_MsgHead.header);
				if (strncmp(st_MsgHead.header, "hBzHbox@", 8))
				{
					//头消息错误，直接关闭返回
					bufferevent_free(pClientBev);
					return ;
				}
				if (evbuffer_remove(src, arrc_RecvCmdBuf, CMD_MAX_LEN) > 0)
				{
					if (strstr(arrc_RecvCmdBuf, "open_video") != NULL)
					{
						printf("read all recv recv recv recv open_video:[%s]\n", arrc_RecvCmdBuf);
						pMessengerArgs->pClientBev = pClientBev;
						//收到rtsp打开请求
						deal_open_video_cmd(arrc_RecvCmdBuf, pMessengerArgs);
					}
#if 1
					else if (strstr(arrc_RecvCmdBuf, "server_info") != NULL)
					{

						printf("recv recv recv recv server_info:[%s]\n", arrc_RecvCmdBuf);
						//拿到了rtsp服务器的ip和端口，需要将视频流发到这里
						//创建线程并发送数据
						bufferevent_free(pClientBev);
						pClientBev = NULL;
						pMessengerArgs->pClientBev = NULL;
						connect_to_rtsp_server(arrc_RecvCmdBuf, pMessengerArgs);
					}
#endif
				}
			}
			break;
		}
		default:
			break;
	}
}


/*
 *	Function: 接收到客户端连接的回调函数
 *
 *	@param pListener : 监听句柄
 *  @param iAcceptSockfd : 收到连接后分配的文件描述符（与客户端连接的文件描述符）
 *  @param pClientAddr : 客户端地址结构体
 *
 *	Retrun: 无
 */
static HB_VOID accept_client_connect_cb(struct evconnlistener *pListener, evutil_socket_t iAcceptSockfd,
	    struct sockaddr *pClientAddr, int slen, void *arg)
{
	struct timeval tv_w;
	LIBEVENT_ARGS_HANDLE pMessengerArgs = (LIBEVENT_ARGS_HANDLE)malloc(sizeof(LIBEVENT_ARGS_OBJ));
	memset(pMessengerArgs, 0, sizeof(LIBEVENT_ARGS_OBJ));

	pMessengerArgs->enumCmdType = CMD_HEAD;
	struct event_base *pEventBase = evconnlistener_get_base(pListener);;
	pMessengerArgs->pEventBase = pEventBase;

#if 1
	//打印对端ip
	struct sockaddr_in *p_PeerAddr = (struct sockaddr_in *)pClientAddr;
	char arrch_PeerIp[16] = {0};
	inet_ntop(AF_INET, &(p_PeerAddr->sin_addr), arrch_PeerIp, sizeof(arrch_PeerIp));
	printf("\nAccept a new connect ! sockfd=%d accept ip=%s : %d\n", iAcceptSockfd,arrch_PeerIp, ntohs(p_PeerAddr->sin_port));
#endif

    // 为新的连接分配并设置 bufferevent,设置BEV_OPT_CLOSE_ON_FREE宏后，当连接断开时也会通知客户端关闭套接字
    struct bufferevent *accept_sockfd_bev = bufferevent_socket_new(pEventBase, iAcceptSockfd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);

    //设置超时，5秒内未收到对端发来数据则断开连接
	tv_w.tv_sec  = 5;
	tv_w.tv_usec = 0;
	bufferevent_set_timeouts(accept_sockfd_bev, &tv_w, NULL);
    bufferevent_setcb(accept_sockfd_bev, deal_client_cmd, NULL, deal_client_cmd_error_cb1, (void*)pMessengerArgs);
    bufferevent_enable(accept_sockfd_bev, EV_READ);

    return;
}


/*
 * function : libevent服务器主监听函数
 *
 *  @param : none
 *
 * return : none
 */
HB_VOID libevent_server_main_listen()
{
	struct event_base *pEventBase;
	struct evconnlistener *pListener;
	struct sockaddr_in  stListenAddr;

	if(evthread_use_pthreads() != 0)
	{
		TRACE_ERR("########## evthread_use_pthreads() err !");
		return ;
	}

	bzero(&stListenAddr, sizeof(stListenAddr));
	stListenAddr.sin_family = AF_INET;
	stListenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	stListenAddr.sin_port = htons(BOX_SERVER_PORT);

	pEventBase = event_base_new();
	if (!pEventBase)
	{
		perror("cmd_base event_base_new()");
		return ;
	}
	//用于线程安全
	if(evthread_make_base_notifiable(pEventBase) != 0)
	{
		TRACE_ERR("###### evthread_make_base_notifiable() err!");
		return ;
	}

	//绑定端口并接收连接
	pListener = evconnlistener_new_bind(pEventBase, accept_client_connect_cb, NULL,
			LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE|LEV_OPT_THREADSAFE,
			1024, (struct sockaddr*)&stListenAddr, sizeof(struct sockaddr_in));
	printf("start listening : \n");
	event_base_dispatch(pEventBase);
	evconnlistener_free(pListener);
	event_base_free(pEventBase);

}
