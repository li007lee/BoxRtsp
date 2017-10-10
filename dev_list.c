/*
 * dev_list.c
 *
 *  Created on: 2017年5月22日
 *      Author: root
 */

//#include <stdio.h>
//#include <string.h>
//#include <malloc.h>

#include "dev_list.h"


///////////////////////////////////////////////
//	Function: 初始化设备链表
//
//	@param: 无
//
//	Retrun: 无
///////////////////////////////////////////////
HB_VOID init_dev_list()
{
	//DEV_LIST_HANDLE p_DevListHead = malloc(sizeof(DEV_LIST_OBJ));

	memset(&st_DevListHead, 0, sizeof(st_DevListHead));
	st_DevListHead.cnt = 0;
	st_DevListHead.p_DevListHead = NULL;
	st_DevListHead.p_DevListEnd = NULL;

//	memset(st_DevListHead.arr_DevIp, 0, sizeof(st_DevListHead.arr_DevIp));
//	st_DevListHead.i_DevRtspPort = 0;
//	st_DevListHead.i_RtspServerNum = 0;
//	st_DevListHead.i_DevChnl = 0;		//设备通道号
//	st_DevListHead.i_DevStreamType = 0;	//设备主子码流
//
//	st_DevListHead.p_Rtsp_list_head = NULL;
//
//	st_DevListHead.p_Prev = &st_DevListHead;
//	st_DevListHead.p_Next = NULL;
//	st_DevListHead.p_End = &st_DevListHead;
//
//	st_DevListHead.p_EventBufBev = NULL;

	pthread_mutex_init(&st_DevListHead.mutex_ListMutex, NULL);

	return;
}


///////////////////////////////////////////////
//	Function: 创建一个新的设备节点并初始化
//
//	@param p_DevId: [IN]设备ID
//	@param i_DevChnl: [IN]设备通道号
//	@param i_DevStreamType: [IN]主子码流
//
//	Retrun: 返回新节点地址
///////////////////////////////////////////////
DEV_LIST_HANDLE create_new_dev_node(HB_CHAR *p_DevId, HB_S32 i_DevChnl, HB_S32 i_DevStreamType)
{
	DEV_LIST_HANDLE p_NewNode = malloc(sizeof(DEV_LIST_OBJ));
	memset(p_NewNode, 0, sizeof(DEV_LIST_OBJ));

	strncpy(p_NewNode->p_DevId, p_DevId, sizeof(p_NewNode->p_DevId));
	p_NewNode->i_DevChnl = i_DevChnl;
	p_NewNode->i_DevStreamType = i_DevStreamType;

	p_NewNode->st_ClientListHead.i_ClientNum = 0;
//	p_NewNode->st_ClientListHead.p_ClientListHead = NULL;
//	p_NewNode->st_ClientListHead.p_ClientListEnd = NULL;
//	pthread_mutex_init(&(p_NewNode->st_ClientListHead.mutex_ListMutex), NULL);
	return p_NewNode;
}


///////////////////////////////////////////////
//	Function: 创建并通过尾插法向链表插入一个节点
//
//	@param: 无
//
//	Retrun: 返回新节点地址
///////////////////////////////////////////////
DEV_LIST_HANDLE add_node_to_dev_list(DEV_LIST_HANDLE p_NewNode)
{
	//DEV_LIST_HANDLE p_Node = malloc(sizeof(DEV_LIST_OBJ));
	//memset(p_Node, 0, sizeof(DEV_LIST_OBJ));
	//pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
	if (st_DevListHead.cnt == 0)
	{
		st_DevListHead.p_DevListHead = p_NewNode;
		st_DevListHead.p_DevListEnd = p_NewNode;

		p_NewNode->p_Next = NULL;
		p_NewNode->p_Prev = NULL;
	}
	else
	{
		p_NewNode->p_Next = NULL;
		p_NewNode->p_Prev = st_DevListHead.p_DevListEnd;
		st_DevListHead.p_DevListEnd->p_Next = p_NewNode;
		st_DevListHead.p_DevListEnd = p_NewNode;
	}

	st_DevListHead.cnt += 1;
	//pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
	//pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
	//pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));

	return p_NewNode;
}


