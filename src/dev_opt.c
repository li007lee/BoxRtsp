/*
 * dev_opt.c
 *
 *  Created on: 2017年9月25日
 *      Author: lijian
 */

#include "libavutil/error.h"
#include "dev_opt.h"
#include "dev_list.h"
#include "rtsp-parser.h"


#define SEND_OPTIONS_MSG 		"OPTIONS %s RTSP/1.0\r\n"\
									"CSeq: %u\r\n"\
									"User-Agent: LibVLC/2.2.6 (LIVE555 Streaming Media v2016.02.22)\r\n\r\n"

#define SEND_DESCRIBE_DIGEST_MSG 		"DESCRIBE %s RTSP/1.0\r\n"\
												"CSeq: 4\r\n"\
												"Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n"\
												"User-Agent: LibVLC/2.2.6 (LIVE555 Streaming Media v2016.02.22)\r\n"\
												"Accept: application/sdp\r\n\r\n"

//根据网卡eth获取相应到mac地址
static HB_S32 get_mac_dev(HB_CHAR *mac_sn, HB_CHAR *dev)
{
	struct ifreq tmp;
	HB_S32 sock_mac;
	// HB_CHAR *tmpflag;
	//HB_CHAR mac_addr[30];
	sock_mac = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_mac == -1)
	{
		return -1;
	}
	memset(&tmp, 0, sizeof(tmp));
	strncpy(tmp.ifr_name, dev, sizeof(tmp.ifr_name) - 1);
	if ((ioctl(sock_mac, SIOCGIFHWADDR, &tmp)) < 0)
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
	HB_CHAR sn_mac[32] = { 0 };
	HB_CHAR mac[32] = { 0 };
	get_mac_dev(mac, ETH_X);
	sprintf(sn_mac, "0x%02x%02x%02x%02x%02x%02x", (HB_U8) mac[0], (HB_U8) mac[1], (HB_U8) mac[2], (HB_U8) mac[3], (HB_U8) mac[4], (HB_U8) mac[5]);
	sn_num = strtoull(sn_mac, 0, 16);
	snprintf(sn, sn_size, "%llu", sn_num);

	return 0;
}

/***********************测试设备连通性***********************/
/***********************测试设备连通性***********************/
/***********************测试设备连通性***********************/

///////////////////////////////////////////////
//	Function: 解析设备返回的sdp信息
//
//	@param p_RecvBuf: [IN]设备返回的sdp信息buffer
//	@param p_DevNode: [OUT]设备信息结构体，解析sdp后填充此结构体
//
//	Retrun: 无
///////////////////////////////////////////////
static void analysis_sdp_info(HB_CHAR *p_SdpBuf, DEV_LIST_HANDLE p_DevNode)
{
	HB_CHAR *p_Pos1 = p_SdpBuf;
	HB_CHAR *p_Pos2 = p_SdpBuf;

	//sdp视频数据
	p_Pos1 = strstr(p_SdpBuf, "m=video");
	if (p_Pos1 != NULL)
	{
		p_Pos2 = strstr(p_Pos1, "\r\n");
		strncpy(p_DevNode->m_video, p_Pos1, p_Pos2 - p_Pos1);
		printf("get m_video:[%s]\n", p_DevNode->m_video);
		p_Pos1 = strstr(p_Pos1, "a=rtpmap");
		if (p_Pos1 != NULL)
		{
			p_Pos2 = strstr(p_Pos1, "\r\n");
			strncpy(p_DevNode->a_rtpmap_video, p_Pos1, p_Pos2 - p_Pos1);
			p_Pos2 = strstr(p_Pos1, "/") + 1;
			p_DevNode->iVideoFrameRate = atoi(p_Pos2);

			printf("get a_rtpmap_video:[%s], FrameRate=[%d]\n", p_DevNode->a_rtpmap_video, p_DevNode->iVideoFrameRate);
		}

		p_Pos1 = strstr(p_Pos1, "a=fmtp");
		if (p_Pos1 != NULL)
		{
			p_Pos2 = strstr(p_Pos1, "\r\n");
			strncpy(p_DevNode->a_fmtp_video, p_Pos1, p_Pos2 - p_Pos1);
			printf("get a_fmtp:[%s]\n", p_DevNode->a_fmtp_video);
		}
	}
	else
	{
		strncpy(p_DevNode->m_video, "NULL", sizeof(p_DevNode->m_video));
		strncpy(p_DevNode->a_rtpmap_video, "NULL", sizeof(p_DevNode->a_rtpmap_video));
		strncpy(p_DevNode->a_fmtp_video, "NULL", sizeof(p_DevNode->a_fmtp_video));
	}

	//sdp音频数据
	p_Pos1 = strstr(p_SdpBuf, "m=audio");
	if (p_Pos1 != NULL)
	{
		p_Pos2 = strstr(p_Pos1, "\r\n");
		strncpy(p_DevNode->m_audio, p_Pos1, p_Pos2 - p_Pos1);
		printf("get m_audio:[%s]\n", p_DevNode->m_audio);
		p_Pos1 = strstr(p_Pos1, "a=rtpmap_audio");
		if (p_Pos1 != NULL)
		{
			p_Pos2 = strstr(p_Pos1, "\r\n");
			strncpy(p_DevNode->a_rtpmap_audio, p_Pos1, p_Pos2 - p_Pos1);
			printf("get a_rtpmap_audio:[%s]\n", p_DevNode->a_rtpmap_audio);
		}
	}
	else
	{
		strncpy(p_DevNode->m_audio, "NULL", sizeof(p_DevNode->m_video));
		strncpy(p_DevNode->a_rtpmap_audio, "NULL", sizeof(p_DevNode->a_rtpmap_video));
	}
}

