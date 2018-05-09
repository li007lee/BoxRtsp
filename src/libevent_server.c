/*
 * libevent_server.c
 *
 *  Created on: 2017年9月21日
 *      Author: lijian
 */

#include "libevent_server.h"
#include "sqlite3.h"
#include "dev_opt.h"
#include "url_code.h"
#include "simclist.h"

struct event_base *pEventBase;
extern HB_CHAR	 glBoxSn[32];
extern HB_CHAR cServerIp[16];

/****************************************数据库操作****************************************/
/****************************************数据库操作****************************************/
/****************************************数据库操作****************************************/

HB_VOID deal_write_ok(struct bufferevent *pBev, void *arg)
{
	struct evbuffer *bevbuf = bufferevent_get_output(pBev);//获取输出缓冲区
	if (evbuffer_get_length(bevbuf) == 0)//获取输入缓冲区中数据的长度，也就是可以读取的长度。
	{
		printf("write ok. close bev.\n");
		//缓冲区无数据
		bufferevent_free(pBev);
		pBev = NULL;
	}
	else
	{
		printf("write data...\n");
	}
}

//获取数据条数回调函数
static HB_S32 load_device_ip_port_cb( HB_VOID * para, HB_S32 n_column, HB_CHAR ** column_value, HB_CHAR ** column_name )
{
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)para;
	HB_CHAR cTmpBuf[512] = {0};
	strncpy(pDevNode->arrDevIp, column_value[0], strlen(column_value[0]));
	pDevNode->iDevRtspPort = atoi(column_value[1]);
	strncpy(pDevNode->arrUserName, column_value[2], strlen(column_value[2]));
	strncpy(pDevNode->arrUserPasswd, column_value[3], strlen(column_value[3]));
	strncpy(pDevNode->arrBasicAuthenticate, column_value[4], strlen(column_value[4]));

	strncpy(cTmpBuf, column_value[5], strlen(column_value[5]));
	snprintf(pDevNode->arrDevRtspMainUrl, sizeof(pDevNode->arrDevRtspMainUrl), \
					"rtsp://%s:%d%s", pDevNode->arrDevIp, pDevNode->iDevRtspPort, cTmpBuf);
//	strncpy(pDevNode->arrDevRtspMainUrl, column_value[5], strlen(column_value[5]));

	memset(cTmpBuf, 0, sizeof(cTmpBuf));
	strncpy(cTmpBuf, column_value[6], strlen(column_value[6]));
	snprintf(pDevNode->arrDevRtspSubUrl, sizeof(pDevNode->arrDevRtspSubUrl), \
					"rtsp://%s:%d%s", pDevNode->arrDevIp, pDevNode->iDevRtspPort, cTmpBuf);

//	strncpy(pDevNode->arrDevRtspSubUrl, column_value[6], strlen(column_value[6]));
	printf("arrDevRtspMainUrl:[%s]\n", pDevNode->arrDevRtspMainUrl);
	printf("arrDevRtspSubUrl:[%s]\n", pDevNode->arrDevRtspSubUrl);

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
	LIBEVENT_ARGS_DEV_HANDLE pMessengerArgsDev = (LIBEVENT_ARGS_DEV_HANDLE)arg;
	CLIENT_LIST_HANDLE pClientNode = pMessengerArgsDev->pClientNode;

