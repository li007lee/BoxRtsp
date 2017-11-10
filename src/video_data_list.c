/*
 * video_data_list.c
 *
 *  Created on: 2017年4月13日
 *      Author: root
 */

#include "my_include.h"
#include "video_data_list.h"


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
	list_init(&(video_data_list->listVideoDataList));    /* 初始化链表 */
//	list_attributes_copy(&(video_data_list->listVideoDataList), element_meter, 1);
	video_data_list->b_wait = HB_FALSE;
	pthread_mutex_init(&video_data_list->list_mutex, NULL);
	pthread_cond_init(&(video_data_list->list_empty), NULL);

	return HB_SUCCESS;
}