/*
 *	Function: 处理与客户端信令交互时产生的异常和错误(第二次信令交互)
 *
 *	@param bev: 异常产生的事件句柄
 *	@param events: 异常事件类型
 *  @parmm args	: 实际为DEV_LIST_HANDLE类型的参数结构体
 *
 *	Retrun: 无
 */
static HB_VOID deal_client_cmd_error_cb2(struct bufferevent *bev, short events, void *arg)
{
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE) arg;
	list_t *plistRtspClient = (list_t *) &(pDevNode->listRtspClient);
	list_t *plistWaitClient = (list_t *) &(pDevNode->listWaitClient);
	HB_S32 err = EVUTIL_SOCKET_ERROR();

	bufferevent_disable(bev, EV_READ | EV_WRITE);

	if (events & BEV_EVENT_EOF)    //对端关闭
	{
		TRACE_ERR("######## deal_client_cmd_error_cb2 BEV_EVENT_EOF(%d) : %s !", err, evutil_socket_error_to_string(err));
	}
	else if (events & BEV_EVENT_ERROR)    //错误事件
	{
		TRACE_ERR("######## deal_client_cmd_error_cb2 BEV_EVENT_ERROR(%d) : %s !", err, evutil_socket_error_to_string(err));
	}
	else if (events & BEV_EVENT_TIMEOUT)    //超时事件
	{
		TRACE_ERR("######## deal_client_cmd_error_cb2 BEV_EVENT_TIMEOUT(%d) : %s !", err, evutil_socket_error_to_string(err));
	}

	if ((pDevNode != NULL) && (list_size(plistRtspClient) < 1) && (list_size(plistWaitClient) < 1))
	{
		printf("del one dev from dev_list!\n");
		pthread_rwlock_wrlock(&rwlockMyLock);
		list_delete(&listDevList, (HB_VOID *) pDevNode);
		pDevNode = NULL;
		pthread_rwlock_unlock(&rwlockMyLock);
	}
	printf("deal_client_cmd_error_cb2 free bev!\n");
	bufferevent_free(bev);
	bev = NULL;
}


static int get_realm_and_nonce(HB_CHAR *msg_str, DEV_LIST_HANDLE pDevNode)
{
	HB_CHAR *start_pos = NULL;
	HB_CHAR *end_pos = NULL;
	start_pos = strstr(msg_str, "realm");
	if(start_pos != NULL)
	{
		start_pos = strchr(start_pos,'"');
		end_pos = strchr(start_pos+2, '"');
		memcpy(pDevNode->realm, start_pos+1, end_pos-start_pos-1);

		start_pos = strstr(msg_str, "nonce");
		if(start_pos != NULL)
		{
			start_pos = strchr(start_pos,'"');
			end_pos = strchr(start_pos+2, '"');
			memcpy(pDevNode->nonce, start_pos+1, end_pos-start_pos-1);
		}
	}
	else
	{
		return -1;
	}
	return 0;
}


static int get_response(HB_CHAR *response, HB_CHAR *method, DEV_LIST_HANDLE pDevNode)
{
	HB_CHAR msg_buf[512] = {0};
	HB_CHAR part1[64] = {0};
	HB_CHAR part3[64] = {0};
	snprintf(msg_buf, sizeof(msg_buf), "%s:%s:%s", pDevNode->arrUserName, pDevNode->realm, pDevNode->arrUserPasswd);
	md5_packages_string(part1, msg_buf, strlen(msg_buf));
	memset(msg_buf, 0, sizeof(msg_buf));
	if (pDevNode->iDevStreamType == 0)
	{
		//主码流
		snprintf(msg_buf, sizeof(msg_buf), "%s:%s", method, pDevNode->arrDevRtspMainUrl);
	}
	else
	{
		//子码流
		if ((pDevNode->arrDevRtspSubUrl == NULL) || (strlen(pDevNode->arrDevRtspSubUrl) == 0))
		{
			//没有子码流依然使用主码流
			snprintf(msg_buf, sizeof(msg_buf), "%s:%s", method, pDevNode->arrDevRtspMainUrl);
		}
		else
		{
			//有子码流
			snprintf(msg_buf, sizeof(msg_buf), "%s:%s", method, pDevNode->arrDevRtspSubUrl);
		}
	}

	md5_packages_string(part3, msg_buf, strlen(msg_buf));
	memset(msg_buf, 0, sizeof(msg_buf));
	snprintf(msg_buf, sizeof(msg_buf), "%s:%s:%s", part1, pDevNode->nonce, part3);
	md5_packages_string(response, msg_buf, strlen(msg_buf));
	return 0;
}