//	CLIENT_LIST_HEAD_HANDLE pRtspClientHead = &(pMessengerArgsDev->pDevNode->stRtspClientHead);
	list_t *plistRtspClient = (list_t *)&(pMessengerArgsDev->pDevNode->listRtspClient);

	HB_S32 err = EVUTIL_SOCKET_ERROR();

	if (what & BEV_EVENT_TIMEOUT)//超时
	{
		TRACE_ERR("\n###########  send_rtsp_to_server_event_error_cb(%d) : send frame  timeout(%s) !\n", err, evutil_socket_error_to_string(err));
	}
	else if (what & BEV_EVENT_ERROR)  //错误
	{
		TRACE_ERR("\n###########  send_rtsp_to_server_event_error_cb(%d) : send frame failed(%s) !\n", err, evutil_socket_error_to_string(err));
	}
	else if (what & BEV_EVENT_EOF) //对端主动关闭
	{
		TRACE_YELLOW("\n###########  send_rtsp_to_server_event_error_cb(%d) : connection closed by peer(%s)!\n", err, evutil_socket_error_to_string(err));
	}
	else
	{
		TRACE_ERR("\n###########  send_rtsp_to_server_event_error_cb(%d) : (%s) !\n", err, evutil_socket_error_to_string(err));
	}

	if (pMessengerArgsDev->pDevNode->iStartThreadFlag == 1) //如果发送线程已经启动，此处只置位
	{
		bufferevent_disable(pClientNode->pSendVideoToServerEvent, EV_READ|EV_WRITE);
		pClientNode->iDelFlag = 1;
	}
	else //若没有启动，从此处释放
	{
//		pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
		bufferevent_disable(pClientNode->pSendVideoToServerEvent, EV_READ|EV_WRITE);
		if (pMessengerArgsDev != NULL)
		{
			pthread_rwlock_wrlock(&rwlockMyLock);
			if (pClientNode != NULL)
			{
//				printf("del one dev from dev_list!\n");
//				del_one_client(pRtspClientHead, pClientNode);
				list_delete(plistRtspClient, (HB_VOID *)pClientNode);
				pMessengerArgsDev->pClientNode = NULL;
			}

			if ((pMessengerArgsDev->pDevNode != NULL) && (list_size(plistRtspClient) < 1))
			{
				//当前设备下如果已经没有用户了，那么需要把设备从设备链表摘除
//				printf("del one dev from dev_list!\n");
//				del_one_from_dev_list(pMessengerArgsDev->pDevNode);
				list_delete(&listDevList, (HB_VOID *)pMessengerArgsDev->pDevNode);
				pMessengerArgsDev->pDevNode = NULL;
			}
			pthread_rwlock_unlock(&rwlockMyLock);
			TRACE_ERR("send_rtsp_to_server_event_error_cb free pMessengerArgs\n");
			free(pMessengerArgsDev);
			pMessengerArgsDev = NULL;
//			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
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
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)arg;
	list_t *plistRtspClient = (list_t *)&(pDevNode->listRtspClient);
//	CLIENT_LIST_HEAD_HANDLE pRtspClientHead = &(pDevNode->stRtspClientHead);

	if (what & BEV_EVENT_CONNECTED)//盒子主动连接rtsp服务器成功
	{
		TRACE_GREEN("Connect to rtsp server succeed！！！！！！！！！！！\n");
		//连接成功创建客户节点
		CLIENT_LIST_HANDLE pClientNode = (CLIENT_LIST_HANDLE)malloc(sizeof(CLIENT_LIST_OBJ));
		memset(pClientNode, 0, sizeof(CLIENT_LIST_OBJ));
		pClientNode->pSendVideoToServerEvent = pConnectRtspServerBev;

		pthread_rwlock_wrlock(&rwlockMyLock);
		//插入链表
		list_append(plistRtspClient, (HB_VOID *)pClientNode);
		//启动读取视频流线程
		if (pDevNode->iStartThreadFlag == 0)
		{
			//如果当前设备还没有启动推流线程，则启动
			pthread_create(&(pDevNode->threadReadVideoId), NULL, read_video_data_from_dev_task, (HB_VOID*)pDevNode);
		}
		LIBEVENT_ARGS_DEV_HANDLE pMessengerArgsDev = (LIBEVENT_ARGS_DEV_HANDLE)malloc(sizeof(LIBEVENT_ARGS_DEV_OBJ));
		memset(pMessengerArgsDev, 0, sizeof(LIBEVENT_ARGS_DEV_OBJ));
		pMessengerArgsDev->pDevNode = pDevNode;
		pMessengerArgsDev->pClientNode = pClientNode;
		TRACE_YELLOW("\n###########  total client total client total client total client== [%d]!\n", list_size(plistRtspClient));
		//设置超时，10秒内未收到对端发来数据则断开连接
		struct timeval tv_w = {10, 0};
		//设置数据发送的写超时
		bufferevent_set_timeouts(pConnectRtspServerBev, NULL, &tv_w);
		//设置连接出错后的回调函数
		bufferevent_setcb(pConnectRtspServerBev, NULL, NULL, send_rtsp_to_server_event_error_cb, (HB_VOID *)pMessengerArgsDev);
		bufferevent_enable(pConnectRtspServerBev, EV_WRITE);
		pthread_rwlock_unlock(&rwlockMyLock);
//		pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
	}
	else  //盒子connect rtsp服务器失败
	{
		TRACE_ERR("\n###########  connect rtsp server failed !\n");
		bufferevent_disable(pConnectRtspServerBev, EV_READ|EV_WRITE);
		TRACE_YELLOW("\n###########  total client total client total client total client== [%d]!\n", list_size(plistRtspClient));
		if (list_size(plistRtspClient) < 1)
		{
			//连接rtsp服务器失败，如果当前用户数为0，那么就将设备从链表中删除
			pthread_rwlock_wrlock(&rwlockMyLock);
//			del_one_from_dev_list(pDevNode);
			list_delete(&listDevList, (HB_VOID *)pDevNode);
			pDevNode = NULL;
			pthread_rwlock_unlock(&rwlockMyLock);
		}

		bufferevent_free(pConnectRtspServerBev);
		pConnectRtspServerBev = NULL;
	}
}


/*
 *	Function: 收到server_info信息后，连接rtsp服务器
 *
 *	@param pCmdBuf: [IN]存储rtsp服务器地址和端口的json串
 *  @parmm pDevNode: [IN]设备节点的指针
 *
 *	Retrun: 无
 */
static HB_VOID connect_to_rtsp_server(HB_CHAR *pServerIp, HB_S32 iServerPort, DEV_LIST_HANDLE pDevNode)
{
	struct sockaddr_in stServeraddr;
	HB_S32 iServeraddrLen;
	struct bufferevent *pSendVideoToServerEvent;//主动连接服务器事件

	pSendVideoToServerEvent = bufferevent_socket_new(pEventBase, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);
	bzero(&stServeraddr, sizeof(stServeraddr));
	stServeraddr.sin_family = AF_INET;
	stServeraddr.sin_port = htons(iServerPort);
	inet_pton(AF_INET, pServerIp, &stServeraddr.sin_addr);
	iServeraddrLen = sizeof(struct sockaddr_in);

	bufferevent_setcb(pSendVideoToServerEvent, NULL, NULL, connect_to_rtsp_server_event_cb, (HB_VOID *)pDevNode);
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
//	LIBEVENT_ARGS_HANDLE pMessengerArgs = (LIBEVENT_ARGS_HANDLE)args;
	HB_S32 err = EVUTIL_SOCKET_ERROR();

	if (events & BEV_EVENT_EOF)//对端关闭
	{
		TRACE_ERR("######## deal_client_cmd_error_cb1 BEV_EVENT_EOF(%d) : %s !", err, evutil_socket_error_to_string(err));
	}
	else if (events & BEV_EVENT_ERROR)//错误事件
	{
		TRACE_ERR("######## deal_client_cmd_error_cb1 BEV_EVENT_ERROR(%d) : %s !", err, evutil_socket_error_to_string(err));
	}
	else if (events & BEV_EVENT_TIMEOUT)//超时事件
	{
		TRACE_ERR("######## deal_client_cmd_error_cb1 BEV_EVENT_TIMEOUT(%d) : %s !", err, evutil_socket_error_to_string(err));
	}

	bufferevent_free(bev);
	bev = NULL;
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
	cJSON *pJsonCmdType = cJSON_GetObjectItem(pJson, "cmdType");
	if(NULL == pJsonCmdType)
	{
		cJSON_Delete(pJson);
		return HB_FAILURE;
	}
	if(!strcmp(pJsonCmdType->valuestring, "open_video"))
	{
		cJSON *pJsonDevId = cJSON_GetObjectItem(pJson, "devId");
		if (pJsonDevId != NULL)
			strcpy(pDevId, pJsonDevId->valuestring);
		cJSON *pJsonDevChnl = cJSON_GetObjectItem(pJson, "devChnl");
		if (pJsonDevChnl != NULL)
			*pDevChnl = pJsonDevChnl->valueint;
		cJSON *pJsonStreamType = cJSON_GetObjectItem(pJson, "devStreamType");
		if (pJsonStreamType != NULL)
			*pDevStreamType = pJsonStreamType->valueint;
		cJSON_Delete(pJson);
	}
	else
	{
		cJSON_Delete(pJson);
		return HB_FAILURE;
	}
	return HB_SUCCESS;
}


