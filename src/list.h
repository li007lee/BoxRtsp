#ifndef _LIST_H
#define _LIST_H

typedef HB_VOID* (*NewListNode)();
typedef HB_VOID (*DelListNode)(HB_VOID* info);

/////////////////////////////////////////////////////////////////////////////////
// 链表节点的数据结构
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagLIST_NODE
{
	HB_VOID *p_value;				//存放数据的指针
	//HB_S32	len;					        //数据的长度
	struct	_tagLIST_NODE *p_next;	//下一个节点的地址
}LIST_NODE_OBJ, *LIST_NODE_HANDLE;

/////////////////////////////////////////////////////////////////////////////////
// 链表的数据结构
/////////////////////////////////////////////////////////////////////////////////
typedef struct _tagLIST
{
	HB_S32               record_flag;
	HB_S32               stop_send_flag;
	HB_S32				cnt;				           // 链表中节点的个数
	LIST_NODE_HANDLE	p_head;		   // 链表中头节点的指针
	LIST_NODE_HANDLE	p_end;			   // 链表中尾节点的指针

	NewListNode			new_node;
	DelListNode			del_node;
}LIST_OBJ, *LIST_HANDLE;

LIST_HANDLE   list_create(HB_VOID);						    // 创建链表
HB_VOID 	  list_reset(LIST_HANDLE hLst);				// 重置链表
HB_VOID 	  list_destroy(LIST_HANDLE hLst);			// 销毁链表
HB_S32	  	  list_remove_head_node(LIST_HANDLE hLst);   	// 从链表头取出节点
HB_VOID*	  list_add_node_to_end(LIST_HANDLE hLst);		    // 向链表尾添加节点


#endif /* _SF_TVS_LIST_H */