static HB_VOID active_connect_eventcb(struct bufferevent *connect_dev_bev, HB_S16 what, HB_VOID *arg)
{
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE) arg;
	list_t *plistWaitClient = (list_t *) &(pDevNode->listWaitClient);

	if (what & BEV_EVENT_CONNECTED)    //盒子主动connect设备成功
	{
#if 1
		//连接设备成功发送describe获取sdp信息
		HB_CHAR cDiscribe[1024] = { 0 };

		if (pDevNode->iDevStreamType == 0)
		{
			//主码流
			snprintf(cDiscribe, sizeof(cDiscribe),
							"DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\nAuthorization: Basic %s\r\n\r\n",
							pDevNode->arrDevRtspMainUrl, pDevNode->arrBasicAuthenticate);
			printf("DESCRIBE main 1: [%s]\n", cDiscribe);
		}
		else
		{
			//子码流
			if ((pDevNode->arrDevRtspSubUrl == NULL) || (strlen(pDevNode->arrDevRtspSubUrl) == 0))
			{
				//没有子码流依然使用主码流
				snprintf(cDiscribe, sizeof(cDiscribe),
								"DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\nAuthorization: Basic %s\r\n\r\n",
								pDevNode->arrDevRtspMainUrl, pDevNode->arrBasicAuthenticate);
				printf("DESCRIBE main 2: [%s]\n", cDiscribe);
			}
			else
			{
				//有子码流
				snprintf(cDiscribe, sizeof(cDiscribe),
								"DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\nAuthorization: Basic %s\r\n\r\n",
								pDevNode->arrDevRtspSubUrl, pDevNode->arrBasicAuthenticate);
				printf("DESCRIBE sub 1: [%s]\n", cDiscribe);
			}
		}

		TRACE_GREEN("\n############  connect dev successful and start to get dev SDP !\n");
		bufferevent_write(connect_dev_bev, cDiscribe, strlen(cDiscribe));
#endif
	}
	else
	{
		//盒子connect 设备失败,当数据库中有此设备，但此设备ip连接不上时会进到此接口
		TRACE_ERR("\n###########  box connect dev  failed !\n");
		pDevNode->enumDevConnectStatus = DISCONNECT;    //设置设备连接失败状态

		if (pDevNode != NULL)
		{
			printf("del one dev from dev_list!\n");
			pthread_rwlock_wrlock(&rwlockMyLock);
			list_destroy(plistWaitClient);
			list_delete(&listDevList, (HB_VOID *) pDevNode);
			pDevNode = NULL;
			pthread_rwlock_unlock(&rwlockMyLock);
		}
//		此连接事件由deal_client_cmd_error_cb1超时时进行释放
		printf("active_connect_eventcb free bev!\n");
		bufferevent_free(connect_dev_bev);
		connect_dev_bev = NULL;
	}
}



/*
 *	Function: 摘要认证回调函数
 *
 *	@param bev: 与设备的链接句柄
 *  @parmm args	: 实际为LIBEVENT_ARGS_HANDLE类型的参数结构体
 *
 *	Retrun: 无
 */
static void read_dev_sdp_digest_cb(struct bufferevent *connect_dev_bev, void *arg)
{
	HB_S32 i;
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE) arg;
	list_t *plistWaitClient = (list_t *) &(pDevNode->listWaitClient);
	BOX_CTRL_CMD_OBJ st_MsgHead;
	HB_CHAR arr_RecvBuf[10240] = { 0 };
	HB_CHAR arr_SendBuf[2048] = { 0 };

	bufferevent_read(connect_dev_bev, arr_RecvBuf, sizeof(arr_RecvBuf));
	bufferevent_free(connect_dev_bev);
	connect_dev_bev = NULL;

	analysis_sdp_info(arr_RecvBuf, pDevNode);

	printf("=============sdp:[%s]\n", arr_RecvBuf);
	memset(&st_MsgHead, 0, sizeof(st_MsgHead));
	memcpy(st_MsgHead.header, "hBzHbox@", 8);
	st_MsgHead.cmd_code = CMD_OK;

	snprintf(arr_SendBuf + sizeof(BOX_CTRL_CMD_OBJ), 2048 - sizeof(BOX_CTRL_CMD_OBJ),
					"{\"CmdType\":\"sdp_info\",\"m_video\":\"%s\",\"a_rtpmap_video\":\"%s\",\"a_fmtp_video\":\"%s\",\"m_audio\":\"%s\",\"a_rtpmap_audio\":\"%s\"}",
					pDevNode->m_video, pDevNode->a_rtpmap_video, pDevNode->a_fmtp_video, pDevNode->m_audio, pDevNode->a_rtpmap_audio);

	st_MsgHead.cmd_length = htonl(strlen(arr_SendBuf + sizeof(BOX_CTRL_CMD_OBJ)));