//设备的查找器(从链表中查找当前设备是不是已经存在)
static HB_S32 DevSeeker(const HB_VOID *pDevNode, const HB_VOID *key)
{
	if ((pDevNode == NULL) || (key == NULL))
	{
		TRACE_ERR("called with NULL pointer: pDevNode=%p, key=%p", pDevNode, key);
		return 0;
	}
	DEV_LIST_HANDLE pCurrentDev = (DEV_LIST_HANDLE)pDevNode;
	DEV_INFO_HANDLE pKey = (DEV_INFO_HANDLE)key;

	//设备id，通道号，主子码流一致才算找到设备
	if ((strcmp(pCurrentDev->pDevId, pKey->pDevId) == 0) \
		&& (pCurrentDev->iDevChnl == pKey->iDevChnl) \
		&& (pCurrentDev->iDevStreamType == pKey->iDevStreamType))
	{
		TRACE_YELLOW("The device is already exist!\n");
		return 1;
	}

	return 0;
}

/*
 *	Function: 收到打开视频流命令时的处理函数
 *
 *	@param pCmdBuf: [IN]命令字符串（json格式）
 *	@param pClientBev: [IN]客户端连接过来的事件句柄
 *
 *	Retrun: 成功返回0，失败返回-1
 */
static HB_S32 deal_open_video_cmd(HB_CHAR *pCmdBuf, struct bufferevent *pClientBev)
{
	HB_S32 iDevChnl = -1;		//设备通道号
	HB_S32 iDevStreamType = -1;//主子码流 0主码流，1子码流
	DEV_LIST_HANDLE pDevNode = NULL;
	HB_CHAR accDevId[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR accDevIdTmp[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR accDevIdDecode[MAX_DEV_ID_LEN] = {0}; //设备ID

	analysis_json_dev_info(pCmdBuf, accDevIdTmp, &iDevChnl, &iDevStreamType);
	url_decode(accDevIdTmp, strlen(accDevIdTmp), accDevIdDecode, MAX_DEV_ID_LEN);
//	strncpy(accDevId, accDevIdDecode+strlen(glBoxSn), MAX_DEV_ID_LEN);
	strncpy(accDevId, accDevIdDecode, MAX_DEV_ID_LEN);

	TRACE_YELLOW("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", accDevIdDecode, iDevChnl, iDevStreamType);

#ifdef HAND_SERVER_IP
	memset(accDevId, 0, sizeof(accDevId));
	strcpy(accDevId, "DS-2CD1201D-I320170526AACH766877798");
#endif

	DEV_INFO_OBJ stCurDev;
	memset(&stCurDev, 0, sizeof(DEV_INFO_OBJ));

	list_attributes_seeker(&listDevList, DevSeeker);
	strncpy(stCurDev.pDevId, accDevId, sizeof(stCurDev.pDevId));
	stCurDev.iDevChnl = iDevChnl;
	stCurDev.iDevStreamType = iDevStreamType;
	pthread_rwlock_wrlock(&rwlockMyLock);
	pDevNode = list_seek(&listDevList, &stCurDev);
	if (pDevNode == NULL)
	{
		//未找到设备
		HB_CHAR sql[1024] = {0};

		pDevNode = (DEV_LIST_HANDLE)malloc(sizeof(DEV_LIST_OBJ));
		memset(pDevNode, 0, sizeof(DEV_LIST_OBJ));

		list_init(&(pDevNode->listRtspClient));
		list_init(&(pDevNode->listWaitClient));

		TRACE_YELLOW("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", accDevId, iDevChnl, iDevStreamType);
		snprintf(sql, sizeof(sql), \
			"select dev_ip,rtsp_port,dev_login_usr,dev_login_passwd,basic_authenticate,rtsp_list.rtsp_main,rtsp_list.rtsp_sub from onvif_dev_data left join rtsp_list on onvif_dev_data.dev_id='%s' where rtsp_list.[dev_id]=onvif_dev_data.dev_id and rtsp_list.[dev_chnl_num]='%d'", \
			accDevId, iDevChnl);
		if (sql_operation(sql, DEV_DATA_BASE_NAME, load_device_ip_port_cb, (HB_VOID *)pDevNode) < 0)
		{
			//数据库操作失败
			free(pDevNode);
			pDevNode = NULL;
			pthread_rwlock_unlock(&rwlockMyLock);
			TRACE_ERR("data_base[%s] exec[%s] failed!\n", DEV_DATA_BASE_NAME, sql);
			return HB_FAILURE;
		}
		if ((pDevNode->arrDevIp == NULL) || (strlen(pDevNode->arrDevIp) < 7))//ip长度至少为7位(0.0.0.0)
		{
			//设备不存在在本地数据库
			free(pDevNode);
			pDevNode = NULL;
			pthread_rwlock_unlock(&rwlockMyLock);
			TRACE_ERR("The device [%s] do not in the database!\n", accDevId);
			return HB_FAILURE;
		}


		strncpy(pDevNode->pDevId, accDevId, sizeof(pDevNode->pDevId));
		pDevNode->iDevChnl = iDevChnl;
		pDevNode->iDevStreamType = iDevStreamType;
		pDevNode->enumDevConnectStatus = CONNECTING;

		TRACE_GREEN("dev_id:[%s], dev_chnl:[%d], dev_streadm:[%d], dev_ip:[%s], dev_port:[%d]\n", \
				pDevNode->pDevId, pDevNode->iDevChnl, pDevNode->iDevStreamType, pDevNode->arrDevIp, \
				pDevNode->iDevRtspPort);

		list_append(&listDevList, pDevNode);
		//添加到等待用户链表
		WAIT_CLIENT_LIST_HANDLE pNewWaitClientNode = (WAIT_CLIENT_LIST_HANDLE)malloc(sizeof(WAIT_CLIENT_LIST_OBJ));
		memset(pNewWaitClientNode, 0, sizeof(WAIT_CLIENT_LIST_OBJ));
		pNewWaitClientNode->pWaitClientBev = pClientBev;
		list_append(&(pDevNode->listWaitClient), (HB_VOID *)pNewWaitClientNode);
		pthread_rwlock_unlock(&rwlockMyLock);
		test_dev_connection(pDevNode);
	}
	else
	{
		//设备已存在，判断设备连接状态
		if(pDevNode->enumDevConnectStatus == CONNECTED) //已连接
		{
			pthread_rwlock_unlock(&rwlockMyLock);
			BOX_CTRL_CMD_OBJ st_MsgHead;
			HB_CHAR arr_SendBuf[2048] = {0};
			memset(&st_MsgHead, 0, sizeof(st_MsgHead));
			memcpy(st_MsgHead.header, "hBzHbox@", 8);
			st_MsgHead.cmd_code = CMD_OK;

			snprintf(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ), 2048-sizeof(BOX_CTRL_CMD_OBJ), \
					"{\"CmdType\":\"sdp_info\",\"m_video\":\"%s\",\"a_rtpmap_video\":\"%s\",\"a_fmtp_video\":\"%s\",\"m_audio\":\"%s\",\"a_rtpmap_audio\":\"%s\"}", \
					pDevNode->m_video, pDevNode->a_rtpmap_video, pDevNode->a_fmtp_video, pDevNode->m_audio, pDevNode->a_rtpmap_audio);
			st_MsgHead.cmd_length = htonl(strlen(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ)));
			memcpy(arr_SendBuf, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ));

			bufferevent_write(pClientBev, arr_SendBuf, ntohl(st_MsgHead.cmd_length)+sizeof(BOX_CTRL_CMD_OBJ));
			bufferevent_setcb(pClientBev, deal_client_cmd, NULL, NULL, (HB_VOID *)pDevNode);
			bufferevent_enable(pClientBev, EV_READ);
		}
		else if(pDevNode->enumDevConnectStatus == CONNECTING) //连接中
		{
			WAIT_CLIENT_LIST_HANDLE pNewWaitClientNode = (WAIT_CLIENT_LIST_HANDLE)malloc(sizeof(WAIT_CLIENT_LIST_OBJ));
			memset(pNewWaitClientNode, 0, sizeof(WAIT_CLIENT_LIST_OBJ));

			pNewWaitClientNode->pWaitClientBev = pClientBev;
			//如果设备还在连接中，将设备插入等待链表
			list_append(&(pDevNode->listWaitClient), (HB_VOID *)pNewWaitClientNode);
			printf("add add add add add add add add %d\n", list_size(&(pDevNode->listWaitClient)));
			pthread_rwlock_unlock(&rwlockMyLock);
		}
	}

	return 0;
}
/***************************处理open_video消息End****************************/
/***************************处理open_video消息End****************************/
/***************************处理open_video消息End****************************/