///////////////////////////////////////////////
//	Function: 删除设备节点
//
//	@param p_DelNode: 需要删除的节点
//
//	Retrun: 失败返回-1, 成功返回0
///////////////////////////////////////////////
HB_S32 remove_one_from_dev_list(DEV_LIST_HANDLE p_DelNode)
{
	if ((p_DelNode == NULL) || (st_DevListHead.cnt == 0))
	{
		//printf("输入的节点有误！\n");
		return -1;
	}
	//pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));

	//删除此设备下的所有客户节点
	destory_client_list(&(p_DelNode->st_ClientListHead));

	//如果只有一个节点
	if (st_DevListHead.cnt == 1)
	{
		st_DevListHead.p_DevListHead = NULL;
		st_DevListHead.p_DevListEnd = NULL;
	}
	else if (p_DelNode->p_Next == NULL) //删除的是链表中的最后一个节点
	{
		p_DelNode->p_Prev->p_Next = NULL;
		st_DevListHead.p_DevListEnd = p_DelNode->p_Prev;
	}
	else if (p_DelNode->p_Prev == NULL) //删除的是第一个节点
	{
		p_DelNode->p_Next->p_Prev = NULL;
		st_DevListHead.p_DevListHead = p_DelNode->p_Next;
	}
	else //删除的是中间节点
	{
		p_DelNode->p_Prev->p_Next = p_DelNode->p_Next;
		p_DelNode->p_Next->p_Prev = p_DelNode->p_Prev;
	}

	st_DevListHead.cnt -= 1;
	free(p_DelNode);
	//pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
	//pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
	return 0;
}


///////////////////////////////////////////////
//	Function: 查找是不是已经有服务器连接了设备
//
//	@param p_DevID: [IN]需要查找的设备id
//	@param i_DevChnl: [IN]设备通道号
//	@param i_DevStreamType: [IN]主子码流
//
//	Retrun: 如果找到返回设备节点的地址，未找到返回NULL
///////////////////////////////////////////////
DEV_LIST_HANDLE find_in_dev_list(HB_CHAR *p_DevID, HB_S32 i_DevChnl, HB_S32 i_DevStreamType)
{
	DEV_LIST_HANDLE p_Object = st_DevListHead.p_DevListHead;
	//pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
	while(p_Object != NULL)
	{
		//设备id，通道号，主子码流一致才算找到设备
		if ((strcmp(p_Object->p_DevId, p_DevID) == 0) \
			&& (p_Object->i_DevChnl == i_DevChnl) \
			&& (p_Object->i_DevStreamType == i_DevStreamType))
		{
			//pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
			return p_Object;
		}
		p_Object = p_Object->p_Next;
	}
	//pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
	//未找到设备
	printf("have not find this device.\n");
	return NULL;
}

///////////////////////////////////////////////
//	Function: 通过socket_id查找设备
//
//	@param p_DevID: [IN]需要查找的设备id
//	@param i_DevChnl: [IN]设备通道号
//	@param i_DevStreamType: [IN]主子码流
//
//	Retrun: 如果找到返回设备节点的地址，未找到返回NULL
///////////////////////////////////////////////
DEV_LIST_HANDLE find_in_dev_list_sockid(HB_S32 i_SockFd)
{
	DEV_LIST_HANDLE p_Object = st_DevListHead.p_DevListHead;
	pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));
	while(p_Object != NULL)
	{
		//设备id，通道号，主子码流一致才算找到设备
		if (p_Object->i_DevChnl == i_SockFd)
		{
			pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
			return p_Object;
		}
		p_Object = p_Object->p_Next;
	}
	pthread_mutex_unlock(&(st_DevListHead.mutex_ListMutex));
	//未找到设备
	return NULL;
}


///////////////////////////////////////////////
//	Function: 清空设备链表
//
//	@param: 无
//
//	Retrun: 无
///////////////////////////////////////////////
HB_VOID destory_dev_list()
{
	while(st_DevListHead.cnt > 0)
	{
		remove_one_from_dev_list(st_DevListHead.p_DevListHead);
	}

	return;
}
