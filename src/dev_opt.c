/*
 * dev_opt.c
 *
 *  Created on: 2017年9月25日
 *      Author: lijian
 */

#include "dev_opt.h"
#include "dev_list.h"
#include "video_data_list.h"

//#include "libavcodec/avcodec.h"

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
			printf("get a_rtpmap_video:[%s]\n", p_DevNode->a_rtpmap_video);
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
 *	Function: 从设备获取sdp信息并解析
 *
 *	@param bev: 与设备的链接句柄
 *  @parmm args	: 实际为LIBEVENT_ARGS_HANDLE类型的参数结构体
 *
 *	Retrun: 无
 */
static void read_dev_sdp_cb(struct bufferevent *connect_dev_bev, void *arg)
{
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)arg;
	BOX_CTRL_CMD_OBJ st_MsgHead;
	HB_CHAR arr_RecvBuf[2048] = {0};
	HB_CHAR arr_SendBuf[2048] = {0};

	bufferevent_read(connect_dev_bev, arr_RecvBuf, sizeof(arr_RecvBuf));
	bufferevent_free(connect_dev_bev);
	connect_dev_bev = NULL;

	analysis_sdp_info(arr_RecvBuf, pDevNode);

	memset(&st_MsgHead, 0, sizeof(st_MsgHead));
	memcpy(st_MsgHead.header, "hBzHbox@", 8);
	st_MsgHead.cmd_code = CMD_OK;

	memcpy(arr_SendBuf, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ));
	snprintf(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ), 2048-sizeof(BOX_CTRL_CMD_OBJ), \
			"{\"CmdType\":\"sdp_info\",\"m_video\":\"%s\",\"a_rtpmap_video\":\"%s\",\"a_fmtp_video\":\"%s\",\"m_audio\":\"%s\",\"a_rtpmap_audio\":\"%s\"}", \
			pDevNode->m_video, pDevNode->a_rtpmap_video, pDevNode->a_fmtp_video, pDevNode->m_audio, pDevNode->a_rtpmap_audio);

	st_MsgHead.cmd_length = strlen(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ));
	pDevNode->enumDevConnectStatus = CONNECTED;//设置为设备已连接状态

	while(pDevNode->stWaitClientHead.iWaitClientNum > 0)
	{
		struct bufferevent *p_AcceptClient_bev = pDevNode->stWaitClientHead.pWaitClientListFirst->pWaitClientBev;
		bufferevent_write(p_AcceptClient_bev, arr_SendBuf, st_MsgHead.cmd_length+sizeof(BOX_CTRL_CMD_OBJ));
		bufferevent_setcb(p_AcceptClient_bev, deal_client_cmd, NULL, NULL, (HB_VOID *)pDevNode);
		bufferevent_enable(p_AcceptClient_bev, EV_READ);
		pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
		del_one_wait_client(&(pDevNode->stWaitClientHead), pDevNode->stWaitClientHead.pWaitClientListFirst);
		pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
	}

	return;
}