/***************************处理push_stream消息****************************/
/***************************处理push_stream消息****************************/
/***************************处理push_stream消息****************************/

/*
 *	Function: 向rtsp服务器发起连接的事件处理回调函数
 *
 *	@param pConnectRtspServerBev: [IN]连接rtsp服务器的事件句柄
 *  @parmm what: [IN]事件触发状态的类型
 *  @parmm arg： [IN]libevent 参数结构体，类型为LIBEVENT_ARGS_HANDLE
 *
 *	Retrun: 无
 */
static HB_VOID send_msg_to_push_client(struct bufferevent *pSendMsgToPushClientEvent, HB_S16 what, HB_VOID *arg)
{
	if (what & BEV_EVENT_CONNECTED)//盒子主动连接rtsp服务器成功
	{
		TRACE_GREEN("Connect succeed！！！！！！！！！！！\n");
		HB_CHAR *pSendBuf = (HB_CHAR *)arg;
		bufferevent_write(pSendMsgToPushClientEvent, pSendBuf, strlen(pSendBuf+sizeof(BOX_CTRL_CMD_OBJ))+sizeof(BOX_CTRL_CMD_OBJ));
		bufferevent_setcb(pSendMsgToPushClientEvent, deal_write_ok, NULL, NULL, NULL);
		bufferevent_enable(pSendMsgToPushClientEvent, EV_WRITE);
	}
	else  //盒子connect rtsp服务器失败
	{
		TRACE_ERR("\n###########  connect pSendMsgToPushClient failed !\n");
		bufferevent_disable(pSendMsgToPushClientEvent, EV_READ|EV_WRITE);

		bufferevent_free(pSendMsgToPushClientEvent);
		pSendMsgToPushClientEvent = NULL;
	}
}

