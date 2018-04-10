/*
 * dev_opt.c
 *
 *  Created on: 2017年9月25日
 *      Author: lijian
 */

#include "dev_opt.h"
#include "dev_list.h"

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
		strncpy(p_DevNode->m_video, p_Pos1, p_Pos2-p_Pos1);
		printf("get m_video:[%s]\n", p_DevNode->m_video);
		p_Pos1 = strstr(p_Pos1, "a=rtpmap");
		if (p_Pos1 != NULL)
		{
			p_Pos2 = strstr(p_Pos1, "\r\n");
			strncpy(p_DevNode->a_rtpmap_video, p_Pos1, p_Pos2-p_Pos1);
			p_Pos2 = strstr(p_Pos1, "/") + 1;
			p_DevNode->iVideoFrameRate = atoi(p_Pos2);

			printf("get a_rtpmap_video:[%s], FrameRate=[%d]\n", p_DevNode->a_rtpmap_video, p_DevNode->iVideoFrameRate);
		}

		p_Pos1 = strstr(p_Pos1, "a=fmtp");
		if (p_Pos1 != NULL)
		{
			p_Pos2 = strstr(p_Pos1, "\r\n");
			strncpy(p_DevNode->a_fmtp_video, p_Pos1, p_Pos2-p_Pos1);
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
		strncpy(p_DevNode->m_audio, p_Pos1, p_Pos2-p_Pos1);
		printf("get m_audio:[%s]\n", p_DevNode->m_audio);
		p_Pos1 = strstr(p_Pos1, "a=rtpmap_audio");
		if (p_Pos1 != NULL)
		{
			p_Pos2 = strstr(p_Pos1, "\r\n");
			strncpy(p_DevNode->a_rtpmap_audio, p_Pos1, p_Pos2-p_Pos1);
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
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)arg;
	list_t *plistRtspClient = (list_t *)&(pDevNode->listRtspClient);
	list_t *plistWaitClient = (list_t *)&(pDevNode->listWaitClient);
	HB_S32 err = EVUTIL_SOCKET_ERROR();

	bufferevent_disable(bev, EV_READ|EV_WRITE);

	if (events & BEV_EVENT_EOF)//对端关闭
	{
		TRACE_ERR("######## deal_client_cmd_error_cb2 BEV_EVENT_EOF(%d) : %s !", err, evutil_socket_error_to_string(err));
	}
	else if (events & BEV_EVENT_ERROR)//错误事件
	{
		TRACE_ERR("######## deal_client_cmd_error_cb2 BEV_EVENT_ERROR(%d) : %s !", err, evutil_socket_error_to_string(err));
	}
	else if (events & BEV_EVENT_TIMEOUT)//超时事件
	{
		TRACE_ERR("######## deal_client_cmd_error_cb2 BEV_EVENT_TIMEOUT(%d) : %s !", err, evutil_socket_error_to_string(err));
	}

	if ((pDevNode != NULL) && (list_size(plistRtspClient) < 1) && (list_size(plistWaitClient) < 1))
	{
		printf("del one dev from dev_list!\n");
		pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
		del_one_from_dev_list(pDevNode);
		pDevNode = NULL;
		pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
	}
	printf("deal_client_cmd_error_cb2 free bev!\n");
	bufferevent_free(bev);
	bev = NULL;
}


/*
 *	Function: 从设备获取sdp信息并解析
 *
 *	@param bev: 与设备的链接句柄
 *  @parmm args	: 实际为LIBEVENT_ARGS_HANDLE类型的参数结构体
 *
 *	Retrun: 无
 */