static HB_VOID active_connect_eventcb(struct bufferevent *connect_dev_bev, HB_S16 what, HB_VOID *arg)
{
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)arg;

	if (what & BEV_EVENT_CONNECTED)//盒子主动connect设备成功
	{
		HB_CHAR arr_Discribe[512] = {0};
		snprintf(arr_Discribe, sizeof(arr_Discribe), "DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\n", pDevNode->arrDevRtspUrl);
//		char *discribe = "DESCRIBE rtsp://10.7.126.242:8554/cam/realmonitor?channel=1&subtype=0 RTSP/1.0\r\nCSeq: 4\r\nAuthorization: Basic YWRtaW46MTIzNDU2\r\nUser-Agent: LibVLC/2.2.4 (LIVE555 Streaming Media v2016.02.22)\r\nAccept: application/sdp\r\n\r\n";
//		printf("describe : [%s]\n", arr_Discribe);
		TRACE_GREEN("\n############  connect dev successful and start to get dev SDP !\n");

		bufferevent_write(connect_dev_bev, arr_Discribe, strlen(arr_Discribe)+1);
		bufferevent_enable(connect_dev_bev, EV_WRITE);
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
			destory_wait_client(&(pDevNode->stWaitClientHead));//如果连接设备失败，则需要删除所有正在等待当前设备的用户节点
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
//HB_VOID test_dev_connection(LIBEVENT_ARGS_HANDLE pMessengerArgs)
HB_VOID test_dev_connection(DEV_LIST_HANDLE pDevNode)
{
	const char *in_filename_v = "rtsp://admin:888888@192.168.8.21:8554/H264MainStream";//汉邦ipc
	//const char *in_filename_v = "rtsp://admin:admin@192.168.8.198:554/video1";//宇视ipc video1-主码流 video2-辅码流 video3-第三码流
	//const char *in_filename_v = "rtsp://admin:123456@10.7.126.242:8554/cam/realmonitor?channel=1&subtype=0";//大华
	//const char *in_filename_v = "rtsp://admin:a1234567@10.6.209.93:9020/h264/ch1/main/av_stream";//海康

	struct sockaddr_in connect_to_addr;
	struct bufferevent *connect_dev_bev = bufferevent_socket_new(pEventBase, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);
//	printf("bufferevent_get_enabled(connect_dev_bev):[%d]\n", bufferevent_get_enabled(connect_dev_bev));

	bzero(&connect_to_addr, sizeof(connect_to_addr));
	connect_to_addr.sin_family = AF_INET;
	connect_to_addr.sin_port = htons(pDevNode->iDevRtspPort);
	inet_pton(AF_INET, pDevNode->arrDevIp, (void *)&connect_to_addr.sin_addr);

	snprintf(pDevNode->arrDevRtspUrl, sizeof(pDevNode->arrDevRtspUrl), "%s", in_filename_v);
    struct timeval tv_w;
	tv_w.tv_sec  = 5;
	tv_w.tv_usec = 0;
	bufferevent_set_timeouts(connect_dev_bev, &tv_w, NULL);
	bufferevent_socket_connect(connect_dev_bev, (struct sockaddr*)&connect_to_addr, sizeof(struct sockaddr_in));
	bufferevent_setcb(connect_dev_bev, read_dev_sdp_cb, NULL, active_connect_eventcb, (HB_VOID *)pDevNode);
    bufferevent_enable(connect_dev_bev, EV_READ);
}
/***********************测试设备连通性END***********************/
/***********************测试设备连通性END***********************/
/***********************测试设备连通性END***********************/



/***********************视频流传输***********************/
/***********************视频流传输***********************/
/***********************视频流传输***********************/
#if 1
static HB_VOID *send_video_data_to_rtsp_task(HB_VOID *param)
{
	TRACE_LOG("send_video_data_to_rtsp_task start!");
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)param;
	CLIENT_LIST_HEAD_HANDLE pRtspClientHead = &(pDevNode->stRtspClientHead);
	CLIENT_LIST_HANDLE pIndexClientNode = NULL;
	CLIENT_LIST_HANDLE pTmpClientNode = NULL;
	BOX_CTRL_CMD_OBJ send_cmd;
	memset(&send_cmd, 0, sizeof(BOX_CTRL_CMD_OBJ));
	AVPacket *pkt = NULL;
    while(1)
    {
		if(pRtspClientHead->iClientNum < 1)
		{
			printf("send thread exit p_ClientListHead->i_ClientNum [%d]!\n", pRtspClientHead->iClientNum);
			pthread_exit(NULL);
		}

    	while(pRtspClientHead->stVideoDataList.plist->cnt < 2)
    	{
    		pthread_mutex_lock(&(pRtspClientHead->stVideoDataList.list_mutex));
    		pRtspClientHead->stVideoDataList.b_wait = HB_TRUE;
    		pthread_cond_wait(&(pRtspClientHead->stVideoDataList.list_empty),&(pRtspClientHead->stVideoDataList.list_mutex));
			if(pRtspClientHead->iClientNum < 1)
			{
				printf("send thread exit p_ClientListHead->i_ClientNum [%d]!\n", pRtspClientHead->iClientNum);
				pthread_mutex_unlock(&(pRtspClientHead->stVideoDataList.list_mutex));
				pthread_exit(NULL);
			}
			pRtspClientHead->stVideoDataList.b_wait = HB_FALSE;
    		pthread_mutex_unlock(&(pRtspClientHead->stVideoDataList.list_mutex));
    	}

		pthread_mutex_lock(&(pRtspClientHead->stVideoDataList.list_mutex));
		pkt = (AVPacket*)(pRtspClientHead->stVideoDataList.plist->p_head->p_value);
		send_cmd.cmd_type = BOX_VIDEO_DATA;
		if(1 == pkt->flags)
		{
			send_cmd.data_type = I_FRAME;
		}
		else
		{
			send_cmd.data_type = BP_FRAME;
		}
		send_cmd.cmd_length = pkt->size;

		pIndexClientNode = pRtspClientHead->pClientListFirst;
		while(pIndexClientNode != NULL)
		{
			pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
			if (pIndexClientNode->iDelFlag == 1)
			{
				pTmpClientNode = pIndexClientNode->pNext;
				if (pIndexClientNode->pSendVideoToServerEvent != NULL)
				{
					bufferevent_free(pIndexClientNode->pSendVideoToServerEvent);
					pIndexClientNode->pSendVideoToServerEvent = NULL;
					del_one_client(pRtspClientHead, pIndexClientNode);
				}
				pIndexClientNode = pTmpClientNode;
			}
			else
			{
				bufferevent_write(pIndexClientNode->pSendVideoToServerEvent, &send_cmd, sizeof(BOX_CTRL_CMD_OBJ));
				bufferevent_write(pIndexClientNode->pSendVideoToServerEvent, pkt->data, pkt->size);
				pIndexClientNode = pIndexClientNode->pNext;
			}
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
		}

		pkt = NULL;
		list_remove_head_node(pRtspClientHead->stVideoDataList.plist);
		pthread_mutex_unlock(&(pRtspClientHead->stVideoDataList.list_mutex));
    }

    return NULL;
}