/*
 *	Function: 收到推流命令
 *
 *	@param pCmdBuf: [IN]命令字符串（json格式）
 *	@param pClientBev: [IN]客户端连接过来的事件句柄
 *
 *	Retrun: 成功返回0，失败返回-1
 */
static HB_S32 push_stream(HB_CHAR *pCmdBuf, struct bufferevent *pClientBev)
{
	HB_S32 iDevChnl = -1;		//设备通道号
	HB_S32 iDevStreamType = -1;//主子码流 0主码流，1子码流
	DEV_LIST_HANDLE pDevNode = NULL;
	HB_CHAR arrServerIp[16] = {0}; //数据需要推送到的服务器ip
	HB_S32 iServerPort = 0; //数据需要推送到的服务器端口
	HB_CHAR arrProtocol[8] = {0}; //设备ID
	HB_CHAR arrDevId[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR arrDevIdTmp[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR arrDevIdDecode[MAX_DEV_ID_LEN] = {0}; //设备ID

	cJSON *pRoot;
	pRoot = cJSON_Parse(pCmdBuf);
	cJSON *pProtocol = cJSON_GetObjectItem(pRoot, "protocol");
	strncpy(arrProtocol, pProtocol->valuestring, sizeof(arrProtocol)); //rtsp或hls
	cJSON *pServerIp = cJSON_GetObjectItem(pRoot, "serverIp");
	strncpy(arrServerIp, pServerIp->valuestring, sizeof(arrServerIp));
	cJSON *pServerPort = cJSON_GetObjectItem(pRoot, "serverPort");
	iServerPort = pServerPort->valueint;
	cJSON *pDevId = cJSON_GetObjectItem(pRoot, "devId");
	strncpy(arrDevIdTmp, pDevId->valuestring, sizeof(arrDevIdTmp));
	cJSON *pDevChnl = cJSON_GetObjectItem(pRoot, "devChnl");
	iDevChnl = pDevChnl->valueint;
	cJSON *pStreamType = cJSON_GetObjectItem(pRoot, "streamType");
	iDevStreamType = pStreamType->valueint;

	url_decode(arrDevIdTmp, strlen(arrDevIdTmp), arrDevIdDecode, MAX_DEV_ID_LEN);
	strncpy(arrDevId, arrDevIdDecode+strlen(glBoxSn), MAX_DEV_ID_LEN);
//	TRACE_YELLOW("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", arrDevIdDecode, iDevChnl, iDevStreamType);
	DEV_INFO_OBJ stCurDev;
	memset(&stCurDev, 0, sizeof(DEV_INFO_OBJ));

	list_attributes_seeker(&listDevList, DevSeeker);

	strncpy(stCurDev.pDevId, arrDevId, sizeof(stCurDev.pDevId));
	stCurDev.iDevChnl = iDevChnl;
	stCurDev.iDevStreamType = iDevStreamType;

	pthread_rwlock_wrlock(&rwlockMyLock);
	pDevNode = list_seek(&listDevList, &stCurDev);
	if (pDevNode == NULL)
	{
		//未找到设备
		HB_CHAR sql[1024] = {0};
		pDevNode = (DEV_LIST_HANDLE)malloc(sizeof(DEV_LIST_OBJ));
		memset(pDevNode, 0, sizeof(DEV_LIST_OBJ));
		list_init(&(pDevNode->listRtspClient));

		TRACE_YELLOW("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", arrDevId, iDevChnl, iDevStreamType);
		snprintf(sql, sizeof(sql), \
			"select dev_ip,rtsp_port,dev_login_usr,dev_login_passwd,basic_authenticate,rtsp_list.rtsp_main,rtsp_list.rtsp_sub from onvif_dev_data left join rtsp_list on onvif_dev_data.dev_id='%s' where rtsp_list.[dev_id]=onvif_dev_data.dev_id and rtsp_list.[dev_chnl_num]='%d'", \
			arrDevId, iDevChnl);
		if (sql_operation(sql, DEV_DATA_BASE_NAME, load_device_ip_port_cb, (HB_VOID *)pDevNode) < 0)
		{
			//数据库操作失败
			free(pDevNode);
			pDevNode = NULL;
			pthread_rwlock_unlock(&rwlockMyLock);
			TRACE_ERR("data_base[%s] exec[%s] failed!\n", DEV_DATA_BASE_NAME, sql);
			return HB_FAILURE;
		}
		if ((pDevNode->arrDevIp == NULL) || (strlen(pDevNode->arrDevIp) < 7))//ip长度至少为7位(0.0.0.0)
		{
			//设备不存在在本地数据库
			free(pDevNode);
			pDevNode = NULL;
			pthread_rwlock_unlock(&rwlockMyLock);
			TRACE_ERR("The device [%s] do not in the database!\n", arrDevId);
			return HB_FAILURE;
		}

		strncpy(pDevNode->pDevId, arrDevId, sizeof(pDevNode->pDevId));
		pDevNode->iDevChnl = iDevChnl;
		pDevNode->iDevStreamType = iDevStreamType;

		TRACE_GREEN("dev_id:[%s], dev_chnl:[%d], dev_streadm:[%d], dev_ip:[%s], dev_port:[%d]\n", \
				pDevNode->pDevId, pDevNode->iDevChnl, pDevNode->iDevStreamType, pDevNode->arrDevIp, \
				pDevNode->iDevRtspPort);

		list_append(&listDevList, pDevNode);
		pthread_rwlock_unlock(&rwlockMyLock);
		connect_to_rtsp_server(arrServerIp, iServerPort, pDevNode);
	}
	else
	{
		//设备已存在，判断设备连接状态
		pthread_rwlock_unlock(&rwlockMyLock);
		connect_to_rtsp_server(arrServerIp, iServerPort, pDevNode);
	}

	return HB_SUCCESS;
}


/*
 *	Function: 收到停止推流命令
 *
 *	@param pCmdBuf: [IN]命令字符串（json格式）
 *	@param pClientBev: [IN]客户端连接过来的事件句柄
 *
 *	Retrun: 成功返回0，失败返回-1
 */
static HB_S32 stop_stream(HB_CHAR *pCmdBuf, struct bufferevent *pClientBev)
{
	HB_S32 iDevChnl = -1;		//设备通道号
	HB_S32 iDevStreamType = -1;//主子码流 0主码流，1子码流
	HB_CHAR arrServerIp[16] = {0}; //数据需要推送到的服务器ip
	HB_S32 iServerPort = 0; //数据需要推送到的服务器端口
	HB_CHAR arrProtocol[8] = {0};
	HB_CHAR arrDevId[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR arrDevIdTmp[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR arrDevIdDecode[MAX_DEV_ID_LEN] = {0}; //设备ID
	BOX_CTRL_CMD_OBJ st_MsgHead;
	HB_CHAR arrcMsgBody[128] = {0};
	HB_CHAR arrcSendBuf[256] = {0};

	cJSON *pRoot;
	pRoot = cJSON_Parse(pCmdBuf);
	cJSON *pProtocol = cJSON_GetObjectItem(pRoot, "protocol");
	strncpy(arrProtocol, pProtocol->valuestring, sizeof(arrProtocol)); //rtsp或hls
	cJSON *pDevId = cJSON_GetObjectItem(pRoot, "devId");
	strncpy(arrDevIdTmp, pDevId->valuestring, sizeof(arrDevIdTmp));
	cJSON *pDevChnl = cJSON_GetObjectItem(pRoot, "devChnl");
	iDevChnl = pDevChnl->valueint;
	cJSON *pStreamType = cJSON_GetObjectItem(pRoot, "streamType");
	iDevStreamType = pStreamType->valueint;

	url_decode(arrDevIdTmp, strlen(arrDevIdTmp), arrDevIdDecode, MAX_DEV_ID_LEN);
	TRACE_YELLOW("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", arrDevIdDecode, iDevChnl, iDevStreamType);
	strncpy(arrDevId, arrDevIdDecode+16, MAX_DEV_ID_LEN);

	memset(&st_MsgHead, 0, sizeof(BOX_CTRL_CMD_OBJ));
	strncpy(st_MsgHead.header, "hBzHbox@", sizeof(st_MsgHead.header));
	//返回接收命令成功信令
	snprintf(arrcMsgBody, sizeof(arrcMsgBody), \
		"{\"cmdType\":\"stop\",\"serverIp\":\"%s\",\"serverPort\":%d,\"devId\":\"%s\",\"devChnl\":%d,\"streamType\":%d}", \
		arrServerIp, iServerPort, arrDevId, iDevChnl, iDevStreamType);
//	st_MsgHead.cmd_length = htonl(strlen(arrcMsgBody));
	st_MsgHead.cmd_length = strlen(arrcMsgBody);
	memcpy(arrcSendBuf, &st_MsgHead, sizeof(st_MsgHead));
	strncpy(arrcSendBuf+sizeof(st_MsgHead), arrcMsgBody, sizeof(arrcSendBuf)-sizeof(st_MsgHead));

	struct sockaddr_in stServeraddr;
	HB_S32 iServeraddrLen;
	struct bufferevent *pSendMsgToPushClientEvent;//主动连接服务器事件

	pSendMsgToPushClientEvent = bufferevent_socket_new(pEventBase, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);
	bzero(&stServeraddr, sizeof(stServeraddr));
	stServeraddr.sin_family = AF_INET;
	stServeraddr.sin_port = htons(8103);
	inet_pton(AF_INET, "127.0.0.1", &stServeraddr.sin_addr);
	iServeraddrLen = sizeof(struct sockaddr_in);

	bufferevent_setcb(pSendMsgToPushClientEvent, NULL, NULL, send_msg_to_push_client, (HB_VOID *)arrcSendBuf);
	bufferevent_socket_connect(pSendMsgToPushClientEvent, (struct sockaddr*)&stServeraddr, iServeraddrLen);


	return HB_SUCCESS;
}


//extern char cServerIp[16];
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
	HB_CHAR *pPos = NULL;
	BOX_CTRL_CMD_OBJ stMsgHead;
	HB_CHAR arrcProtocol[8] = {0};
	HB_CHAR arrcCmdType[32] = {0};
	HB_CHAR arrcRetBuf[128] = {0};
	HB_CHAR arrcMsgBody[32] = {0};
	HB_CHAR arrcRecvCmdBuf[CMD_MAX_LEN] = {0};

	memset(&stMsgHead, 0, sizeof(BOX_CTRL_CMD_OBJ));

	struct evbuffer *src = bufferevent_get_input(pClientBev);//获取输入缓冲区
	HB_S32 len = evbuffer_get_length(src);//获取输入缓冲区中数据的长度，也就是可以读取的长度。

	if(len < sizeof(BOX_CTRL_CMD_OBJ))
	{
		printf("data small than head len! continue ...\n");
		return;
	}

	evbuffer_copyout(src, (void*)(&stMsgHead), sizeof(BOX_CTRL_CMD_OBJ));
	if (strncmp(stMsgHead.header, "hBzHbox@", strlen("hBzHbox@")) != 0)
	{
		//头消息错误，直接关闭返回
		TRACE_ERR("st_MsgHead.header error\n");
		bufferevent_free(pClientBev);
		return ;
	}

	if(evbuffer_get_length(src) < (ntohl(stMsgHead.cmd_length) + sizeof(BOX_CTRL_CMD_OBJ)))
	{
		printf("\n2222222222recv len=%d   msg_len=%d\n", (HB_S32)evbuffer_get_length(src), (HB_S32)(ntohl(stMsgHead.cmd_length)));
		return;
	}
	bufferevent_read(pClientBev, (HB_VOID*)(arrcRecvCmdBuf), (ntohl(stMsgHead.cmd_length) + sizeof(BOX_CTRL_CMD_OBJ)));
	bufferevent_disable(pClientBev, EV_READ);

	pPos = arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ);
	cJSON *pRoot;
	pRoot = cJSON_Parse(pPos);
	cJSON *pProtocol = cJSON_GetObjectItem(pRoot, "protocol");
	if (pProtocol != NULL)
	{
		strncpy(arrcProtocol, pProtocol->valuestring, sizeof(arrcProtocol)); //rtsp或hls
	}

	cJSON *pCmdType = cJSON_GetObjectItem(pRoot, "cmdType");
	if (pCmdType != NULL)
	{
		strncpy(arrcCmdType, pCmdType->valuestring, sizeof(arrcCmdType)); //play或stop
		printf("arrcCmdType:[%s]\n", arrcCmdType);
	}

	if (!strncmp(arrcCmdType, "open_video", 10))
	{
		TRACE_GREEN("Recv Cmd1 : [%s]\n", arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
		//收到rtsp打开请求
		deal_open_video_cmd(arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ), pClientBev);
	}
	else if (!strncmp(arrcCmdType, "server_info", 11))
	{
		TRACE_GREEN("Recv Cmd2 : [%s]\n", arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
		HB_CHAR arrServerIp[16] = {0};
		HB_S32	iServerPort = -1;

		cJSON *pServerIp = cJSON_GetObjectItem(pRoot, "serverIp");
		if (pServerIp != NULL)
			strncpy(arrServerIp, pServerIp->valuestring, sizeof(arrServerIp));
		cJSON *pServerPort = cJSON_GetObjectItem(pRoot, "serverPort");
		if (NULL != pServerPort)
			iServerPort = pServerPort->valueint;
		//拿到了rtsp服务器的ip和端口，需要将视频流发到这里
		//创建线程并发送数据
		bufferevent_free(pClientBev);
		pClientBev = NULL;

#ifdef HAND_SERVER_IP
		printf("cServerIp:[%s], iServerPort=%d\n", cServerIp, iServerPort);
		connect_to_rtsp_server(cServerIp, iServerPort, (DEV_LIST_HANDLE)arg);
#else
		printf("cServerIp:[%s], iServerPort=%d\n", arrServerIp, iServerPort);
		connect_to_rtsp_server(arrServerIp, iServerPort, (DEV_LIST_HANDLE)arg);
#endif
	}
	else
	{
		//返回接收命令成功信令
		strncpy(arrcMsgBody, "{\"code\":0}", sizeof(arrcMsgBody));
		stMsgHead.cmd_length = htonl(strlen(arrcMsgBody));
		memcpy(arrcRetBuf, &stMsgHead, sizeof(stMsgHead));
		strncpy(arrcRetBuf+sizeof(stMsgHead), arrcMsgBody, sizeof(arrcRetBuf)-sizeof(stMsgHead));
		bufferevent_write(pClientBev, arrcRetBuf, sizeof(arrcMsgBody)+sizeof(stMsgHead));
		bufferevent_setcb(pClientBev, deal_write_ok, NULL, NULL, NULL);
		bufferevent_enable(pClientBev, EV_WRITE);

		if (strncmp(arrcProtocol, "rtsp", 4) == 0)
		{
			struct sockaddr_in stServeraddr;
			HB_S32 iServeraddrLen;
			struct bufferevent *pSendMsgToPushClientEvent;//主动连接服务器事件

			pSendMsgToPushClientEvent = bufferevent_socket_new(pEventBase, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);
			bzero(&stServeraddr, sizeof(stServeraddr));
			stServeraddr.sin_family = AF_INET;
			stServeraddr.sin_port = htons(8103);
			inet_pton(AF_INET, "127.0.0.1", &stServeraddr.sin_addr);
			iServeraddrLen = sizeof(struct sockaddr_in);

			bufferevent_setcb(pSendMsgToPushClientEvent, NULL, NULL, send_msg_to_push_client, (HB_VOID *)arrcRecvCmdBuf);
			bufferevent_socket_connect(pSendMsgToPushClientEvent, (struct sockaddr*)&stServeraddr, iServeraddrLen);
		}
		else if (strncmp(arrcProtocol, "hls", 3) == 0)
		{
			if (strncmp(arrcCmdType, "play", 4) == 0)
			{
				//收到推送视频流信令
				TRACE_GREEN("Recv Cmd : [%s]\n", arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
				push_stream(arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ), pClientBev);
			}
			else if (strncmp(arrcCmdType, "stop", 4) == 0)
			{
				//收到关闭视频流信令
				TRACE_GREEN("Recv Cmd : [%s]\n", arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
				stop_stream(arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ), pClientBev);
			}
		}
		else
		{
			TRACE_ERR("Recv ERR Cmd : [%s]\n", arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
			//拿到了rtsp服务器的ip和端口，需要将视频流发到这里
			//创建线程并发送数据
			bufferevent_free(pClientBev);
			pClientBev = NULL;
		}
	}

#if 0
	if(evbuffer_remove(src, &stMsgHead, sizeof(BOX_CTRL_CMD_OBJ)))
	{
		if (evbuffer_remove(src, arrcRecvCmdBuf, CMD_MAX_LEN) > 0)
		{
			if (strstr(arrc_RecvCmdBuf, "open_video") != NULL)
			{
				TRACE_GREEN("Recv Cmd1 : [%s]\n", arrcRecvCmdBuf);
				//收到rtsp打开请求
				deal_open_video_cmd(arrcRecvCmdBuf, pClientBev);
			}
			else if (strstr(arrcRecvCmdBuf, "server_info") != NULL)
			{
				TRACE_GREEN("Recv Cmd2 : [%s]\n", arrcRecvCmdBuf);
				//拿到了rtsp服务器的ip和端口，需要将视频流发到这里
				//创建线程并发送数据
				bufferevent_free(pClientBev);
				pClientBev = NULL;
				connect_to_rtsp_server(arrcRecvCmdBuf, (DEV_LIST_HANDLE)arg);
			}
			else if(strstr(arrcRecvCmdBuf, "play") != NULL)
			{
				HB_CHAR arrcMsgBody[32] = {0};
				HB_CHAR arrcRetBuf[128] = {0};
//				bufferevent_read(pClientBev, (HB_VOID*)(arrc_RecvCmdBuf), (ntohl(st_MsgHead.cmd_length) + sizeof(BOX_CTRL_CMD_OBJ)));

				bufferevent_disable(pClientBev, EV_READ);

				//返回接收命令成功信令
				strncpy(arrcMsgBody, "{\"code\":0}", sizeof(arrcMsgBody));
				st_MsgHead.cmd_length = htonl(strlen(arrcMsgBody));
				memcpy(arrcRetBuf, &st_MsgHead, sizeof(stMsgHead));
				strncpy(arrcRetBuf+sizeof(stMsgHead), arrcMsgBody, sizeof(arrcRetBuf)-sizeof(stMsgHead));
				bufferevent_write(pClientBev, arrcRetBuf, sizeof(arrcMsgBody)+sizeof(st_MsgHead));
				bufferevent_setcb(pClientBev, deal_write_ok, NULL, NULL, NULL);
				bufferevent_enable(pClientBev, EV_WRITE);
				TRACE_GREEN("Recv Cmd : [%s]\n", arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
				push_stream(arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ), pClientBev);
			}
			else if(strstr(arrcRecvCmdBuf, "stop") != NULL)
			{
				HB_CHAR arrcMsgBody[32] = {0};
				HB_CHAR arrcRetBuf[128] = {0};
//				bufferevent_read(pClientBev, (HB_VOID*)(arrc_RecvCmdBuf), (ntohl(st_MsgHead.cmd_length) + sizeof(BOX_CTRL_CMD_OBJ)));

				bufferevent_disable(pClientBev, EV_READ);

				//返回接收命令成功信令
				strncpy(arrcMsgBody, "{\"code\":0}", sizeof(arrcMsgBody));
				stMsgHead.cmd_length = htonl(strlen(arrcMsgBody));
				memcpy(arrcRetBuf, &st_MsgHead, sizeof(stMsgHead));
				strncpy(arrcRetBuf+sizeof(stMsgHead), arrcMsgBody, sizeof(arrcRetBuf)-sizeof(stMsgHead));
				bufferevent_write(pClientBev, arrcRetBuf, sizeof(arrcMsgBody)+sizeof(stMsgHead));
				bufferevent_setcb(pClientBev, deal_write_ok, NULL, NULL, NULL);
				bufferevent_enable(pClientBev, EV_WRITE);
				TRACE_GREEN("Recv Cmd : [%s]\n", arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
				stop_stream(arrcRecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ), pClientBev);
			}
		}
	}
#endif
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
	struct timeval tv_read;

#if 1
	//打印对端ip
	struct sockaddr_in *p_PeerAddr = (struct sockaddr_in *)pClientAddr;
	char arrch_PeerIp[16] = {0};
	inet_ntop(AF_INET, &(p_PeerAddr->sin_addr), arrch_PeerIp, sizeof(arrch_PeerIp));
	TRACE_YELLOW("\nA new Client[%s]:[%d] connect !\n", arrch_PeerIp, ntohs(p_PeerAddr->sin_port));
#endif

    // 为新的连接分配并设置 bufferevent,设置BEV_OPT_CLOSE_ON_FREE宏后，当连接断开时也会通知客户端关闭套接字
    struct bufferevent *accept_sockfd_bev = bufferevent_socket_new(pEventBase, iAcceptSockfd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
    //设置低水位，当数据长度大于消息头时才读取数据
    bufferevent_setwatermark(accept_sockfd_bev, EV_READ, sizeof(BOX_CTRL_CMD_OBJ)+1, 0);
    //设置超时，5秒内未收到对端发来数据则断开连接
    tv_read.tv_sec  = 10;
    tv_read.tv_usec = 0;
	//注意，在盒子连接设备处也设置了超时，此处超时需大于盒子与设备连接时的超时，当前盒子与设备连接超时时间为5s
	bufferevent_set_timeouts(accept_sockfd_bev, &tv_read, NULL);
    bufferevent_setcb(accept_sockfd_bev, deal_client_cmd, NULL, deal_client_cmd_error_cb1, NULL);
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


	//LEV_OPT_CLOSE_ON_EXEC 用于为套接字设置close-on-exec标志，
	//这个标志符的具体作用在于当开辟其他进程调用exec（）族函数时，在调用exec函数之前为exec族函数释放对应的文件描述符。
	//绑定端口并接收连接
	pListener = evconnlistener_new_bind(pEventBase, accept_client_connect_cb, NULL,
			LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE|LEV_OPT_THREADSAFE,
			-1, (struct sockaddr*)&stListenAddr, sizeof(struct sockaddr_in));
	event_base_dispatch(pEventBase);
	evconnlistener_free(pListener);
	event_base_free(pEventBase);
}