//	st_MsgHead.cmd_length = strlen(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ));

	memcpy(arr_SendBuf, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ));

//	printf("st_MsgHead.cmd_length = %d, head_len[%d]\n", st_MsgHead.cmd_length, sizeof(BOX_CTRL_CMD_OBJ));
	pDevNode->enumDevConnectStatus = CONNECTED;    //设置为设备已连接状态

	for (i = 0; i < list_size(plistWaitClient); i++)
	{
		WAIT_CLIENT_LIST_HANDLE pCurWaitClient = (WAIT_CLIENT_LIST_HANDLE) list_get_at(plistWaitClient, i);
		struct bufferevent *p_AcceptClient_bev = pCurWaitClient->pWaitClientBev;
		bufferevent_write(p_AcceptClient_bev, arr_SendBuf, ntohl(st_MsgHead.cmd_length) + sizeof(BOX_CTRL_CMD_OBJ));
		bufferevent_setcb(p_AcceptClient_bev, deal_client_cmd, NULL, deal_client_cmd_error_cb2, (HB_VOID *) pDevNode);
		bufferevent_enable(p_AcceptClient_bev, EV_READ);
	}
	pthread_rwlock_wrlock(&rwlockMyLock);
	list_destroy(plistWaitClient);
	pthread_rwlock_unlock(&rwlockMyLock);
}



/*
 *	Function: 基本认证回调函数
 *
 *	@param bev: 与设备的链接句柄
 *  @parmm args	: 实际为LIBEVENT_ARGS_HANDLE类型的参数结构体
 *
 *	Retrun: 无
 */
static void read_dev_sdp_basic_cb(struct bufferevent *connect_dev_bev, void *arg)
{
	HB_S32 i;
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE) arg;
	list_t *plistWaitClient = (list_t *) &(pDevNode->listWaitClient);
	BOX_CTRL_CMD_OBJ st_MsgHead;
	HB_CHAR arr_RecvBuf[10240] = { 0 };
	HB_CHAR arr_SendBuf[2048] = { 0 };

	bufferevent_read(connect_dev_bev, arr_RecvBuf, sizeof(arr_RecvBuf));
	printf("=============recv sdp:[%s]\n", arr_RecvBuf);
#if 1
	rtsp_parser_t* parser;
	parser = rtsp_parser_create(RTSP_PARSER_CLIENT);
	if(NULL == parser)
	{
		return;
	}
	HB_S32 msg_len = strlen(arr_RecvBuf);
	if(rtsp_parser_input(parser, arr_RecvBuf, &msg_len) != 0)
	{
		return;
	}

	HB_S32 rtsp_code = 0;
	rtsp_code = rtsp_get_status_code(parser);

	if(rtsp_code != 200 && rtsp_code != 401)
	{
		rtsp_parser_destroy(parser);
		return;
	}

	if (401 == rtsp_code)
	{
		//基本认证失败，发送摘要认证
		HB_CHAR cDiscribe[1024] = { 0 };

		HB_CHAR *authenticate_str = NULL;
		authenticate_str = rtsp_get_header_by_name(parser, (const char*)"WWW-Authenticate");
		rtsp_parser_destroy(parser);
		if(NULL == authenticate_str)
		{
			return;
		}
		if(strcasestr(authenticate_str, "Digest")) //摘要认证
		{
			if(get_realm_and_nonce(authenticate_str, pDevNode) < 0)
			{
				return;
			}

			printf("\nMMMMMMM   realm=[%s] nonce=[%s]\n", pDevNode->realm, pDevNode->nonce);
			HB_CHAR response[64] = {0};
			get_response(response, "DESCRIBE", pDevNode);
			memset(cDiscribe, 0, sizeof(cDiscribe));
			if (pDevNode->iDevStreamType == 0)
			{
				//主码流
				snprintf(cDiscribe, sizeof(cDiscribe), SEND_DESCRIBE_DIGEST_MSG, pDevNode->arrDevRtspMainUrl, pDevNode->arrUserName,
								pDevNode->realm, pDevNode->nonce, pDevNode->arrDevRtspMainUrl, response);
			}
			else
			{
				//子码流
				if ((pDevNode->arrDevRtspSubUrl == NULL) || (strlen(pDevNode->arrDevRtspSubUrl) == 0))
				{
					//没有子码流依然使用主码流
					snprintf(cDiscribe, sizeof(cDiscribe), SEND_DESCRIBE_DIGEST_MSG, pDevNode->arrDevRtspMainUrl, pDevNode->arrUserName,
													pDevNode->realm, pDevNode->nonce, pDevNode->arrDevRtspMainUrl, response);
				}
				else
				{
					//有子码流
					snprintf(cDiscribe, sizeof(cDiscribe), SEND_DESCRIBE_DIGEST_MSG, pDevNode->arrDevRtspSubUrl, pDevNode->arrUserName,
													pDevNode->realm, pDevNode->nonce, pDevNode->arrDevRtspSubUrl, response);
				}
			}

			printf("\n>>>>>>>>>>>>>>>>>>>>>>> authentication DESCRIBE send: [%s]\n", cDiscribe);

			bufferevent_write(connect_dev_bev, cDiscribe, strlen(cDiscribe));
			bufferevent_setcb(connect_dev_bev, read_dev_sdp_digest_cb, NULL, active_connect_eventcb, (HB_VOID *) pDevNode);
			bufferevent_enable(connect_dev_bev, EV_READ);

			return ;
		}
	}
	bufferevent_free(connect_dev_bev);
	connect_dev_bev = NULL;
