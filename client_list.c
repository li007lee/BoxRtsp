/*
 * rtsp_server_list.c
 *
 *  Created on: 2017年5月23日
 *      Author: root
 */

#include "client_list.h"

CLIENT_LIST_HEAD_HANDLE create_client_list()
{
	CLIENT_LIST_HEAD_HANDLE p_ClientListHead = malloc(sizeof(CLIENT_LIST_OBJ));

	p_ClientListHead->i_ClientNum = 0;
	p_ClientListHead->p_ClientListFirst = NULL;
	p_ClientListHead->p_ClientListLast = NULL;

//	pthread_mutex_init(&(p_ClientListHead->mutex_ListMutex), NULL);

	return p_ClientListHead;
}


CLIENT_LIST_HANDLE new_client_node()
{
	CLIENT_LIST_HANDLE p_NewNode = malloc(sizeof(CLIENT_LIST_OBJ));

	memset(p_NewNode, 0, sizeof(CLIENT_LIST_OBJ));

	return p_NewNode;
}


HB_S32 add_client_in_tail(CLIENT_LIST_HEAD_HANDLE p_ClientListHead, CLIENT_LIST_HANDLE p_NewNode)
{
	if (NULL == p_ClientListHead || NULL == p_NewNode)
	{
		printf("client head is NULL or New Node is NULL!\n");
		return -1;
	}

	if (0 == p_ClientListHead->i_ClientNum)
	{
		p_ClientListHead->p_ClientListFirst = p_NewNode;
		p_ClientListHead->p_ClientListLast = p_NewNode;

		p_NewNode->p_Next = NULL;
		p_NewNode->p_Prev = NULL;
	}
	else
	{
		p_NewNode->p_Next = NULL;
		p_NewNode->p_Prev = p_ClientListHead->p_ClientListLast;
		p_ClientListHead->p_ClientListLast->p_Next = p_NewNode;
		p_ClientListHead->p_ClientListLast = p_NewNode;
	}

	p_ClientListHead->i_ClientNum += 1;
	printf("total client = %d\n", p_ClientListHead->i_ClientNum);

	return 0;
}


HB_S32 remove_one_client(CLIENT_LIST_HEAD_HANDLE p_ClientListHead, CLIENT_LIST_HANDLE p_DelNode)
{
	if ((p_DelNode == NULL) || (p_ClientListHead->i_ClientNum == 0))
	{
		printf("输入的节点有误！\n");
		return -1;
	}
	//pthread_mutex_lock(&(p_ClientListHead->mutex_ListMutex));
	//如果只有一个节点
	if (p_ClientListHead->i_ClientNum == 1)
	{
		printf("del only one\n");
		p_ClientListHead->p_ClientListFirst = NULL;
		p_ClientListHead->p_ClientListLast = NULL;
	}
	else if (p_DelNode->p_Next == NULL) //删除的是链表中的最后一个节点
	{
		printf("del last\n");
		p_DelNode->p_Prev->p_Next = NULL;
		p_ClientListHead->p_ClientListLast = p_DelNode->p_Prev;
	}
	else if (p_DelNode->p_Prev == NULL) //删除的是第一个节点
	{
		printf("del first\n");
		p_DelNode->p_Next->p_Prev = NULL;
		p_ClientListHead->p_ClientListFirst = p_DelNode->p_Next;
	}
	else //删除的是中间节点
	{
		printf("del middle\n");
		p_DelNode->p_Prev->p_Next = p_DelNode->p_Next;
		p_DelNode->p_Next->p_Prev = p_DelNode->p_Prev;
	}

	p_ClientListHead->i_ClientNum -= 1;
	free(p_DelNode);
	p_DelNode = NULL;
	printf("total client = %d\n", p_ClientListHead->i_ClientNum);

	//pthread_mutex_unlock(&(p_ClientListHead->mutex_ListMutex));

	return 0;
}


HB_VOID destory_client_list(CLIENT_LIST_HEAD_HANDLE p_ClientListHead)
{
	while(p_ClientListHead->p_ClientListFirst != NULL)
	{
		remove_one_client(p_ClientListHead, p_ClientListHead->p_ClientListFirst);
	}

	return;
}
