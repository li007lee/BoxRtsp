/*
 * rtsp_server_list.h
 *
 *  Created on: 2017年5月23日
 *      Author: root
 */

#ifndef CLIENT_LIST_H_
#define CLIENT_LIST_H_


#include "my_include.h"
#include "simclist.h"


//视频缓冲结构体
typedef struct _tagVIDEO_DATA_LIST
{
	list_t listVideoDataList;
	HB_BOOL			b_wait;			 //等待标志位
	pthread_mutex_t		list_mutex;	 //链表互斥锁
	pthread_cond_t		    list_empty;	 //链表空的条件锁
}VIDEO_DATA_LIST_OBJ, *VIDEO_DATA_LIST_HANDLE;



/////////////////////////////////////////////////////////////////////////////////
// RTSP服务器链表数据结构
/////////////////////////////////////////////////////////////////////////////////
//已经接入的用户的链表
typedef struct _tagCLIENT_LIST
{
	HB_S32 iDelFlag;//节点删除标志,客户端开连接或者网络异常时会被置位，为1时表明需要删除当前客户节点
	HB_S32 iMissFrameFlag;	//丢帧表示1表示丢过帧，0表示为丢过
	HB_S64 pts;
	struct bufferevent *pSendVideoToServerEvent;//主动连接服务器事件（视频流推送事件句柄）

	struct _tagCLIENT_LIST *pPrev;
	struct _tagCLIENT_LIST *pNext;

}CLIENT_LIST_OBJ, *CLIENT_LIST_HANDLE;

typedef struct CLIENT_LIST_HEAD
{
	HB_S32 iClientNum;	//当前用户数
	pthread_t threadReadVideoId; //视频读取线程id
	pthread_t threadSendVideoId; //视频发送线程id

	HB_S32 iStartThreadFlag; //视频流收发线程启动标志位，1 已启动，0未启动

	VIDEO_DATA_LIST_OBJ stVideoDataList;//存储视频流的链表

	CLIENT_LIST_HANDLE pClientListFirst;//第一个客户节点
	CLIENT_LIST_HANDLE pClientListLast;//最后一个客户节点

}CLIENT_LIST_HEAD_OBJ, *CLIENT_LIST_HEAD_HANDLE;


HB_S32	add_client_in_tail(CLIENT_LIST_HEAD_HANDLE pClientListHead, CLIENT_LIST_HANDLE pNewNode);
HB_S32 del_one_client(CLIENT_LIST_HEAD_HANDLE pClientListHead, CLIENT_LIST_HANDLE pDelNode);
HB_VOID destory_client_list(CLIENT_LIST_HEAD_HANDLE pClientListHead);

// 描述：视频缓冲数据链表初始化
HB_S32 video_data_list_init(VIDEO_DATA_LIST_HANDLE video_data_list);
#endif /* CLIENT_LIST_H_ */
