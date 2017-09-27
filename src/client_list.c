/*
 * rtsp_server_list.c
 *
 *  Created on: 2017年5月23日
 *      Author: root
 */

#include "client_list.h"

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
 *	@param pClientListHead: [IN]需要插入链表的链表头节点
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
		del_one_client(p_ClientListHead, p_ClientListHead->pClientListFirst);
	}

	return;
}