static void read_dev_sdp_cb(struct bufferevent *connect_dev_bev, void *arg)
{
	HB_S32 i;
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)arg;
	list_t *plistWaitClient = (list_t *)&(pDevNode->listWaitClient);
	BOX_CTRL_CMD_OBJ st_MsgHead;
	HB_CHAR arr_RecvBuf[10240] = {0};
	HB_CHAR arr_SendBuf[2048] = {0};

	bufferevent_read(connect_dev_bev, arr_RecvBuf, sizeof(arr_RecvBuf));
	bufferevent_free(connect_dev_bev);
	connect_dev_bev = NULL;

	analysis_sdp_info(arr_RecvBuf, pDevNode);

//	printf("=============sdp[%d]: [%s]\n", ret, arr_RecvBuf);
	memset(&st_MsgHead, 0, sizeof(st_MsgHead));
	memcpy(st_MsgHead.header, "hBzHbox@", 8);
	st_MsgHead.cmd_code = CMD_OK;


	snprintf(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ), 2048-sizeof(BOX_CTRL_CMD_OBJ), \
			"{\"CmdType\":\"sdp_info\",\"m_video\":\"%s\",\"a_rtpmap_video\":\"%s\",\"a_fmtp_video\":\"%s\",\"m_audio\":\"%s\",\"a_rtpmap_audio\":\"%s\"}", \
			pDevNode->m_video, pDevNode->a_rtpmap_video, pDevNode->a_fmtp_video, pDevNode->m_audio, pDevNode->a_rtpmap_audio);

	st_MsgHead.cmd_length = htonl(strlen(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ)));
//	st_MsgHead.cmd_length = strlen(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ));

	memcpy(arr_SendBuf, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ));

//	printf("st_MsgHead.cmd_length = %d, head_len[%d]\n", st_MsgHead.cmd_length, sizeof(BOX_CTRL_CMD_OBJ));
	pDevNode->enumDevConnectStatus = CONNECTED;//设置为设备已连接状态

	for (i=0;i<list_size(plistWaitClient);i++)
	{
		//获取每个zigbee模块下的传感器
		WAIT_CLIENT_LIST_HANDLE pCurWaitClient = (WAIT_CLIENT_LIST_HANDLE)list_get_at(plistWaitClient, i);
		struct bufferevent *p_AcceptClient_bev = pCurWaitClient->pWaitClientBev;
		bufferevent_write(p_AcceptClient_bev, arr_SendBuf, ntohl(st_MsgHead.cmd_length)+sizeof(BOX_CTRL_CMD_OBJ));
		bufferevent_setcb(p_AcceptClient_bev, deal_client_cmd, NULL, deal_client_cmd_error_cb2, (HB_VOID *)pDevNode);
		bufferevent_enable(p_AcceptClient_bev, EV_READ);
		pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
//		del_one_wait_client(&(pDevNode->stWaitClientHead), pDevNode->stWaitClientHead.pWaitClientListFirst);
		list_delete(plistWaitClient, pCurWaitClient);
		pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
	}

	return;
}




