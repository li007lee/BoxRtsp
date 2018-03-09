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
#include "url_code.h"

struct event_base *pEventBase;


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

/****************************************数据库操作****************************************/
/****************************************数据库操作****************************************/
/****************************************数据库操作****************************************/

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
	CLIENT_LIST_HEAD_HANDLE pRtspClientHead = &(pMessengerArgsDev->pDevNode->stRtspClientHead);

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

	if (pRtspClientHead->iStartThreadFlag == 1) //如果发送线程已经启动，此处只置位
	{
		bufferevent_disable(pClientNode->pSendVideoToServerEvent, EV_READ|EV_WRITE);
		pClientNode->iDelFlag = 1;
	}
	else //若没有启动，从此处释放
	{
		pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
		bufferevent_disable(pClientNode->pSendVideoToServerEvent, EV_READ|EV_WRITE);
		if (pMessengerArgsDev != NULL)
		{
			if (pClientNode != NULL)
			{
//				printf("del one dev from dev_list!\n");
				del_one_client(pRtspClientHead, pClientNode);
				pMessengerArgsDev->pClientNode = NULL;
			}

			if ((pMessengerArgsDev->pDevNode != NULL) && (pMessengerArgsDev->pDevNode->stRtspClientHead.iClientNum < 1))
			{
				//当前设备下如果已经没有用户了，那么需要把设备从设备链表摘除
//				printf("del one dev from dev_list!\n");
				del_one_from_dev_list(pMessengerArgsDev->pDevNode);
				pMessengerArgsDev->pDevNode = NULL;
			}
			TRACE_ERR("send_rtsp_to_server_event_error_cb free pMessengerArgs\n");
			free(pMessengerArgsDev);
			pMessengerArgsDev = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
		}
	}
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
	CLIENT_LIST_HEAD_HANDLE pRtspClientHead = &(pDevNode->stRtspClientHead);

	if (what & BEV_EVENT_CONNECTED)//盒子主动连接rtsp服务器成功
	{
		TRACE_GREEN("Connect to rtsp server succeed！！！！！！！！！！！\n");
		//连接成功创建客户节点
		CLIENT_LIST_HANDLE pClientNode = (CLIENT_LIST_HANDLE)malloc(sizeof(CLIENT_LIST_OBJ));
		memset(pClientNode, 0, sizeof(CLIENT_LIST_OBJ));
		pClientNode->pSendVideoToServerEvent = pConnectRtspServerBev;

		pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
		//插入链表
		add_client_in_tail(pRtspClientHead, pClientNode);
		//启动读取视频流线程
		if (pRtspClientHead->iStartThreadFlag == 0)
		{
			//如果当前设备还没有启动推流线程，则启动
			pthread_create(&(pRtspClientHead->threadReadVideoId), NULL, read_video_data_from_dev_task, (HB_VOID*)pDevNode);
		}
		LIBEVENT_ARGS_DEV_HANDLE pMessengerArgsDev = (LIBEVENT_ARGS_DEV_HANDLE)malloc(sizeof(LIBEVENT_ARGS_DEV_OBJ));
		memset(pMessengerArgsDev, 0, sizeof(LIBEVENT_ARGS_DEV_OBJ));
		pMessengerArgsDev->pDevNode = pDevNode;
		pMessengerArgsDev->pClientNode = pClientNode;

		struct timeval tv_w;
	    //设置超时，10秒内未收到对端发来数据则断开连接
	    tv_w.tv_sec  = 10;
	    tv_w.tv_usec = 0;
		//设置数据发送的写超时
		bufferevent_set_timeouts(pConnectRtspServerBev, NULL, &tv_w);
		//设置连接出错后的回调函数
		bufferevent_setcb(pConnectRtspServerBev, NULL, NULL, send_rtsp_to_server_event_error_cb, (HB_VOID *)pMessengerArgsDev);
		bufferevent_enable(pConnectRtspServerBev, EV_WRITE);
		pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
	}
	else  //盒子connect rtsp服务器失败
	{
		TRACE_ERR("\n###########  connect rtsp server failed !\n");
		bufferevent_disable(pConnectRtspServerBev, EV_READ|EV_WRITE);

		if (pRtspClientHead->iClientNum < 1)
		{
			//连接rtsp服务器失败，如果当前用户数为0，那么就将设备从链表中删除
			pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
			del_one_from_dev_list(pDevNode);
			pDevNode = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
		}

		bufferevent_free(pConnectRtspServerBev);
		pConnectRtspServerBev = NULL;
	}
}


