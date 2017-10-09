/*
 * rtsp_server_list.c
 *
 *  Created on: 2017年5月23日
 *      Author: root
 */

#include "client_list.h"
#include "event2/bufferevent.h"

/*
 *	Function: 向用户链表中插入节点
 *
 *	@param pClientListHead: [IN]需要插入链表的链表头节点
 *  @parmm pNewNode: [IN]需要插入的节点
 *
 *	Retrun: 成功返回HB_SUCCESS，失败返回HB_FAILURE
 */
HB_S32 add_client_in_tail(CLIENT_LIST_HEAD_HANDLE pClientListHead, CLIENT_LIST_HANDLE pNewNode)
{
	if (NULL == pClientListHead || NULL == pNewNode)
	{
		TRACE_ERR("client head is NULL or New Node is NULL!\n");
		return HB_FAILURE;
	}

	if (0 == pClientListHead->iClientNum)
	{
		pNewNode->pNext = NULL;
		pNewNode->pPrev = NULL;
		pClientListHead->pClientListFirst = pNewNode;
		pClientListHead->pClientListLast = pNewNode;
	}
	else
	{
		pNewNode->pNext = NULL;
		pNewNode->pPrev = pClientListHead->pClientListLast;
		pClientListHead->pClientListLast->pNext = pNewNode;
		pClientListHead->pClientListLast = pNewNode;
	}

	pClientListHead->iClientNum += 1;
	TRACE_YELLOW("total client = %d\n", pClientListHead->iClientNum);

	return HB_SUCCESS;
}


/*
 *	Function: 从用户链表删除节点
 *
 *	@param pClientListHead: [IN]需要删除链表的链表头节点
 *  @parmm pDelNode: [IN]需要删除的节点
 *
 *	Retrun: 成功返回HB_SUCCESS，失败返回HB_FAILURE
 */
HB_S32 del_one_client(CLIENT_LIST_HEAD_HANDLE pClientListHead, CLIENT_LIST_HANDLE pDelNode)
{
	if ((pDelNode == NULL) || (pClientListHead->iClientNum == 0))
	{
		TRACE_ERR("需要删除的节点为空或链表为空！\n");
		return HB_FAILURE;
	}

	//如果只有一个节点
	if (pClientListHead->iClientNum == 1)
	{
		pClientListHead->pClientListFirst = NULL;
		pClientListHead->pClientListLast = NULL;
	}
	else if (pDelNode->pNext == NULL) //删除的是链表中的最后一个节点
	{
		pDelNode->pPrev->pNext = NULL;
		pClientListHead->pClientListLast = pDelNode->pPrev;
	}
	else if (pDelNode->pPrev == NULL) //删除的是第一个节点
	{
		pDelNode->pNext->pPrev = NULL;
		pClientListHead->pClientListFirst = pDelNode->pNext;
	}
	else //删除的是中间节点
	{
		pDelNode->pPrev->pNext = pDelNode->pNext;
		pDelNode->pNext->pPrev = pDelNode->pPrev;
	}

	pClientListHead->iClientNum -= 1;
	free(pDelNode);
	pDelNode = NULL;
	TRACE_YELLOW("total client = %d\n", pClientListHead->iClientNum);

	return 0;
}


/*
 *	Function: 销毁用户链表
 *
 *	@param pClientListHead: [IN]需要销毁链表的链表头
 *
 *	Retrun: 无
 */