static HB_VOID active_connect_eventcb(struct bufferevent *connect_dev_bev, HB_S16 what, HB_VOID *arg)
{
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)arg;
	list_t *plistWaitClient = (list_t *)&(pDevNode->listWaitClient);

	if (what & BEV_EVENT_CONNECTED)//盒子主动connect设备成功
	{
		//连接设备成功发送describe获取sdp信息
		HB_CHAR arr_Discribe[1024] = {0};

		if (pDevNode->iDevStreamType==0)
		{
			//主码流
			snprintf(arr_Discribe, sizeof(arr_Discribe), \
				"DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\nAuthorization: Basic %s\r\n\r\n", \
				pDevNode->arrDevRtspMainUrl, pDevNode->arrBasicAuthenticate);
			printf("DESCRIBE main 1: [%s]\n", arr_Discribe);
		}
		else
		{
			//子码流
			if ((pDevNode->arrDevRtspSubUrl == NULL) || (strlen(pDevNode->arrDevRtspSubUrl) == 0))
			{
				//没有子码流依然使用主码流
				snprintf(arr_Discribe, sizeof(arr_Discribe), \
					"DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\nAuthorization: Basic %s\r\n\r\n", \
					pDevNode->arrDevRtspMainUrl, pDevNode->arrBasicAuthenticate);
				printf("DESCRIBE main 2: [%s]\n", arr_Discribe);
			}
			else
			{
				//有子码流
				snprintf(arr_Discribe, sizeof(arr_Discribe), \
					"DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\nAuthorization: Basic %s\r\n\r\n", \
					pDevNode->arrDevRtspSubUrl, pDevNode->arrBasicAuthenticate);
				printf("DESCRIBE sub 1: [%s]\n", arr_Discribe);
			}
		}

		TRACE_GREEN("\n############  connect dev successful and start to get dev SDP !\n");
		bufferevent_write(connect_dev_bev, arr_Discribe, strlen(arr_Discribe));
	}
	else
	{
		//盒子connect 设备失败,当数据库中有此设备，但此设备ip连接不上时会进到此接口
		TRACE_ERR("\n###########  box connect dev  failed !\n");
		pDevNode->enumDevConnectStatus = DISCONNECT;//设置设备连接失败状态

		if (pDevNode != NULL)
		{
			printf("del one dev from dev_list!\n");
			pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
			list_destroy(plistWaitClient);
			del_one_from_dev_list(pDevNode);
			pDevNode = NULL;
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
		}
//		此连接事件由deal_client_cmd_error_cb1超时时进行释放
		printf("active_connect_eventcb free bev!\n");
		bufferevent_free(connect_dev_bev);
		connect_dev_bev = NULL;
	}
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
	struct bufferevent *connect_dev_bev = bufferevent_socket_new(pEventBase, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);

	tv_read.tv_sec  = 5;
	tv_read.tv_usec = 0;

	bzero(&connect_to_addr, sizeof(connect_to_addr));
	connect_to_addr.sin_family = AF_INET;
	connect_to_addr.sin_port = htons(pDevNode->iDevRtspPort);
	inet_pton(AF_INET, pDevNode->arrDevIp, (void *)&connect_to_addr.sin_addr);
	printf("pDevNode->arrDevIp[%s], pDevNode->port[%d]\n", pDevNode->arrDevIp, pDevNode->iDevRtspPort);

	bufferevent_set_timeouts(connect_dev_bev, &tv_read, NULL);
	bufferevent_socket_connect(connect_dev_bev, (struct sockaddr*)&connect_to_addr, sizeof(struct sockaddr_in));
	bufferevent_setcb(connect_dev_bev, read_dev_sdp_cb, NULL, active_connect_eventcb, (HB_VOID *)pDevNode);
    bufferevent_enable(connect_dev_bev, EV_READ|EV_WRITE);
}
/***********************测试设备连通性END***********************/
/***********************测试设备连通性END***********************/
/***********************测试设备连通性END***********************/



/***********************视频流传输***********************/
/***********************视频流传输***********************/
/***********************视频流传输***********************/
static int interrupt_cb(void *ctx)
{
    // do something
	static int count = 0;

	if (count++ > 100)
	{
		count = 0;
		time_t *time_last = (time_t *)ctx;
		time_t time_now = 0;

		time(&time_now);
//		printf("time_last=[%ld], time_now=[%ld]\n", *time_last, time_now);
		if ((time_now-*time_last) > 5)
		{
			printf("av_read_frame time out\n");
			return AVERROR_EOF;//这个就是超时的返回
		}
	}

    return 0;
}

