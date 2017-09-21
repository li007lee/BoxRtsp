/*
 * dev_list.h
 *
 *  Created on: 2017年5月22日
 *      Author: root
 */

#ifndef DEV_LIST_H_
#define DEV_LIST_H_

#include "my_include.h"

#include "event.h"
#include "event2/listener.h"

#include "client_list.h" //rtsp服务端的链表头文件

#define MAX_DEV_ID_LEN	256

/////////////////////////////////////////////////////////////////////////////////
// 设备链表数据结构
/////////////////////////////////////////////////////////////////////////////////


typedef struct DEV_LIST
{
	HB_CHAR p_DevId[256];		//设备ID
	HB_S32 i_DevChnl;		//设备通道号
	HB_S32 i_DevStreamType;	//设备主子码流
	HB_CHAR arr_DevIp[16];	//设备ip
	HB_S32	i_DevRtspPort;		//设备rtsp端口
	HB_CHAR arr_DevRtspUrl[512];

	/*************视频sdp信息*************/
	HB_CHAR m_video[64];
	HB_CHAR a_rtpmap_video[128];
	HB_CHAR a_fmtp_video[512];
	HB_CHAR m_audio[64];
	HB_CHAR a_rtpmap_audio[128];
	/*************视频sdp信息*************/

	//struct bufferevent *p_EventDev;
	HB_S32 exit_flag;	//设备退出标志

	struct DEV_LIST *p_Prev;
	struct DEV_LIST *p_Next;

	CLIENT_LIST_HEAD_OBJ st_ClientListHead;		//client链表中头节点的指针

}DEV_LIST_OBJ, *DEV_LIST_HANDLE;


typedef struct _tagDEV_LIST_HEAD
{
	HB_S32	cnt;//打开的设备的个数

	DEV_LIST_HANDLE p_DevListHead;
	DEV_LIST_HANDLE p_DevListEnd;

//	pthread_mutex_t	mutex_ListMutex;	 //链表互斥锁
	pthread_mutex_t	mutex_DevListMutex;	 //设备链表互斥所

}DEV_LIST_HEAD_OBJ, *DEV_LIST_HEAD_HANDLE;

DEV_LIST_HEAD_OBJ st_DevListHead;

///////////////////////////////////////////////
//	Function: 初始化设备链表
//
//	@param: 无
//
//	Retrun: 无
///////////////////////////////////////////////
HB_VOID init_dev_list();


///////////////////////////////////////////////
//	Function: 创建一个新的设备节点并初始化
//
//	@param p_DevId: [IN]设备ID
//	@param i_DevChnl: [IN]设备通道号
//	@param i_DevStreamType: [IN]主子码流
//
//	Retrun: 返回新节点地址
///////////////////////////////////////////////
DEV_LIST_HANDLE create_new_dev_node(HB_CHAR *p_DevId, HB_S32 i_DevChnl, HB_S32 i_DevStreamType);


///////////////////////////////////////////////
//	Function: 尾插法向链表插入节点
//
//	@param: 无
//
//	Retrun: 返回新节点地址
///////////////////////////////////////////////
DEV_LIST_HANDLE add_node_to_dev_list(DEV_LIST_HANDLE p_NewNode);

///////////////////////////////////////////////
//	Function: 删除设备节点
//
//	@param p_DelNode: 需要删除的节点
//
//	Retrun: 失败返回-1, 成功返回0
///////////////////////////////////////////////
HB_S32 remove_one_from_dev_list(DEV_LIST_HANDLE p_DelNode);

///////////////////////////////////////////////
//	Function: 查找是不是已经有服务器连接了设备
//
//	@param p_DevID: [IN]需要查找的设备id
//	@param i_DevChnl: [IN]设备通道号
//	@param i_DevStreamType: [IN]主子码流
//
//	Retrun: 如果找到返回设备节点的地址，未找到返回NULL
DEV_LIST_HANDLE find_in_dev_list(HB_CHAR *p_DevID, HB_S32 i_DevChnl, HB_S32 i_DevStreamType);


///////////////////////////////////////////////
//	Function: 清空设备链表
//
//	@param: 无
//
//	Retrun: 无
///////////////////////////////////////////////
HB_VOID destory_dev_list();

#endif /* DEV_LIST_H_ */