/*
 *	Function: 连接rtsp服务器
 *
 *	@param pServerIp: [IN]服务器地址
 *	@param pServerIp: [IN]服务器端口
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


//	printf("deal_client_cmd_error_cb1 free bev!\n");
	if (bev != NULL)
	{
		bufferevent_free(bev);
		bev = NULL;
	}
//	if (pMessengerArgs != NULL)
//	{
//		if (pMessengerArgs->pDevNode != NULL)
//		{
//			printf("del one dev from dev_list!\n");
//			pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
//			del_one_from_dev_list(pMessengerArgs->pDevNode);
//			pMessengerArgs->pDevNode = NULL;
//			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
//		}
//		if (pMessengerArgs->pClientBev != NULL)
//		{
//			printf("bufferevent_free pClientBev!\n");
//			bufferevent_free(pMessengerArgs->pClientBev);
//			pMessengerArgs->pClientBev = NULL;
//		}
//		printf("deal_client_cmd_error_cb1 free pMessengerArgs\n");
//		free(pMessengerArgs);
//		pMessengerArgs = NULL;
//	}
//	else
//	{
//		printf("deal_client_cmd_error_cb1 free bev!\n");
//		//伪代码
//		bufferevent_free(bev);
//	}

}


//根据网卡eth获取相应到mac地址
static HB_S32 get_mac_dev(HB_CHAR *mac_sn, HB_CHAR *dev)
{
    struct ifreq tmp;
    HB_S32 sock_mac;
   // HB_CHAR *tmpflag;
    //HB_CHAR mac_addr[30];
    sock_mac = socket(AF_INET, SOCK_STREAM, 0);
    if( sock_mac == -1)
    {
        return -1;
    }
    memset(&tmp,0,sizeof(tmp));
    strncpy(tmp.ifr_name, dev, sizeof(tmp.ifr_name)-1 );
    if( (ioctl( sock_mac, SIOCGIFHWADDR, &tmp)) < 0 )
    {
    	close(sock_mac);
        return -1;
    }

    close(sock_mac);
    memcpy(mac_sn, tmp.ifr_hwaddr.sa_data, 6);
    return 0;
}


//获取盒子到序列号
HB_S32 get_sys_sn(HB_CHAR *sn, HB_S32 sn_size)
{
	HB_U64 sn_num = 0;
	HB_CHAR sn_mac[32] = {0};
	HB_CHAR mac[32] = {0};
	get_mac_dev(mac, ETH_X);
	sprintf(sn_mac, "0x%02x%02x%02x%02x%02x%02x",
			(HB_U8)mac[0],
			(HB_U8)mac[1],
			(HB_U8)mac[2],
			(HB_U8)mac[3],
			(HB_U8)mac[4],
			(HB_U8)mac[5]);
	sn_num = strtoull(sn_mac, 0, 16);
	snprintf(sn, sn_size, "%llu", sn_num);

	return 0;
}



/***************************处理push_stream消息****************************/
/***************************处理push_stream消息****************************/
/***************************处理push_stream消息****************************/
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
	HB_CHAR accDevId[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR accDevIdTmp[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR accDevIdDecode[MAX_DEV_ID_LEN] = {0}; //设备ID
	HB_CHAR cMacSn[32] = {0};

	cJSON *pRoot;
	pRoot = cJSON_Parse(pCmdBuf);
	cJSON *pServerIp = cJSON_GetObjectItem(pRoot, "ServerIp");
	strncpy(arrServerIp, pServerIp->valuestring, sizeof(arrServerIp));
	cJSON *pServerPort = cJSON_GetObjectItem(pRoot, "ServerPort");
	iServerPort = pServerPort->valueint;
	cJSON *pDevId = cJSON_GetObjectItem(pRoot, "DevId");
	strncpy(accDevIdTmp, pDevId->valuestring, sizeof(accDevIdTmp));
	cJSON *pDevChnl = cJSON_GetObjectItem(pRoot, "DevChnl");
	iDevChnl = pDevChnl->valueint;
	cJSON *pStreamType = cJSON_GetObjectItem(pRoot, "StreamType");
	iDevStreamType = pStreamType->valueint;

	url_decode(accDevIdTmp, strlen(accDevIdTmp), accDevIdDecode, MAX_DEV_ID_LEN);
	TRACE_YELLOW("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", accDevIdDecode, iDevChnl, iDevStreamType);

	pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
	pDevNode = find_in_dev_list(accDevIdDecode+16, iDevChnl, iDevStreamType);
	if (pDevNode == NULL)
	{
		//未找到设备
		HB_CHAR sql[1024] = {0};
		pDevNode = (DEV_LIST_HANDLE)malloc(sizeof(DEV_LIST_OBJ));
		memset(pDevNode, 0, sizeof(DEV_LIST_OBJ));

		get_sys_sn(cMacSn, sizeof(cMacSn));
//		printf("cMacSn:[%s]\n", cMacSn);
//		strncpy(accDevId, accDevIdDecode+strlen(cMacSn)+1, MAX_DEV_ID_LEN);
		strncpy(accDevId, accDevIdDecode+16, MAX_DEV_ID_LEN);
		TRACE_YELLOW("devid=[%s] dev_chnl=[%d] dev_stream_type=[%d]\n", accDevId, iDevChnl, iDevStreamType);
		snprintf(sql, sizeof(sql), \
			"select dev_ip,rtsp_port,dev_login_usr,dev_login_passwd,basic_authenticate,rtsp_list.rtsp_main,rtsp_list.rtsp_sub from onvif_dev_data left join rtsp_list on onvif_dev_data.dev_id='%s' where rtsp_list.[dev_id]=onvif_dev_data.dev_id and rtsp_list.[dev_chnl_num]='%d'", \
			accDevId, iDevChnl-1);
		if (sql_operation(sql, DEV_DATA_BASE_NAME, load_device_ip_port_cb, (HB_VOID *)pDevNode) < 0)
		{
			//数据库操作失败
			free(pDevNode);
			pDevNode = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			TRACE_ERR("data_base[%s] exec[%s] failed!\n", DEV_DATA_BASE_NAME, sql);
			return HB_FAILURE;
		}
		if ((pDevNode->arrDevIp == NULL) || (strlen(pDevNode->arrDevIp) < 7))//ip长度至少为7位(0.0.0.0)
		{
			//设备不存在在本地数据库
			free(pDevNode);
			pDevNode = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			TRACE_ERR("The device [%s] do not in the database!\n", accDevId);
			return HB_FAILURE;
		}

		strncpy(pDevNode->pDevId, accDevId, sizeof(pDevNode->pDevId));
		pDevNode->iDevChnl = iDevChnl;
		pDevNode->iDevStreamType = iDevStreamType;

		TRACE_GREEN("dev_id:[%s], dev_chnl:[%d], dev_streadm:[%d], dev_ip:[%s], dev_port:[%d]\n", \
				pDevNode->pDevId, pDevNode->iDevChnl, pDevNode->iDevStreamType, pDevNode->arrDevIp, \
				pDevNode->iDevRtspPort);

		add_to_dev_list(pDevNode);
		pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
//		bufferevent_free(pClientBev);
//		pClientBev = NULL;
		connect_to_rtsp_server(arrServerIp, iServerPort, pDevNode);
	}
	else
	{
		//设备已存在，判断设备连接状态
		connect_to_rtsp_server(arrServerIp, iServerPort, pDevNode);
		pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
	}

	return HB_SUCCESS;
}

/***************************处理push_stream消息End****************************/
/***************************处理push_stream消息End****************************/
/***************************处理push_stream消息End****************************/




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
	HB_CHAR arrcRetBuf[128] = {0};
	HB_CHAR arrcMsgBody[32] = {0};
	HB_CHAR arrc_RecvCmdBuf[CMD_MAX_LEN] = {0};

	memset(&st_MsgHead, 0, sizeof(BOX_CTRL_CMD_OBJ));

	struct evbuffer *src = bufferevent_get_input(pClientBev);//获取输入缓冲区
	HB_S32 len = evbuffer_get_length(src);//获取输入缓冲区中数据的长度，也就是可以读取的长度。

	if(len < sizeof(BOX_CTRL_CMD_OBJ))
	{
		printf("data small than head len! continue ...\n");
		return;
	}

	evbuffer_copyout(src, (void*)(&st_MsgHead), sizeof(BOX_CTRL_CMD_OBJ));
	if (strncmp(st_MsgHead.header, "hBzHbox@", strlen("hBzHbox@")) != 0)
	{
		//头消息错误，直接关闭返回
		TRACE_ERR("st_MsgHead.header error\n");
		bufferevent_free(pClientBev);
		return ;
	}

	if(evbuffer_get_length(src) < (ntohl(st_MsgHead.cmd_length) + sizeof(BOX_CTRL_CMD_OBJ)))
	{
		printf("\n2222222222recv len=%d   msg_len=%d\n", evbuffer_get_length(src), (HB_S32)(ntohl(st_MsgHead.cmd_length)));
//		struct timeval tv_read;
//	    tv_read.tv_sec  = 5;//10秒超时时间,接收到新的连接后，10秒内没收到任何数据，则用accept_client_event_cb回调函数处理
//	    tv_read.tv_usec = 0;
//		bufferevent_set_timeouts(buf_bev, &tv_read, NULL);
//		bufferevent_setcb(buf_bev, accept_client_read_cb, NULL, accept_client_event_cb, NULL);
//		bufferevent_enable(buf_bev, EV_READ);
		return;
	}


//	if (evbuffer_remove(src, arrc_RecvCmdBuf, CMD_MAX_LEN) > 0)
	bufferevent_read(pClientBev, (HB_VOID*)(arrc_RecvCmdBuf), (ntohl(st_MsgHead.cmd_length) + sizeof(BOX_CTRL_CMD_OBJ)));

	bufferevent_disable(pClientBev, EV_READ);

	//返回接收命令成功信令
	strncpy(arrcMsgBody, "{\"code\":0}", sizeof(arrcMsgBody));
	st_MsgHead.cmd_length = htonl(strlen(arrcMsgBody));
	memcpy(arrcRetBuf, &st_MsgHead, sizeof(st_MsgHead));
	strncpy(arrcRetBuf+sizeof(st_MsgHead), arrcMsgBody, sizeof(arrcRetBuf)-sizeof(st_MsgHead));
	bufferevent_write(pClientBev, arrcRetBuf, sizeof(arrcMsgBody)+sizeof(st_MsgHead));
	bufferevent_setcb(pClientBev, deal_write_ok, NULL, NULL, NULL);
	bufferevent_enable(pClientBev, EV_WRITE);
//	bufferevent_free(pClientBev);
//	pClientBev = NULL;

	//解析命令
	if (strstr(arrc_RecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ), "push_stream") != NULL)
	{
		//收到推送视频流信令
		TRACE_GREEN("Recv Cmd : [%s]\n", arrc_RecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
		push_stream(arrc_RecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ), pClientBev);
	}
	else
	{
		TRACE_ERR("Recv ERR Cmd : [%s]\n", arrc_RecvCmdBuf+sizeof(BOX_CTRL_CMD_OBJ));
		//拿到了rtsp服务器的ip和端口，需要将视频流发到这里
		//创建线程并发送数据
//		bufferevent_free(pClientBev);
//		pClientBev = NULL;
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
			1024, (struct sockaddr*)&stListenAddr, sizeof(struct sockaddr_in));
	event_base_dispatch(pEventBase);
	evconnlistener_free(pListener);
	event_base_free(pEventBase);
}
