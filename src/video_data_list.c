/*
 * video_data_list.c
 *
 *  Created on: 2017年4月13日
 *      Author: root
 */

#include "my_include.h"
#include "libavcodec/avcodec.h"
#include "list.h"
#include "video_data_list.h"

HB_VOID delete_vedio_data_node(HB_VOID *p_node)
{
	LIST_NODE_HANDLE handle = (LIST_NODE_HANDLE)p_node;
//	static int num = 0;

	if (NULL != handle->p_value)
	{
		av_packet_free((AVPacket**)(&(handle->p_value)));
		handle->p_value = NULL;
//		printf("free free free av_packet = %d\n", ++num);
	}

	return;
}


////////////////////////////////////////////////////////////////////////////////
// 函数名：video_data_list_init
// 描述：视频缓冲数据链表初始化
// 参数：
//  ［IN］
// 返回值：
//  	HB_FAILURE - 失败
//		HB_SUCCESS - 成功
// 说明：
////////////////////////////////////////////////////////////////////////////////
HB_S32 video_data_list_init(VIDEO_DATA_LIST_HANDLE video_data_list)
{
	video_data_list->plist = list_create();
	//vide_data_list.plist->new_node = create_sensor_data_node;
	video_data_list->plist->del_node = delete_vedio_data_node;
	video_data_list->b_wait = HB_FALSE;
	video_data_list->plist->stop_send_flag = 0;
	//list_reset(ele_data_list->plist);
	//list_add_node_to_end(ele_data_list->plist);
	pthread_mutex_init(&video_data_list->list_mutex, NULL);
	pthread_cond_init(&(video_data_list->list_empty), NULL);

	return HB_SUCCESS;
}
