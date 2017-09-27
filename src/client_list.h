/*
 * rtsp_server_list.h
 *
 *  Created on: 2017年5月23日
 *      Author: root
 */

#ifndef CLIENT_LIST_H_
#define CLIENT_LIST_H_


#include "my_include.h"
#include "video_data_list.h"


/////////////////////////////////////////////////////////////////////////////////
// RTSP服务器链表数据结构
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagCLIENT_LIST
{
	int iDelFlag;//节点删除标志,客户端开连接或者网络异常时会被置位，为1时表明需要删除当前客户节点
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

	pthread_mutex_t	mutexClientListMutex;	 //客户链表互斥锁

}CLIENT_LIST_HEAD_OBJ, *CLIENT_LIST_HEAD_HANDLE;

HB_S32	add_client_in_tail(CLIENT_LIST_HEAD_HANDLE pClientListHead, CLIENT_LIST_HANDLE pNewNode);
HB_S32 del_one_client(CLIENT_LIST_HEAD_HANDLE pClientListHead, CLIENT_LIST_HANDLE pDelNode);
HB_VOID destory_client_list(CLIENT_LIST_HEAD_HANDLE pClientListHead);


#endif /* CLIENT_LIST_H_ */
