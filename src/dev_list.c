/*
 * dev_list.c
 *
 *  Created on: 2017年9月22日
 *      Author: lijian
 */

#include "dev_list.h"

/*
 *	Function: 初始化设备链表头
 *
 *	@param: 无
 *
 *	Retrun: 无
 */
HB_VOID init_dev_list()
{
	memset(&stDevListHead, 0, sizeof(DEV_LIST_HEAD_OBJ));
	stDevListHead.iDevNum = 0;
	stDevListHead.pDevListFirst = NULL;
	stDevListHead.pDevListLast = NULL;

	pthread_mutex_init(&stDevListHead.mutexDevListMutex, NULL);
}


/*
 *	Function: 尾插法向链表插入节点
 *
 *	@param pNewNode: [IN]需要插入链表的新节点
 *
 *	Retrun: 无
 */
HB_VOID add_to_dev_list(DEV_LIST_HANDLE pNewNode)
{
	if (stDevListHead.iDevNum == 0) //如果当前链表为空
	{
		pNewNode->pNext = NULL;
		pNewNode->pPrev = NULL;

		stDevListHead.pDevListFirst = pNewNode;
		stDevListHead.pDevListLast = pNewNode;
	}
	else
	{
		pNewNode->pNext = NULL;
		pNewNode->pPrev = stDevListHead.pDevListLast;
		stDevListHead.pDevListLast->pNext = pNewNode;
		stDevListHead.pDevListLast = pNewNode;
	}

	stDevListHead.iDevNum += 1;

	TRACE_GREEN("Add device!Cur device num [%d]", stDevListHead.iDevNum);
}


/*
 *	Function: 删除设备节点
 *
 *	@param pDelNode: [IN]需要删除的节点
 *
 *	Retrun: 失败返回-1, 成功返回0
 */
HB_S32 del_one_from_dev_list(DEV_LIST_HANDLE pDelNode)
{
	if ((pDelNode == NULL) || (stDevListHead.iDevNum == 0))
	{
		TRACE_ERR("正在删除的节点为空或设备链表为空！");
		return HB_FAILURE;
	}
	//pthread_mutex_lock(&(st_DevListHead.mutex_ListMutex));

	//删除此设备下的所有客户节点
//	destory_client_list(&(pDelNode->st_ClientListHead));

	//如果只有一个节点
	if (stDevListHead.iDevNum == 1)
	{
		stDevListHead.pDevListFirst = NULL;
		stDevListHead.pDevListLast = NULL;
	}
	else if (pDelNode->pNext == NULL) //删除的是链表中的最后一个节点
	{
		pDelNode->pPrev->pNext = NULL;
		stDevListHead.pDevListLast = pDelNode->pPrev;
	}
	else if (pDelNode->pPrev == NULL) //删除的是第一个节点
	{
		pDelNode->pNext->pPrev = NULL;
		stDevListHead.pDevListFirst = pDelNode->pNext;
	}
	else //删除的是中间节点
	{
		pDelNode->pPrev->pNext = pDelNode->pNext;
		pDelNode->pNext->pPrev = pDelNode->pPrev;
	}

	stDevListHead.iDevNum -= 1;

	//在此处销毁用户链表中的所有节点

	free(pDelNode);
	pDelNode = NULL;

	TRACE_YELLOW("Del device! Cur device num [%d]", stDevListHead.iDevNum);
	return HB_SUCCESS;
}


/*
 *	Function: 查找设备是否已经存在
 *
 *	@param pDevID: [IN]设备id
 *	@param iDevChnl: [IN]设备通道号
 *	@param iDevStreamType: [IN]设备主子码流
 *
 *	Retrun: 找到了返回该设备节点的地址，未找到返回NULL
 */
DEV_LIST_HANDLE find_in_dev_list(HB_CHAR *pDevID, HB_S32 iDevChnl, HB_S32 iDevStreamType)
{
	DEV_LIST_HANDLE pObj = stDevListHead.pDevListFirst;
	while(pObj != NULL)
	{
		//设备id，通道号，主子码流一致才算找到设备
		if ((strcmp(pObj->pDevId, pDevID) == 0) \
			&& (pObj->iDevChnl == iDevChnl) \
			&& (pObj->iDevStreamType == iDevStreamType))
		{
			TRACE_YELLOW("The device is already exist!\n");
			return pObj;
		}
		pObj = pObj->pNext;
	}

	//未找到设备
	TRACE_YELLOW("have not find this device.\n");
	return NULL;
}
