/*
 * video_data_list.h
 *
 *  Created on: 2017年4月13日
 *      Author: root
 */

#ifndef VIDEO_DATA_LIST_H_
#define VIDEO_DATA_LIST_H_

#include "simclist.h"

//typedef struct _tagLIST_TEST
//{
//	HB_VOID *p_value;	//存放数据的指针
//}LIST_TEST_OBJ, *LIST_TEST_HANDLE;


typedef struct _tagVIDEO_DATA_LIST
{
//	LIST_HANDLE		plist;			 //链表指针
	list_t listVideoDataList;
	HB_BOOL			b_wait;			 //等待标志位
	pthread_mutex_t		list_mutex;	 //链表互斥锁
	pthread_cond_t		    list_empty;	 //链表空的条件锁
}VIDEO_DATA_LIST_OBJ, *VIDEO_DATA_LIST_HANDLE;


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
HB_S32 video_data_list_init(VIDEO_DATA_LIST_HANDLE video_data_list);

#endif /* VIDEO_DATA_LIST_H_ */