#endif
	analysis_sdp_info(arr_RecvBuf, pDevNode);

	printf("=============sdp:[%s]\n", arr_RecvBuf);
	memset(&st_MsgHead, 0, sizeof(st_MsgHead));
	memcpy(st_MsgHead.header, "hBzHbox@", 8);
	st_MsgHead.cmd_code = CMD_OK;

	snprintf(arr_SendBuf + sizeof(BOX_CTRL_CMD_OBJ), 2048 - sizeof(BOX_CTRL_CMD_OBJ),
					"{\"CmdType\":\"sdp_info\",\"m_video\":\"%s\",\"a_rtpmap_video\":\"%s\",\"a_fmtp_video\":\"%s\",\"m_audio\":\"%s\",\"a_rtpmap_audio\":\"%s\"}",
					pDevNode->m_video, pDevNode->a_rtpmap_video, pDevNode->a_fmtp_video, pDevNode->m_audio, pDevNode->a_rtpmap_audio);

	st_MsgHead.cmd_length = htonl(strlen(arr_SendBuf + sizeof(BOX_CTRL_CMD_OBJ)));
//	st_MsgHead.cmd_length = strlen(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ));

	memcpy(arr_SendBuf, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ));

//	printf("st_MsgHead.cmd_length = %d, head_len[%d]\n", st_MsgHead.cmd_length, sizeof(BOX_CTRL_CMD_OBJ));
	pDevNode->enumDevConnectStatus = CONNECTED;    //设置为设备已连接状态

	for (i = 0; i < list_size(plistWaitClient); i++)
	{
		WAIT_CLIENT_LIST_HANDLE pCurWaitClient = (WAIT_CLIENT_LIST_HANDLE) list_get_at(plistWaitClient, i);
		struct bufferevent *p_AcceptClient_bev = pCurWaitClient->pWaitClientBev;
		bufferevent_write(p_AcceptClient_bev, arr_SendBuf, ntohl(st_MsgHead.cmd_length) + sizeof(BOX_CTRL_CMD_OBJ));
		bufferevent_setcb(p_AcceptClient_bev, deal_client_cmd, NULL, deal_client_cmd_error_cb2, (HB_VOID *) pDevNode);
		bufferevent_enable(p_AcceptClient_bev, EV_READ);
	}
	pthread_rwlock_wrlock(&rwlockMyLock);
	list_destroy(plistWaitClient);
	pthread_rwlock_unlock(&rwlockMyLock);
}


/*
 * Function: 测试设备的连通性
 *
 * @param pMessengerArgs : [IN] libevent 通信参数
 *
 * Return : 无
 */
HB_VOID test_dev_connection(DEV_LIST_HANDLE pDevNode)
{
	struct timeval tv_read; //读超时
//	const char *in_filename_v = "rtsp://admin:888888@192.168.8.21:8554/H264MainStream";//汉邦ipc
//	const char *in_filename_v = "rtsp://admin:admin@192.168.8.198:554/video1";//宇视ipc video1-主码流 video2-辅码流 video3-第三码流
//	const char *in_filename_v = "rtsp://admin:123456@10.7.126.242:8554/cam/realmonitor?channel=1&subtype=0";//大华
//	const char *in_filename_v = "rtsp://admin:admin12345@192.168.8.64:10001/Streaming/Channels/101?transportmode=unicast&amp;profile=Profile_1893387798";//海康
	struct sockaddr_in connect_to_addr;
	struct bufferevent *connect_dev_bev = bufferevent_socket_new(pEventBase, -1,
					BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);

	tv_read.tv_sec = 5;
	tv_read.tv_usec = 0;

	bzero(&connect_to_addr, sizeof(connect_to_addr));
	connect_to_addr.sin_family = AF_INET;
	connect_to_addr.sin_port = htons(pDevNode->iDevRtspPort);
	inet_pton(AF_INET, pDevNode->arrDevIp, (void *) &connect_to_addr.sin_addr);
	printf("pDevNode->arrDevIp[%s], pDevNode->port[%d]\n", pDevNode->arrDevIp, pDevNode->iDevRtspPort);

	bufferevent_set_timeouts(connect_dev_bev, &tv_read, NULL);
	bufferevent_setcb(connect_dev_bev, read_dev_sdp_basic_cb, NULL, active_connect_eventcb, (HB_VOID *)pDevNode);
	bufferevent_socket_connect(connect_dev_bev, (struct sockaddr*) &connect_to_addr, sizeof(struct sockaddr_in));
	bufferevent_enable(connect_dev_bev, EV_READ | EV_WRITE);
}
/***********************测试设备连通性END***********************/
/***********************测试设备连通性END***********************/
/***********************测试设备连通性END***********************/

