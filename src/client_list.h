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
	int del_flag;//节点删除标志
	struct bufferevent *p_SendVideoToServerEvent;//主动连接服务器事件

	struct _tagCLIENT_LIST *p_Prev;
	struct _tagCLIENT_LIST *p_Next;

}CLIENT_LIST_OBJ, *CLIENT_LIST_HANDLE;

typedef struct CLIENT_LIST_HEAD
{
	HB_S32 i_ClientNum;	//客户节点的个数
	pthread_t thread_ReadVideo_id; //视频读取线程id
	pthread_t thread_SendVideo_id; //视频发送线程id

	HB_S32 start_thread_flag; //线程启动标志位，1 已启动，0未启动

	VIDEO_DATA_LIST_OBJ video_data_list;//存储视频流的链表

	CLIENT_LIST_HANDLE p_ClientListFirst;//第一个客户节点
	CLIENT_LIST_HANDLE p_ClientListLast;//最后一个客户节点

	pthread_mutex_t	mutex_ClientListMutex;	 //客户链表互斥锁

}CLIENT_LIST_HEAD_OBJ, *CLIENT_LIST_HEAD_HANDLE;


CLIENT_LIST_HEAD_HANDLE create_client_list();
CLIENT_LIST_HANDLE new_client_node();
HB_S32	add_client_in_tail(CLIENT_LIST_HEAD_HANDLE p_ClientListHead, CLIENT_LIST_HANDLE p_NewNode);
HB_S32 remove_one_client(CLIENT_LIST_HEAD_HANDLE p_ClientListHead, CLIENT_LIST_HANDLE p_DelNode);
HB_VOID destory_client_list(CLIENT_LIST_HEAD_HANDLE p_ClientListHead);


#endif /* CLIENT_LIST_H_ */