//从设备读取视频流线程
HB_VOID *read_video_data_from_dev_task(HB_VOID *arg)
{
	pthread_detach(pthread_self());
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)arg;
	list_t *plistRtspClient = (list_t *)&(pDevNode->listRtspClient);
	HB_CHAR arrOpenRtspUrl[1024] = {0};
	pDevNode->iStartThreadFlag = 1;

	pthread_t thread_id = pthread_self();

	TRACE_YELLOW("thread_id[%lu]--->dev_id[%s]--->dev_Chnl[%d]--->dev_stream_type[%d]\n", \
			thread_id, pDevNode->pDevId, pDevNode->iDevChnl, pDevNode->iDevStreamType);

	time_t  time_now = time(NULL);

    int ret, i;
    int videoindex_v=-1;
    int audioindex_a=-1;

    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *in_fmt_ctx_v = avformat_alloc_context();
    in_fmt_ctx_v->interrupt_callback.callback = interrupt_cb; //--------注册回调函数,用于控制av_read_frame的超时
    in_fmt_ctx_v->interrupt_callback.opaque = &time_now;

	AVDictionary* options = NULL;
    //设置rtsp传输模式为tcp
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    if (pDevNode->iDevStreamType == 0)
    {
    	//主码流
        snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s:%s@%s", \
        	pDevNode->arrUserName, pDevNode->arrUserPasswd, pDevNode->arrDevRtspMainUrl+strlen("rtsp://"));
    }
    else
    {
    	//子码流
    	if ((pDevNode->arrDevRtspSubUrl == NULL) || (strlen(pDevNode->arrDevRtspSubUrl) == 0))
    	{
    		//如果没有子码流，依然使用主码流打开
    		snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s:%s@%s", \
    				pDevNode->arrUserName, pDevNode->arrUserPasswd, pDevNode->arrDevRtspMainUrl+strlen("rtsp://"));
    	}
    	else
    	{
    		snprintf(arrOpenRtspUrl, sizeof(arrOpenRtspUrl), "rtsp://%s:%s@%s", \
    		    	pDevNode->arrUserName, pDevNode->arrUserPasswd, pDevNode->arrDevRtspSubUrl+strlen("rtsp://"));
    	}
    }

    printf("OpenRtspUrl[%s]\n", arrOpenRtspUrl);
    ret = avformat_open_input(&in_fmt_ctx_v, arrOpenRtspUrl, NULL, &options);
    if(ret != 0)
    {
    	ret = avformat_open_input(&in_fmt_ctx_v, arrOpenRtspUrl, NULL, NULL);
        if(ret != 0)
        {
			printf("\n0000000000000avformat_open_input failed ret=%d\n", ret);
			av_dict_free(&options);
			goto End;
        }

    }
    av_dict_free(&options);

    for (i = 0; i < in_fmt_ctx_v->nb_streams; i++)
    {
        //Create output AVStream according to input AVStream
//        if(in_fmt_ctx_v->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
    	AVStream *stream = in_fmt_ctx_v->streams[i];
    	AVCodec *codec =  avcodec_find_decoder(stream->codecpar->codec_id);
    	AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);//需要使用avcodec_free_context释放

    	//事实上codecpar包含了大部分解码器相关的信息，这里是直接从AVCodecParameters复制到AVCodecContext
    	avcodec_parameters_to_context(codec_ctx, stream->codecpar);
