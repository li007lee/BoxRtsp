/*
 * deal_dev_msg.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include "deal_dev_msg.h"
#include "client_list.h"
#include "video_data_list.h"

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

//static void read_dev_sdp_cb(struct bufferevent *connect_dev_bev, void *arg)
void read_dev_sdp_cb(struct bufferevent *connect_dev_bev, void *arg)
{
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)arg;
	DEV_LIST_HANDLE p_DevNode = p_CommunicateTags->p_DevNode;
	struct bufferevent *p_AcceptClient_bev = p_CommunicateTags->client_bev;
	BOX_CTRL_CMD_OBJ st_MsgHead;
	HB_CHAR arr_RecvBuf[2048] = {0};
	HB_CHAR arr_SendBuf[2048] = {0};

	//p_DevNode->p_EventDev = connect_dev_bev;
	bufferevent_read(connect_dev_bev, arr_RecvBuf, sizeof(arr_RecvBuf));
	printf("sdp sdp sdp sdp sdp [%s]\n", arr_RecvBuf);
	bufferevent_free(connect_dev_bev);

	analysis_sdp_info(arr_RecvBuf, p_DevNode);

	memset(&st_MsgHead, 0, sizeof(st_MsgHead));
	memcpy(st_MsgHead.header, "hBzHbox@", 8);
	st_MsgHead.cmd_code = CMD_OK;

	memcpy(arr_SendBuf, &st_MsgHead, sizeof(BOX_CTRL_CMD_OBJ));
	snprintf(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ), 2048-sizeof(BOX_CTRL_CMD_OBJ), \
			"{\"CmdType\":\"sdp_info\",\"m_video\":\"%s\",\"a_rtpmap_video\":\"%s\",\"a_fmtp_video\":\"%s\",\"m_audio\":\"%s\",\"a_rtpmap_audio\":\"%s\"}", \
			p_DevNode->m_video, p_DevNode->a_rtpmap_video, p_DevNode->a_fmtp_video, p_DevNode->m_audio, p_DevNode->a_rtpmap_audio);

	st_MsgHead.cmd_length = strlen(arr_SendBuf+sizeof(BOX_CTRL_CMD_OBJ));

	printf("p_DevNode->p_EventDev addr : [%p]\n", p_CommunicateTags->client_bev);
	p_CommunicateTags->enum_DataType = CMD_HEAD;
	bufferevent_write(p_AcceptClient_bev, arr_SendBuf, st_MsgHead.cmd_length+sizeof(BOX_CTRL_CMD_OBJ));
	bufferevent_setcb(p_AcceptClient_bev, deal_client_request, NULL, deal_server_info_error_cb, (HB_VOID *)p_CommunicateTags);
	bufferevent_enable(p_AcceptClient_bev, EV_READ);

	return;
}

static HB_VOID active_connect_eventcb(struct bufferevent *connect_dev_bev, HB_S16 what, HB_VOID *arg)
{
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)arg;
	DEV_LIST_HANDLE p_DevNode = p_CommunicateTags->p_DevNode;

	if (what & BEV_EVENT_CONNECTED)//盒子主动connect设备成功
	{
		HB_CHAR arr_Discribe[512] = {0};

		p_DevNode->connecet_status = CONNECTED;
		snprintf(arr_Discribe, sizeof(arr_Discribe), "DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\n", p_DevNode->arr_DevRtspUrl);

		//char *discribe = "DESCRIBE rtsp://10.7.126.242:8554/cam/realmonitor?channel=1&subtype=0 RTSP/1.0\r\nCSeq: 4\r\nAuthorization: Basic YWRtaW46MTIzNDU2\r\nUser-Agent: LibVLC/2.2.4 (LIVE555 Streaming Media v2016.02.22)\r\nAccept: application/sdp\r\n\r\n";

		printf("describe : [%s]\n", arr_Discribe);
		printf("\n############  connect dev successful and start get dev SDP !\n");
		bufferevent_write(connect_dev_bev, arr_Discribe, strlen(arr_Discribe)+1);
		bufferevent_setcb(connect_dev_bev, read_dev_sdp_cb, NULL, NULL, (HB_VOID *)p_CommunicateTags);
		bufferevent_enable(connect_dev_bev, EV_READ|EV_WRITE);

		return;
	}
	//else if (what & BEV_EVENT_ERROR)  //盒子connect 设备失败
	else
	{
		bufferevent_disable(p_CommunicateTags->client_bev, EV_READ|EV_WRITE);
		bufferevent_disable(connect_dev_bev, EV_READ|EV_WRITE);
		printf("\n###########  box connect dev  failed !\n");

		pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
		remove_one_from_dev_list(p_DevNode);
		p_DevNode = NULL;
		pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
		p_CommunicateTags->p_DevNode = NULL;
		bufferevent_free(p_CommunicateTags->client_bev);

		free(p_CommunicateTags);
		bufferevent_free(connect_dev_bev);
	}
}

HB_S32 test_dev_connection(CLIENT_INFO_HANDLE p_CommunicateTags)
{
	//const char *in_filename_v = "rtsp://192.168.8.120:8554/rtsp_test";
	const char *in_filename_v = "rtsp://admin:888888@192.168.8.21:8554/H264MainStream";//汉邦ipc
	//const char *in_filename_v = "rtsp://admin:admin@192.168.8.198:554/video1";//宇视ipc video1-主码流 video2-辅码流 video3-第三码流
	//const char *in_filename_v = "rtsp://admin:123456@10.7.126.242:8554/cam/realmonitor?channel=1&subtype=0";//大华

//	const char *in_filename_v = "rtsp://admin:a1234567@10.6.209.93:9020/h264/ch1/main/av_stream";//海康

	DEV_LIST_HANDLE p_DevNode = p_CommunicateTags->p_DevNode;
	struct bufferevent *client_bev = p_CommunicateTags->client_bev;
	HB_S32 connect_to_addrlen;
	struct sockaddr_in connect_to_addr;
	struct bufferevent *connect_dev_bev = NULL;
	struct event_base *base = bufferevent_get_base(client_bev);

	connect_dev_bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);
	bzero(&connect_to_addr, sizeof(connect_to_addr));
	connect_to_addr.sin_family = AF_INET;
	connect_to_addr.sin_port = htons(p_DevNode->i_DevRtspPort);
	inet_pton(AF_INET, p_DevNode->arr_DevIp, &connect_to_addr.sin_addr);

	//snprintf(p_DevNode->arr_DevRtspUrl, sizeof(p_DevNode->arr_DevRtspUrl), "DESCRIBE %s RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\n", in_filename_v);
	snprintf(p_DevNode->arr_DevRtspUrl, sizeof(p_DevNode->arr_DevRtspUrl), "%s", in_filename_v);
	connect_to_addrlen = sizeof(struct sockaddr_in);

	bufferevent_setcb(connect_dev_bev, NULL, NULL, active_connect_eventcb, (HB_VOID *)p_CommunicateTags);
//	p_DevNode->connecet_status = CONNECTING;

	if (bufferevent_socket_connect(connect_dev_bev, (struct sockaddr*)&connect_to_addr, connect_to_addrlen) < 0)//非阻塞连接设备端
	{
		bufferevent_free(connect_dev_bev);
		connect_dev_bev = NULL;
//		pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
		return -1;
	}
//	bufferevent_enable(connect_dev_bev, EV_READ);

	return 0;
}
/***********************测试设备连通性END***********************/
/***********************测试设备连通性END***********************/
/***********************测试设备连通性END***********************/