/***********************视频流传输***********************/
/***********************视频流传输***********************/
/***********************视频流传输***********************/
#if 0
static int interrupt_cb(void *ctx)
{
	// do something
	static int count = 0;

	if (count++ > 100)
	{
		count = 0;
		time_t *time_last = (time_t *) ctx;
		time_t time_now = 0;

		time(&time_now);
//		printf("time_last=[%ld], time_now=[%ld]\n", *time_last, time_now);
		if ((time_now - *time_last) > 15)
		{
			printf("av_read_frame time out cur_time:%ld, last_time:%ld\n", time_now, *time_last);
			return AVERROR_EOF; //这个就是超时的返回
		}
	}

	return 0;
}
#endif

//从设备读取视频流线程
HB_VOID *read_video_data_from_dev_task(HB_VOID *arg)
{
	pthread_detach(pthread_self());
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE) arg;
	list_t *plistRtspClient = (list_t *) &(pDevNode->listRtspClient);
	HB_CHAR arrOpenRtspUrl[1024] = { 0 };
	pDevNode->iStartThreadFlag = 1;

	pthread_t thread_id = pthread_self();

	TRACE_YELLOW("thread_id[%lu]--->dev_id[%s]--->dev_Chnl[%d]--->dev_stream_type[%d]\n", thread_id, pDevNode->pDevId, pDevNode->iDevChnl,
					pDevNode->iDevStreamType);

//	time_t time_now = time(NULL);

	int i;
	HB_S32 iRet = 0;
	int videoindex_v = -1;
	int audioindex_a = -1;

	//Input AVFormatContext and Output AVFormatContext
	AVFormatContext *in_fmt_ctx_v = avformat_alloc_context();
//	in_fmt_ctx_v->interrupt_callback.callback = interrupt_cb; //--------注册回调函数,用于控制av_read_frame的超时
//	in_fmt_ctx_v->interrupt_callback.opaque = &time_now;

	AVDictionary* options = NULL;
//	av_dict_set(&options, "max_delay", "50000000", 0);
	//设置rtsp传输模式为tcp
	av_dict_set(&options, "rtsp_transport", "tcp", 0);
	av_dict_set(&options, "stimeout", "5000000", 0);//打开流超时时间 单位us  10s
//	av_dict_set(&options, "rw_timeout", "2000", 0);//读取流超时 单位ms

	if (pDevNode->iDevStreamType == 0)
	{
		//主码流
		if ((pDevNode->arrUserName == NULL) || (strlen(pDevNode->arrUserName) == 0))
		{
			//没有用户名密码
			snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s", pDevNode->arrDevRtspMainUrl + strlen("rtsp://"));
		}
		else
		{
			snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s:%s@%s", pDevNode->arrUserName, pDevNode->arrUserPasswd,
							pDevNode->arrDevRtspMainUrl + strlen("rtsp://"));
		}

	}
	else
	{

		//子码流
		if ((pDevNode->arrUserName == NULL) || (strlen(pDevNode->arrUserName) == 0))
		{
			//没有用户名密码
			if ((pDevNode->arrDevRtspSubUrl == NULL) || (strlen(pDevNode->arrDevRtspSubUrl) == 0))
			{
				//如果没有子码流，依然使用主码流打开
				snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s", pDevNode->arrDevRtspMainUrl + strlen("rtsp://"));
			}
			else
			{
				snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s", pDevNode->arrDevRtspSubUrl + strlen("rtsp://"));
			}
		}
		else
		{
			if ((pDevNode->arrDevRtspSubUrl == NULL) || (strlen(pDevNode->arrDevRtspSubUrl) == 0))
			{
				//如果没有子码流，依然使用主码流打开
				snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s:%s@%s", pDevNode->arrUserName, pDevNode->arrUserPasswd,
								pDevNode->arrDevRtspMainUrl + strlen("rtsp://"));
			}
			else
			{
				snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s:%s@%s", pDevNode->arrUserName, pDevNode->arrUserPasswd,
								pDevNode->arrDevRtspSubUrl + strlen("rtsp://"));
			}
		}
	}

	printf("OpenRtspUrl[%s]\n", arrOpenRtspUrl);
	iRet = avformat_open_input(&in_fmt_ctx_v, arrOpenRtspUrl, NULL, &options);