HB_VOID destory_client_list(CLIENT_LIST_HEAD_HANDLE p_ClientListHead)
{
	while(p_ClientListHead->pClientListFirst != NULL)
	{
		if (p_ClientListHead->pClientListFirst->pSendVideoToServerEvent != NULL)
		{
			bufferevent_free(p_ClientListHead->pClientListFirst->pSendVideoToServerEvent);
			p_ClientListHead->pClientListFirst->pSendVideoToServerEvent = NULL;
		}
		del_one_client(p_ClientListHead, p_ClientListHead->pClientListFirst);
	}

	return;
}





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//当多人同时连接同一设备时，只有一个人去连接实际设备，其他人等待连接状态改变，此处为等待者链表
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	Function: 向等待用户链表中插入节点
 *
 *	@param pWaitClientListHead: [IN]需要插入链表的链表头节点
 *  @parmm pNewNode: [IN]需要插入的节点
 *
 *	Retrun: 成功返回HB_SUCCESS，失败返回HB_FAILURE
 */
HB_S32 add_wait_client_in_tail(WAIT_CLIENT_LIST_HEAD_HANDLE pWaitClientListHead, WAIT_CLIENT_LIST_HANDLE pNewNode)
{
	if (NULL == pWaitClientListHead || NULL == pNewNode)
	{
		TRACE_ERR("client head is NULL or New Node is NULL!\n");
		return HB_FAILURE;
	}

	if (0 == pWaitClientListHead->iWaitClientNum)
	{
		pNewNode->pNext = NULL;
		pNewNode->pPrev = NULL;
		pWaitClientListHead->pWaitClientListFirst = pNewNode;
		pWaitClientListHead->pWaitClientListLast = pNewNode;
	}
	else
	{
		pNewNode->pNext = NULL;
		pNewNode->pPrev = pWaitClientListHead->pWaitClientListLast;
		pWaitClientListHead->pWaitClientListLast->pNext = pNewNode;
		pWaitClientListHead->pWaitClientListLast = pNewNode;
	}

	pWaitClientListHead->iWaitClientNum += 1;
	TRACE_YELLOW("total client = %d\n", pWaitClientListHead->iWaitClientNum);

	return HB_SUCCESS;
}


/*
 *	Function: 从等待用户链表删除节点
 *
 *	@param pWaitClientListHead: [IN]需要删除链表的链表头节点
 *  @parmm pDelNode: [IN]需要删除的节点
 *
 *	Retrun: 成功返回HB_SUCCESS，失败返回HB_FAILURE
 */
HB_S32 del_one_wait_client(WAIT_CLIENT_LIST_HEAD_HANDLE pWaitClientListHead, WAIT_CLIENT_LIST_HANDLE pDelNode)
{
	if ((pDelNode == NULL) || (pWaitClientListHead->iWaitClientNum == 0))
	{
		TRACE_ERR("需要删除的节点为空或链表为空！\n");
		return HB_FAILURE;
	}

	//如果只有一个节点
	if (pWaitClientListHead->iWaitClientNum == 1)
	{
		pWaitClientListHead->pWaitClientListFirst = NULL;
		pWaitClientListHead->pWaitClientListLast = NULL;
	}
	else if (pDelNode->pNext == NULL) //删除的是链表中的最后一个节点
	{
		pDelNode->pPrev->pNext = NULL;
		pWaitClientListHead->pWaitClientListLast = pDelNode->pPrev;
	}
	else if (pDelNode->pPrev == NULL) //删除的是第一个节点
	{
		pDelNode->pNext->pPrev = NULL;
		pWaitClientListHead->pWaitClientListFirst = pDelNode->pNext;
	}
	else //删除的是中间节点
	{
		pDelNode->pPrev->pNext = pDelNode->pNext;
		pDelNode->pNext->pPrev = pDelNode->pPrev;
	}

	pWaitClientListHead->iWaitClientNum -= 1;
	free(pDelNode);
	pDelNode = NULL;
	TRACE_YELLOW("total client = %d\n", pWaitClientListHead->iWaitClientNum);

	return 0;
}


HB_VOID destory_wait_client(WAIT_CLIENT_LIST_HEAD_HANDLE pWaitClientListHead)
{
	while(pWaitClientListHead->iWaitClientNum > 1)
	{
		del_one_wait_client(pWaitClientListHead, pWaitClientListHead->pWaitClientListFirst);
	}
}