/***********************视频流传输***********************/
/***********************视频流传输***********************/
/***********************视频流传输***********************/

static HB_VOID *send_video_data_to_rtsp_task(HB_VOID *param)
{
	TRACE_LOG("send_video_data_to_rtsp_task start!");
	HB_S32 i = 0;
	DEV_LIST_HANDLE p_DevNode = (CLIENT_INFO_HANDLE)param;
	CLIENT_LIST_HEAD_HANDLE p_ClientListHead = &(p_DevNode->st_ClientListHead);
//	CLIENT_LIST_HANDLE p_ClientListFirst = p_ClientListHead->p_ClientListFirst;
	CLIENT_LIST_HANDLE p_IndexClientNode = NULL;
	CLIENT_LIST_HANDLE p_TmpClientNode = NULL;
	BOX_CTRL_CMD_OBJ send_cmd;
	memset(&send_cmd, 0, sizeof(BOX_CTRL_CMD_OBJ));
	AVPacket *pkt = NULL;
    while(1)
    {
		if(p_ClientListHead->i_ClientNum < 1)
		{
			printf("send thread exit p_ClientListHead->i_ClientNum [%d]!\n", p_ClientListHead->i_ClientNum);
			//pthread_mutex_unlock(&(p_ClientListHead->video_data_list.list_mutex));
			pthread_exit(NULL);
		}

    	while(p_ClientListHead->video_data_list.plist->cnt < 2)
    	{
    		pthread_mutex_lock(&(p_ClientListHead->video_data_list.list_mutex));
    		p_ClientListHead->video_data_list.b_wait = HB_TRUE;
    		pthread_cond_wait(&(p_ClientListHead->video_data_list.list_empty),&(p_ClientListHead->video_data_list.list_mutex));
			if(p_ClientListHead->i_ClientNum < 1)
			{
				printf("send thread exit p_ClientListHead->i_ClientNum [%d]!\n", p_ClientListHead->i_ClientNum);
				pthread_mutex_unlock(&(p_ClientListHead->video_data_list.list_mutex));
				pthread_exit(NULL);
			}
    		p_ClientListHead->video_data_list.b_wait = HB_FALSE;
    		pthread_mutex_unlock(&(p_ClientListHead->video_data_list.list_mutex));
    	}

		pthread_mutex_lock(&(p_ClientListHead->video_data_list.list_mutex));
		pkt = (AVPacket*)(p_ClientListHead->video_data_list.plist->p_head->p_value);
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


//		p_IndexClientNode = p_ClientListFirst;
		p_IndexClientNode = p_ClientListHead->p_ClientListFirst;
		while(p_IndexClientNode != NULL)
		{
			pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
			if (p_IndexClientNode->del_flag == 1)
			{
				p_TmpClientNode = p_IndexClientNode->p_Next;
				if (p_IndexClientNode->p_SendVideoToServerEvent != NULL)
				{
					bufferevent_free(p_IndexClientNode->p_SendVideoToServerEvent);
					p_IndexClientNode->p_SendVideoToServerEvent = NULL;
					remove_one_client(p_ClientListHead, p_IndexClientNode);
				}
				p_IndexClientNode = p_TmpClientNode;
			}
//			else if (p_IndexClientNode->del_flag == 0)
			else
			{
//				evbuffer_freeze(p_IndexClientNode->p_SendVideoToServerEvent->output, 1);
				if (p_IndexClientNode->p_SendVideoToServerEvent == NULL)
				{
					printf("lalalalaalalalalalalalal\n");
					p_IndexClientNode = p_IndexClientNode->p_Next;
					pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
					continue;
				}
				bufferevent_write(p_IndexClientNode->p_SendVideoToServerEvent, &send_cmd, sizeof(BOX_CTRL_CMD_OBJ));
				bufferevent_write(p_IndexClientNode->p_SendVideoToServerEvent, pkt->data, pkt->size);
//				evbuffer_unfreeze(p_IndexClientNode->p_SendVideoToServerEvent->output, 1);
				p_IndexClientNode = p_IndexClientNode->p_Next;
			}
			pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
		}

		pkt = NULL;
		list_remove_head_node(p_ClientListHead->video_data_list.plist);
		pthread_mutex_unlock(&(p_ClientListHead->video_data_list.list_mutex));
    }

    return NULL;
}