#if 1
	if (iRet != 0)
	{
		printf("\n0000000000000avformat_open_input failed ret=%d\n", iRet);
		av_dict_set(&options, "rtsp_transport", "udp", 0);
		iRet = avformat_open_input(&in_fmt_ctx_v, arrOpenRtspUrl, NULL, &options);
		if (iRet != 0)
		{
			printf("\n111111111111111avformat_open_input failed ret=%d\n", iRet);
			av_dict_free(&options);
			goto End;
		}

	}
#endif
	av_dict_free(&options);

	for (i = 0; i < in_fmt_ctx_v->nb_streams; i++)
	{
		//Create output AVStream according to input AVStream
//        if(in_fmt_ctx_v->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		AVStream *stream = in_fmt_ctx_v->streams[i];
		AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
		AVCodecContext *codec_ctx = avcodec_alloc_context3(codec); //需要使用avcodec_free_context释放

		//事实上codecpar包含了大部分解码器相关的信息，这里是直接从AVCodecParameters复制到AVCodecContext
		avcodec_parameters_to_context(codec_ctx, stream->codecpar);
//    	av_codec_set_pkt_timebase(codec_ctx, stream->time_base);
		if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
		{
//			AVStream *in_stream = in_fmt_ctx_v->streams[i];
//			printf("\nBBBBBBBBB   [frame rate = %lf]  [%d*%d]\n", (in_stream->avg_frame_rate.num/(double)(in_stream->avg_frame_rate.den)), codec_ctx->width, codec_ctx->height);
			videoindex_v = i;
		}
		else if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioindex_a = i;
		}
		avcodec_free_context(&codec_ctx);
		codec_ctx = NULL;
	}

	TRACE_GREEN("\n####  open rtsp successful dev_id[%s]-->dev_Chnl[%d]-->dev_stream_type[%d]\n", pDevNode->pDevId, pDevNode->iDevChnl,
					pDevNode->iDevStreamType);

	HB_S64 iIFlag = 0; //出现I帧的标志位
	HB_S32 iWriteBufLen = 0;
	BOX_CTRL_CMD_OBJ send_cmd;
	memset(&send_cmd, 0, sizeof(BOX_CTRL_CMD_OBJ));
	strncpy(send_cmd.header, "hBzHbox@", 8);