static int interrupt_cb(void *ctx)
{
    // do something
//	AVFormatContext *pFormatCtx = (AVFormatContext *)ctx;
	time_t *time_last = (time_t *)ctx;
	time_t time_now = 0;

	time(&time_now);
//	printf("time_last=[%ld], time_now=[%ld]\n", *time_last, time_now);
    if ((time_now-*time_last) > 5)
    {
		printf("av_read_frame time out\n");
		return AVERROR_EOF;//这个就是超时的返回
//		return 1;
    }

    return 0;
}

//从设备读取视频流线程
HB_VOID *read_video_data_from_dev_task(HB_VOID *arg)
{
	pthread_detach(pthread_self());
	DEV_LIST_HANDLE pDevNode = (DEV_LIST_HANDLE)arg;
	CLIENT_LIST_HEAD_HANDLE pClientListHead = &(pDevNode->stRtspClientHead);
	pClientListHead->iStartThreadFlag = 1;

	time_t  time_now = time(NULL);;

	HB_S32 read_video_data_node_task_flag = 0;
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
//    av_dict_set(&options, "stimeout", "10000000", 0);
    ret = avformat_open_input(&in_fmt_ctx_v, pDevNode->arrDevRtspUrl, NULL, &options);
    if(ret != 0)
    {
    	ret = avformat_open_input(&in_fmt_ctx_v, pDevNode->arrDevRtspUrl, NULL, NULL);
        if(ret != 0)
        {
			printf("\n0000000000000avformat_open_input failed ret=%d\n", ret);
			av_dict_free(&options);
			goto End;
        }

    }
    av_dict_free(&options);

//    printf("Open rtsp succeed!\n");
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
    printf("\n####  open rtsp successful, video_index=%d  audio_index=%d\n", videoindex_v, audioindex_a);
    video_data_list_init(&(pClientListHead->stVideoDataList));

#if 1
	//pthread_create(&p_ClientListHead->thread_SendVideo_id, NULL, send_video_data_to_rtsp_task, (HB_VOID*)p_CommunicateTags);
    pthread_create(&(pClientListHead->threadSendVideoId), NULL, send_video_data_to_rtsp_task, (HB_VOID*)pDevNode);
	read_video_data_node_task_flag = 1;
	printf("\n#########  pthread id = %lu\n", pClientListHead->threadSendVideoId);
#endif
    HB_S32 I_flag = 0;
    while (1)
    {
    	if(pClientListHead->iClientNum < 1)
    	{
    		printf("p_ClientListHead->i_ClientNum=[%d]\n", pClientListHead->iClientNum);
    		break;
    	}
    	AVPacket *p_pkt = (AVPacket*)malloc(sizeof(AVPacket));
        av_init_packet(p_pkt);

		if(av_read_frame(in_fmt_ctx_v, p_pkt) >= 0)
		{
			time(&time_now);
			if(videoindex_v == p_pkt->stream_index)//视频帧
			{
				if(1 == p_pkt->flags)//I帧
				{
					I_flag = 1;
					//printf("total client [%d]\n", p_ClientListHead->i_ClientNum);
					printf("\nVIDEO VIDEOframe type=%d duration=%lld pts=%lld\n", p_pkt->flags, p_pkt->duration, p_pkt->pts);
				}
				else if (0 == I_flag)//第一个I帧来之前BP帧全部丢弃
				{
					av_packet_free(&p_pkt);
					continue;
				}
				//printf("\nVIDEO VIDEO VIDEO VIDEO  frame type=%d duration=%lld pts=%lld\n", p_pkt->flags, p_pkt->duration, p_pkt->pts);
				//frame_type = 0;
			}
			else if (audioindex_a == p_pkt->stream_index)//音频帧
			{
				av_packet_free(&p_pkt);
				continue;
				//第一个I帧来之前，音频也不播放
//				if(0 == I_flag)
//				{
//					av_packet_free(&p_pkt);
//					continue;
//				}
				//frame_type = 1;
				//printf("\nAUDIO AUDIO AUDIO AUDIO  frame type=%d duration=%lld pts=%lld\n", p_pkt->flags, p_pkt->duration, p_pkt->pts);
				//av_packet_free(&p_pkt);
				//continue;
			}
			else	//其它类型帧直接忽略
			{
				//printf("\nOTHER OTHER OTHER OTHER  frame type=%d duration=%lld pts=%lld\n", p_pkt->flags, p_pkt->duration, p_pkt->pts);
				av_packet_free(&p_pkt);
				continue;
			}

			LIST_NODE_HANDLE video_data_node = NULL;
			pthread_mutex_lock(&(pClientListHead->stVideoDataList.list_mutex));
			video_data_node = (LIST_NODE_HANDLE)list_add_node_to_end(pClientListHead->stVideoDataList.plist);
			video_data_node->p_value = p_pkt;
			//sonser_data_node->buf_len = sen_data.length;
			pthread_mutex_unlock(&(pClientListHead->stVideoDataList.list_mutex));

			if(HB_TRUE == pClientListHead->stVideoDataList.b_wait)
			{
				pthread_mutex_lock(&(pClientListHead->stVideoDataList.list_mutex));
				pthread_cond_signal(&(pClientListHead->stVideoDataList.list_empty));
				pthread_mutex_unlock(&(pClientListHead->stVideoDataList.list_mutex));
			}
		}
		else
		{
			printf("av_read_frame() failed!\n");
//			iAvReadFrameFailedNum++;
//			if (iAvReadFrameFailedNum < 25)
//			{
//				//连续错误小于25帧视为正常丢帧
//				continue;
//			}
			pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
			destory_client_list(pClientListHead);
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			break;
		}
    }

End:
	avformat_close_input(&in_fmt_ctx_v);
	if(1 == read_video_data_node_task_flag)
	{
		printf("join send thread!\n");
		read_video_data_node_task_flag = 0;
		pthread_mutex_lock(&(pClientListHead->stVideoDataList.list_mutex));
		pthread_cond_signal(&(pClientListHead->stVideoDataList.list_empty));
		pthread_mutex_unlock(&(pClientListHead->stVideoDataList.list_mutex));
		pthread_join(pClientListHead->threadSendVideoId, NULL);//等待发送rtp包线程退出
	}
	pthread_mutex_lock(&(pClientListHead->stVideoDataList.list_mutex));
	list_destroy(pClientListHead->stVideoDataList.plist);
	pthread_mutex_unlock(&(pClientListHead->stVideoDataList.list_mutex));

	pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
	del_one_from_dev_list(pDevNode);
	pDevNode = NULL;
	pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));

	TRACE_BLUE("read video thread exit!\n");
	pthread_exit(NULL);
}
#endif
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
