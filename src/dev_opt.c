/*
 * dev_opt.c
 *
 *  Created on: 2017年9月25日
 *      Author: lijian
 */

#include "dev_opt.h"
#include "dev_list.h"
#include "client_list.h"

/***********************视频流传输***********************/
/***********************视频流传输***********************/
/***********************视频流传输***********************/
#if 1
static HB_VOID *send_video_data_to_rtsp_task(HB_VOID *param)
{
	HB_S32 iWriteBufLen = 0;
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

    	while(list_size(&(pRtspClientHead->stVideoDataList.listVideoDataList)) < 2)
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
		pkt = (AVPacket*)list_get_at(&(pRtspClientHead->stVideoDataList.listVideoDataList) , 0);
//		pthread_mutex_unlock(&(pRtspClientHead->stVideoDataList.list_mutex));

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
//		send_cmd.pts = pkt->pts;
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
				//获取当前缓冲区中已有数据的长度
				iWriteBufLen = evbuffer_get_length(bufferevent_get_output(pIndexClientNode->pSendVideoToServerEvent));
				if ((LIBEVENT_WRITE_BUF_SIZE - iWriteBufLen) < (pkt->size + sizeof(BOX_CTRL_CMD_OBJ)))
				{
					printf("send buff full send buff full send buff full!\n");
					//如果当前缓冲区的空间不足以存储一帧数据，则丢帧
					pIndexClientNode->iMissFrameFlag = 1;
					pIndexClientNode = pIndexClientNode->pNext;
					pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
					continue;
				}
				else
				{
					//若缓冲区未满，则判断是否丢过帧，若丢过帧则需要将p帧丢弃，从I帧开始发送
					if ((1 == pIndexClientNode->iMissFrameFlag) && (1 != pkt->flags))
					{
						//丢过帧且当前不是I帧，此帧丢弃
						printf("miss miss miss miss miss miss frame!\n");
						pIndexClientNode = pIndexClientNode->pNext;
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
				bufferevent_write(pIndexClientNode->pSendVideoToServerEvent, pkt->data, pkt->size);
//				printf("data_type:[%d] ------> data_size:[%d] ------>pts:[%lld]\n", send_cmd.data_type, pkt->size, send_cmd.pts);
				pIndexClientNode = pIndexClientNode->pNext;

			}
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
		}

		av_packet_free(&pkt);
//		pthread_mutex_lock(&(pRtspClientHead->stVideoDataList.list_mutex));
		list_delete_at(&(pRtspClientHead->stVideoDataList.listVideoDataList), 0);
		pthread_mutex_unlock(&(pRtspClientHead->stVideoDataList.list_mutex));
//		printf("total node : [%d]\n", list_size(&(pRtspClientHead->stVideoDataList.listVideoDataList)));
    }

    return NULL;
}

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
	CLIENT_LIST_HEAD_HANDLE pClientListHead = &(pDevNode->stRtspClientHead);
	HB_CHAR arrOpenRtspUrl[1024] = {0};
	pClientListHead->iStartThreadFlag = 1;

	pthread_t thread_id = pthread_self();

	TRACE_YELLOW("thread_id[%lu]--->dev_id[%s]--->dev_Chnl[%d]--->dev_stream_type[%d]\n", \
			thread_id, pDevNode->pDevId, pDevNode->iDevChnl, pDevNode->iDevStreamType);

	time_t  time_now = time(NULL);

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

    video_data_list_init(&(pClientListHead->stVideoDataList));
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

#if 1
    pthread_create(&(pClientListHead->threadSendVideoId), NULL, send_video_data_to_rtsp_task, (HB_VOID*)pDevNode);
	read_video_data_node_task_flag = 1;
#endif
	HB_S64 iIFlag = 0; //由于前几帧的pts经常出错，此变量用于忽略第两个个I帧，从第三I帧开始发送数据
    HB_S32 iCount = 0;
    HB_S64 llPtsOld = 0;
    int iRet = 0;
    while (1)
    {
    	if(pClientListHead->iClientNum < 1)
    	{
    		TRACE_YELLOW("p_ClientListHead->i_ClientNum=[%d]\n", pClientListHead->iClientNum);
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

#if 0
			BOX_CTRL_CMD_OBJ send_cmd;
			HB_S32 iWriteBufLen = 0;
			CLIENT_LIST_HEAD_HANDLE pRtspClientHead = &(pDevNode->stRtspClientHead);
			CLIENT_LIST_HANDLE pIndexClientNode = NULL;
			CLIENT_LIST_HANDLE pTmpClientNode = NULL;
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
					//获取当前缓冲区中已有数据的长度
					iWriteBufLen = evbuffer_get_length(bufferevent_get_output(pIndexClientNode->pSendVideoToServerEvent));
					if ((LIBEVENT_WRITE_BUF_SIZE - iWriteBufLen) < (p_pkt->size + sizeof(BOX_CTRL_CMD_OBJ)))
					{
						printf("send buff full send buff full send buff full!\n");
						//如果当前缓冲区的空间不足以存储一帧数据，则丢帧
						pIndexClientNode->iMissFrameFlag = 1;
						pIndexClientNode = pIndexClientNode->pNext;
						pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
						continue;
					}
					else
					{
						//若缓冲区未满，则判断是否丢过帧，若丢过帧则需要将p帧丢弃，从I帧开始发送
						if ((1 == pIndexClientNode->iMissFrameFlag) && (1 != p_pkt->flags))
						{
							//丢过帧且当前不是I帧，此帧丢弃
							printf("miss miss miss miss miss miss frame!\n");
							pIndexClientNode = pIndexClientNode->pNext;
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
					pIndexClientNode = pIndexClientNode->pNext;

				}
				pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			}

			av_packet_free(&p_pkt);
#endif

#if 1
			pthread_mutex_lock(&(pClientListHead->stVideoDataList.list_mutex));
			list_append(&(pClientListHead->stVideoDataList.listVideoDataList), p_pkt);
			pthread_mutex_unlock(&(pClientListHead->stVideoDataList.list_mutex));
			if(HB_TRUE == pClientListHead->stVideoDataList.b_wait)
			{
				pthread_mutex_lock(&(pClientListHead->stVideoDataList.list_mutex));
				pthread_cond_signal(&(pClientListHead->stVideoDataList.list_empty));
				pthread_mutex_unlock(&(pClientListHead->stVideoDataList.list_mutex));
			}
#endif
		}
		else
		{
			TRACE_ERR("av_read_frame() failed!%d\n", iRet);
			av_packet_free(&p_pkt);
			pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
			destory_client_list(pClientListHead);
			pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));
			break;
		}
    }

End:
	printf("avformat_close_input\n");
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
	list_destroy(&(pClientListHead->stVideoDataList.listVideoDataList));
//	printf("destory total node : [%d]\n", list_size(&(pClientListHead->stVideoDataList.listVideoDataList)));
//	list_destroy(pClientListHead->stVideoDataList.plist);
	pthread_mutex_unlock(&(pClientListHead->stVideoDataList.list_mutex));

	pthread_mutex_lock(&(stDevListHead.mutexDevListMutex));
	del_one_from_dev_list(pDevNode);
	pDevNode = NULL;
	pthread_mutex_unlock(&(stDevListHead.mutexDevListMutex));

	TRACE_ERR("read video thread[%lu] exit!\n", thread_id);
	pthread_exit(NULL);
}
#endif
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
/***********************视频流传输END***********************/
