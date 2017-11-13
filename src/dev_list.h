/*
 * dev_list.h
 *
 *  Created on: 2017年9月22日
 *      Author: lijian
 */

#ifndef SRC_DEV_LIST_H_
#define SRC_DEV_LIST_H_

#include "my_include.h"

#include "client_list.h"

#define MAX_DEV_ID_LEN	256

//盒子与设备的连接状态
typedef enum _DEV_CONNECT_STATUS
{
	CONNECTED=1,	//已连接
	CONNECTING,		//连接中
	DISCONNECT,		//连接失败
	NONE			//未连接s
}DEV_CONNECT_STATUS;

//具体记载设备信息的结构体
typedef struct _tagDEV_LIST
{
	HB_CHAR pDevId[256];	//设备ID
	HB_S32 iDevChnl;		//设备通道号
	HB_S32 iDevStreamType;	//设备主子码流
	HB_CHAR arrDevIp[16];	//设备ip
	HB_S32	iDevRtspPort;		//设备rtsp端口
	HB_CHAR arrDevRtspMainUrl[512]; //设备rtsp地址
	HB_CHAR arrDevRtspSubUrl[512]; //设备rtsp地址
//	HB_CHAR arrDevRtspUrl[512]; //设备rtsp地址
	HB_CHAR arrUserName[64];	//设备用户名
	HB_CHAR arrUserPasswd[64];	//设备密码
	HB_CHAR arrBasicAuthenticate[128]; //基本认证

	/*************视频sdp信息*************/
	HB_S32 iVideoFrameRate; //视频帧率
	HB_S32 iPtsRateInterval; //pts帧间隔

	HB_CHAR m_video[64];
	HB_CHAR a_rtpmap_video[128];
	HB_CHAR a_fmtp_video[512];
	HB_CHAR m_audio[64];
	HB_CHAR a_rtpmap_audio[128];
	/*************视频sdp信息*************/

	DEV_CONNECT_STATUS enumDevConnectStatus; //设备连接状态
	CLIENT_LIST_HEAD_OBJ stRtspClientHead;//用户链表的链表头
	WAIT_CLIENT_LIST_HEAD_OBJ stWaitClientHead; //等待连接的用户链表

	struct _tagDEV_LIST *pPrev;
	struct _tagDEV_LIST *pNext;
}DEV_LIST_OBJ, *DEV_LIST_HANDLE;

//设备链表结构体头，程序启动时初始化
typedef struct _tagDEV_LIST_HEAD
{
	HB_S32	iDevNum;//当前设备链表中设备的个数

	DEV_LIST_HANDLE pDevListFirst;	//指向设备链表头结点
	DEV_LIST_HANDLE pDevListLast;	//指向设备链表尾结点

	pthread_mutex_t	mutexDevListMutex;	 //设备链表互斥所
}DEV_LIST_HEAD_OBJ, *DEV_LIST_HEAD_HANDLE;

DEV_LIST_HEAD_OBJ stDevListHead;

HB_VOID init_dev_list();
HB_VOID add_to_dev_list(DEV_LIST_HANDLE pNewNode);
HB_S32 del_one_from_dev_list(DEV_LIST_HANDLE pDelNode);
DEV_LIST_HANDLE find_in_dev_list(HB_CHAR *pDevID, HB_S32 iDevChnl, HB_S32 iDevStreamType);


#endif /* SRC_DEV_LIST_H_ */