//从设备读取视频流线程
HB_VOID *read_video_data_from_dev_task(HB_VOID *param)
{
	pthread_detach(pthread_self());
	CLIENT_INFO_HANDLE p_CommunicateTags = (CLIENT_INFO_HANDLE)param;
	DEV_LIST_HANDLE p_DevNode = p_CommunicateTags->p_DevNode;
	CLIENT_LIST_HEAD_HANDLE p_ClientListHead = &(p_DevNode->st_ClientListHead);

	p_ClientListHead->start_thread_flag = 1;
	HB_S32 read_video_data_node_task_flag = 0;

    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *in_fmt_ctx_v = avformat_alloc_context();
    int ret, i;
    int videoindex_v=-1;
    int audioindex_a=-1;
	AVDictionary* options = NULL;
    //设置rtsp传输模式为tcp
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    ret = avformat_open_input(&in_fmt_ctx_v, p_DevNode->arr_DevRtspUrl, NULL, &options);
    if(ret != 0)
    {
    	ret = avformat_open_input(&in_fmt_ctx_v, p_DevNode->arr_DevRtspUrl, NULL, NULL);
        if(ret != 0)
        {
			printf("\n0000000000000 ret=%d\n", ret);
			av_dict_free(&options);
			goto End;
        }

    }
    av_dict_free(&options);

    printf("Open rtsp succeed!\n");
    for (i = 0; i < in_fmt_ctx_v->nb_streams; i++)
    {
        //Create output AVStream according to input AVStream
        if(in_fmt_ctx_v->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
			AVStream *in_stream = in_fmt_ctx_v->streams[i];
			printf("\nBBBBBBBBB   [frame rate = %lf]  [%d*%d]\n", \
					(in_stream->avg_frame_rate.num/(double)(in_stream->avg_frame_rate.den)), \
					in_stream->codec->width, in_stream->codec->height);
			videoindex_v=i;
			//break;
        }
        else if(in_fmt_ctx_v->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
        {
			audioindex_a=i;
			//break;
        }

    }
    printf("\n####  open rtsp successful, video_index=%d  audio_index=%d\n", videoindex_v, audioindex_a);
    video_data_list_init(&(p_ClientListHead->video_data_list));

#if 1
	//pthread_create(&p_ClientListHead->thread_SendVideo_id, NULL, send_video_data_to_rtsp_task, (HB_VOID*)p_CommunicateTags);
    pthread_create(&p_ClientListHead->thread_SendVideo_id, NULL, send_video_data_to_rtsp_task, (HB_VOID*)p_DevNode);
	read_video_data_node_task_flag = 1;
	printf("\n#########  pthread id = %lu\n", p_ClientListHead->thread_SendVideo_id);
#endif
    HB_S32 I_flag = 0;
    while (1)
    {
    	if(p_ClientListHead->i_ClientNum < 1)
    	{
    		printf("p_ClientListHead->i_ClientNum=[%d]\n", p_ClientListHead->i_ClientNum);
    		goto End;
    	}
    	AVPacket *p_pkt = (AVPacket*)malloc(sizeof(AVPacket));
        av_init_packet(p_pkt);

		if(av_read_frame(in_fmt_ctx_v, p_pkt) >= 0)
		{
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
#if 1
			//printf("+++++++++++++++++data in video list [%d]\n", p_ClientListHead->video_data_list.plist->cnt);
			LIST_NODE_HANDLE video_data_node = NULL;
			pthread_mutex_lock(&(p_ClientListHead->video_data_list.list_mutex));
			video_data_node = (LIST_NODE_HANDLE)list_add_node_to_end(p_ClientListHead->video_data_list.plist);
			video_data_node->p_value = p_pkt;
			//sonser_data_node->buf_len = sen_data.length;
			pthread_mutex_unlock(&(p_ClientListHead->video_data_list.list_mutex));

			if(HB_TRUE == p_ClientListHead->video_data_list.b_wait)
			{
				pthread_mutex_lock(&(p_ClientListHead->video_data_list.list_mutex));
				pthread_cond_signal(&(p_ClientListHead->video_data_list.list_empty));
				pthread_mutex_unlock(&(p_ClientListHead->video_data_list.list_mutex));
			}
#endif
		}
		else
		{
			printf("av_read_frame() failed!\n");
			break;
		}
    }

End:
	if(1 == read_video_data_node_task_flag)
	{
		printf("join send thread!\n");
		pthread_mutex_lock(&(p_ClientListHead->video_data_list.list_mutex));
		pthread_cond_signal(&(p_ClientListHead->video_data_list.list_empty));
		pthread_mutex_unlock(&(p_ClientListHead->video_data_list.list_mutex));
		pthread_join(p_ClientListHead->thread_SendVideo_id, NULL);//等待发送rtp包线程退出
	}
	//pthread_mutex_lock(&(p_ClientListHead->video_data_list.list_mutex));
	list_destroy(p_ClientListHead->video_data_list.plist);
	//pthread_mutex_unlock(&(p_ClientListHead->video_data_list.list_mutex));

	//	for (i=0; i<p_ClientListHead->i_ClientNum; i++)
//	{
//		bufferevent_free(p_ClientListHead->p_ClientListFirst->p_SendVideoToServerEvent);
//	}
	//destory_client_list(&(p_DevNode->st_ClientListHead));
	pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
	remove_one_from_dev_list(p_DevNode);
	pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
	free(p_CommunicateTags);
	p_CommunicateTags = NULL;
	avformat_close_input(&in_fmt_ctx_v);

	printf("read video thread exit!\n");
    return NULL;
}

/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/