//	HB_S32 av_read_err_count = 0;
//	HB_S64 old_pts = 0;

	while (1)
	{
		if (list_size(plistRtspClient) < 1)
		{
			TRACE_YELLOW("Don't have any people watch the video. Exit!!!!!!!!!\n");
			break;
		}
		AVPacket *p_pkt = (AVPacket*) malloc(sizeof(AVPacket));
		av_init_packet(p_pkt);

		if ((iRet = av_read_frame(in_fmt_ctx_v, p_pkt)) >= 0)
		{
//			printf("\n thread:[%lu] VIDEO VIDEO VIDEO VIDEO  frame type=%d duration=%lld pts=%lld\n", thread_id, p_pkt->flags, p_pkt->duration, p_pkt->pts);
//			printf("\nVIDEO VIDEOframe type=%d duration=%lld pts=%llu\n", p_pkt->flags, p_pkt->duration, p_pkt->pts);
			if (videoindex_v == p_pkt->stream_index) //视频帧
			{

//				printf("\nVIDEO VIDEOframe type=%d duration=%lld pts=%lld interval=%lld data_len=%d\n", p_pkt->flags, p_pkt->duration, p_pkt->pts, p_pkt->pts - old_pts, p_pkt->size);
//				old_pts = p_pkt->pts;
				if (1 == p_pkt->flags) //I帧
				{
//					printf("IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII=%ld\n", time(&time_now));
//					time(&time_now);
					if (iIFlag == 0)
					{
						printf("\nVIDEO VIDEOframe type=%d duration=%lld pts=%lld data_len=%d\n", p_pkt->flags, p_pkt->duration, p_pkt->pts, p_pkt->size);
						if(p_pkt->pts > 0)
						{
							iIFlag = 1;
						}
						else
						{
							av_packet_free(&p_pkt);
							continue;
						}
					}
				}
				else //BP帧
				{
					if (!iIFlag) //第一个I帧前的数据全部丢弃
					{
						av_packet_free(&p_pkt);
						continue;
					}
				}
			}
			else if (audioindex_a == p_pkt->stream_index) //音频帧
			{
				av_packet_free(&p_pkt);
				continue;
//				//第一个I帧来之前，音频也不播放
//				if(0 == I_flag)
//				{
//					av_packet_free(&p_pkt);
//					continue;
//				}
//				frame_type = 1;
//				printf("\nAUDIO AUDIO AUDIO AUDIO  frame type=%d duration=%lld pts=%lld\n", p_pkt->flags, p_pkt->duration, p_pkt->pts);
//				av_packet_free(&p_pkt);
//				continue;
			}
			else	//其它类型帧直接忽略
			{
				av_packet_free(&p_pkt);
				continue;
			}

#if 1
			send_cmd.cmd_type = BOX_VIDEO_DATA;
			if (1 == p_pkt->flags)
			{
				send_cmd.data_type = I_FRAME;
			}
			else
			{
				send_cmd.data_type = BP_FRAME;
			}
			send_cmd.cmd_length = p_pkt->size;

			HB_S32 iClientNum = list_size(plistRtspClient);
//			printf("client num = %d\n", iClientNum);

			pthread_rwlock_wrlock(&rwlockMyLock);
			for (i = 0; i < iClientNum; ++i)
			{
				CLIENT_LIST_HANDLE pClientNode = (CLIENT_LIST_HANDLE)list_get_at(plistRtspClient, i);
				if (pClientNode->iDelFlag == 1)
				{
					if (pClientNode->pSendVideoToServerEvent != NULL)
					{
						bufferevent_free(pClientNode->pSendVideoToServerEvent);
						pClientNode->pSendVideoToServerEvent = NULL;
						list_delete(plistRtspClient, (HB_VOID *) pClientNode);
						--i;
						--iClientNum;
						TRACE_YELLOW("\n###########  total client total client total client total client== [%d]!\n", list_size(plistRtspClient));
					}
				}
				else if (1 == pClientNode->iSendFrameFlag)
				{
					//获取当前缓冲区中已有数据的长度
					iWriteBufLen = evbuffer_get_length(bufferevent_get_output(pClientNode->pSendVideoToServerEvent));
					if ((LIBEVENT_WRITE_BUF_SIZE - iWriteBufLen) < (p_pkt->size + sizeof(BOX_CTRL_CMD_OBJ)))
					{
						printf("send buff full send buff full send buff full!\n");
						//如果当前缓冲区的空间不足以存储一帧数据，则丢帧(清空当前缓冲区) evbuffer_drain用于清空缓冲区
						evbuffer_drain(bufferevent_get_output(pClientNode->pSendVideoToServerEvent), iWriteBufLen);
						pClientNode->iMissFrameFlag = 1;
//							pthread_rwlock_unlock(&rwlockMyLock);
//							pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
						continue;
					}
					else
					{
						//若缓冲区未满，则判断是否丢过帧，若丢过帧则需要将p帧丢弃，从I帧开始发送
						if ((1 == pClientNode->iMissFrameFlag) && (1 != p_pkt->flags))
						{;
							//丢过帧且当前不是I帧，此帧丢弃
//								printf("miss miss miss miss miss miss frame!\n");
//								pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
//								pthread_rwlock_unlock(&rwlockMyLock);
							continue;
						}
					}

					pClientNode->iMissFrameFlag = 0;
//					pClientNode->pts += pDevNode->iPtsRateInterval;
//					send_cmd.pts = pClientNode->pts;
					send_cmd.pts = p_pkt->pts;
//						send_cmd.uiVideoSec = (((HB_U32)(pClientNode->pts))/90)/1000;
//						send_cmd.uiVideoUsec = (HB_U32)(((pClientNode->pts/90)%1000)*1000);
					bufferevent_write(pClientNode->pSendVideoToServerEvent, &send_cmd, sizeof(BOX_CTRL_CMD_OBJ));
					bufferevent_write(pClientNode->pSendVideoToServerEvent, p_pkt->data, p_pkt->size);
				}
//					printf("%pdata_type:[%d] ------> data_size:[%d] ------>pts:[%lld]\n", pClientNode->pSendVideoToServerEvent, send_cmd.data_type, p_pkt->size, send_cmd.pts);

			}
			pthread_rwlock_unlock(&rwlockMyLock);
			av_packet_free(&p_pkt);
#endif
		}
		else
		{
			av_packet_free(&p_pkt);
			TRACE_ERR("av_read_frame() failed!%d\n", iRet);
			break;

//			++av_read_err_count;
//			av_packet_free(&p_pkt);
//			TRACE_ERR("av_read_frame() failed!%d, err_count=%d\n", iRet, av_read_err_count);
//			if (av_read_err_count > 10)
//			{
////				TRACE_ERR("av_read_frame() failed !  Break\n", iRet);
//				break;
//			}
		}
	}

	End: printf("avformat_close_input\n");
	avformat_close_input(&in_fmt_ctx_v);
	printf("avformat_close_input ok!\n");
	pthread_rwlock_wrlock(&rwlockMyLock);
	list_destroy(plistRtspClient);
	list_delete(&listDevList, (HB_VOID *) pDevNode);
	free(pDevNode);
	pDevNode = NULL;
	pthread_rwlock_unlock(&rwlockMyLock);

	TRACE_ERR("read video thread[%lu] exit!\n", thread_id);
	return 0;
}
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