//    	av_codec_set_pkt_timebase(codec_ctx, stream->time_base);
    	if(codec_ctx->codec_type==AVMEDIA_TYPE_VIDEO)
        {
//			AVStream *in_stream = in_fmt_ctx_v->streams[i];
//			printf("\nBBBBBBBBB   [frame rate = %lf]  [%d*%d]\n", (in_stream->avg_frame_rate.num/(double)(in_stream->avg_frame_rate.den)), codec_ctx->width, codec_ctx->height);
			videoindex_v=i;
        }
        else if(codec_ctx->codec_type==AVMEDIA_TYPE_AUDIO)
        {
			audioindex_a=i;
        }
    	avcodec_free_context(&codec_ctx);
    	codec_ctx = NULL;
    }

    TRACE_GREEN("\n####  open rtsp successful dev_id[%s]-->dev_Chnl[%d]-->dev_stream_type[%d]\n", \
    		pDevNode->pDevId, pDevNode->iDevChnl, pDevNode->iDevStreamType);

	HB_S64 iIFlag = 0; //由于前几帧的pts经常出错，此变量用于忽略第两个个I帧，从第三I帧开始发送数据
    HB_S32 iCount = 0;
    HB_S64 llPtsOld = 0;
    int iRet = 0;
    while (1)
    {
    	if(list_size(plistRtspClient) < 1)
    	{
    		TRACE_YELLOW("Don't have any people watch the video. Exit!!!!!!!!!\n");
    		break;
    	}
    	AVPacket *p_pkt = (AVPacket*)malloc(sizeof(AVPacket));
        av_init_packet(p_pkt);

		if((iRet = av_read_frame(in_fmt_ctx_v, p_pkt)) >= 0)
		{
//			printf("\n thread:[%lu] VIDEO VIDEO VIDEO VIDEO  frame type=%d duration=%lld pts=%lld\n", thread_id, p_pkt->flags, p_pkt->duration, p_pkt->pts);
//			printf("\nVIDEO VIDEOframe type=%d duration=%lld pts=%llu\n", p_pkt->flags, p_pkt->duration, p_pkt->pts);

			if(videoindex_v == p_pkt->stream_index)//视频帧
			{
				if(1 == p_pkt->flags)//I帧
				{
					time(&time_now);
					if (iIFlag == 0)
					{
						//I帧第一次出现,一般都是异常数据，直接丢弃
						iIFlag++;
						av_packet_free(&p_pkt);
						continue;
					}
					else if (iIFlag == 1)
					{
						//I帧第二次出现,开始累积pts数据用于计算帧间隔
						iIFlag++;
						av_packet_free(&p_pkt);
						continue;
					}
					else if (iIFlag == 2)
					{
						iIFlag++;
						//I帧第二次出现,开始累积pts数据用于计算帧间隔
						//因为是从第三个I帧发送数据，所以开始发送数据帧前计算出pts间隔
//						printf("llPtsOld=%lld\n", llPtsOld);
						if (iCount > 0)
						{
							pDevNode->iPtsRateInterval = (HB_S32)((p_pkt->pts-llPtsOld)/iCount);
							TRACE_YELLOW("thread_id[%lu]-->dev_id[%s]-->dev_Chnl[%d]-->dev_stream_type[%d]-->iPtsRateInterval[%d]\n", \
									thread_id, pDevNode->pDevId, pDevNode->iDevChnl, pDevNode->iDevStreamType, pDevNode->iPtsRateInterval);
						}
						else
						{
							TRACE_ERR("thread_id[%lu] Calc iPtsRateInterval failed! Retry!\n", thread_id);
							iIFlag = 2;
							av_packet_free(&p_pkt);
							continue;
						}
					}
				}
				else //BP帧
				{
					if (iIFlag < 3) //第三个I帧前的数据全部丢弃
					{
						if (iIFlag == 2)
						{
							if (iCount == 0)
							{
								llPtsOld = p_pkt->pts;
	//							printf("p_pkt->pts=%lld\n", p_pkt->pts);
							}
							iCount++;
						}
						av_packet_free(&p_pkt);
						continue;
					}
				}
			}
			else if (audioindex_a == p_pkt->stream_index)//音频帧
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
				//printf("\nOTHER OTHER OTHER OTHER  frame type=%d duration=%lld pts=%lld\n", p_pkt->flags, p_pkt->duration, p_pkt->pts);
				av_packet_free(&p_pkt);
				continue;
			}

#if 1
			BOX_CTRL_CMD_OBJ send_cmd;
			HB_S32 iWriteBufLen = 0;
			list_t *plistRtspClient = (list_t *)&(pDevNode->listRtspClient);
			memset(&send_cmd, 0, sizeof(BOX_CTRL_CMD_OBJ));
			send_cmd.cmd_type = BOX_VIDEO_DATA;
			if(1 == p_pkt->flags)
			{
				send_cmd.data_type = I_FRAME;
			}
			else
			{
				send_cmd.data_type = BP_FRAME;
			}
			send_cmd.cmd_length = p_pkt->size;
	//		send_cmd.pts = pkt->pts;
//			pIndexClientNode = pRtspClientHead->pClientListFirst;

			for (i=0;i<list_size(plistRtspClient);i++)
			{
				//为每个zigbee设备下的传感器进行注册
				CLIENT_LIST_HANDLE pIndexClientNode = (CLIENT_LIST_HANDLE)list_get_at(plistRtspClient, i);
				pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
				if (pIndexClientNode->iDelFlag == 1)
				{
					if (pIndexClientNode->pSendVideoToServerEvent != NULL)
					{
						bufferevent_free(pIndexClientNode->pSendVideoToServerEvent);
						pIndexClientNode->pSendVideoToServerEvent = NULL;
						list_delete(plistRtspClient, (HB_VOID *)pIndexClientNode);
						TRACE_YELLOW("\n###########  total client total client total client total client== [%d]!\n", list_size(plistRtspClient));
					}
				}
				else
				{
					//获取当前缓冲区中已有数据的长度
					iWriteBufLen = evbuffer_get_length(bufferevent_get_output(pIndexClientNode->pSendVideoToServerEvent));
					if ((LIBEVENT_WRITE_BUF_SIZE - iWriteBufLen) < (p_pkt->size + sizeof(BOX_CTRL_CMD_OBJ)))
					{
						printf("send buff full send buff full send buff full!\n");
						//如果当前缓冲区的空间不足以存储一帧数据，则丢帧(清空当前缓冲区)
						evbuffer_drain(bufferevent_get_output(pIndexClientNode->pSendVideoToServerEvent), iWriteBufLen);
						pIndexClientNode->iMissFrameFlag = 1;
						pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
						continue;
					}
					else
					{
						//若缓冲区未满，则判断是否丢过帧，若丢过帧则需要将p帧丢弃，从I帧开始发送
						if ((1 == pIndexClientNode->iMissFrameFlag) && (1 != p_pkt->flags))
						{
							//丢过帧且当前不是I帧，此帧丢弃
//							printf("miss miss miss miss miss miss frame!\n");
							pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
							continue;
						}
					}

					//获取写缓冲的缓冲区大小
	//				int len = bufferevent_get_max_to_write(pIndexClientNode->pSendVideoToServerEvent);
	//				printf("############size:%d\n", len);
					pIndexClientNode->iMissFrameFlag = 0;
					pIndexClientNode->pts += pDevNode->iPtsRateInterval;
					send_cmd.pts = pIndexClientNode->pts;
	//				send_cmd.uiVideoSec = (((HB_U32)(pIndexClientNode->pts))/90)/1000;
	//				send_cmd.uiVideoUsec = (HB_U32)(((pIndexClientNode->pts/90)%1000)*1000);
					bufferevent_write(pIndexClientNode->pSendVideoToServerEvent, &send_cmd, sizeof(BOX_CTRL_CMD_OBJ));
					bufferevent_write(pIndexClientNode->pSendVideoToServerEvent, p_pkt->data, p_pkt->size);
	//				printf("data_type:[%d] ------> data_size:[%d] ------>pts:[%lld]\n", send_cmd.data_type, pkt->size, send_cmd.pts);
				}
				pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			}
			av_packet_free(&p_pkt);
#endif
		}
		else
		{
			TRACE_ERR("av_read_frame() failed!%d\n", iRet);
			av_packet_free(&p_pkt);
			pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
//			destory_client_list(pClientListHead);
			list_destroy(plistRtspClient);
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			break;
		}
    }

End:
	printf("avformat_close_input\n");
	avformat_close_input(&in_fmt_ctx_v);
	printf("avformat_close_input ok!\n");
	pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
	del_one_from_dev_list(pDevNode);
	pDevNode = NULL;
	pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));

	TRACE_ERR("read video thread[%lu] exit!\n", thread_id);
	return 0;
}
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
