/*
 * dev_list.h
 *
 *  Created on: 2017年9月22日
 *      Author: lijian
 */

#ifndef SRC_DEV_LIST_H_
#define SRC_DEV_LIST_H_

#include "my_include.h"
#include "simclist.h"

#define MAX_DEV_ID_LEN	256

//已经接入的用户的链表
typedef struct _tagCLIENT_LIST
{
	HB_S32 iDelFlag;//节点删除标志,客户端开连接或者网络异常时会被置位，为1时表明需要删除当前客户节点
	HB_S32 iMissFrameFlag;	//丢帧表示1表示丢过帧，0表示为丢过
	HB_S64 pts;
	struct bufferevent *pSendVideoToServerEvent;//主动连接服务器事件（视频流推送事件句柄）
}CLIENT_LIST_OBJ, *CLIENT_LIST_HANDLE;

//等待接入的用户链表
typedef struct WAIT_CLIENT_LIST
{
	struct bufferevent *pWaitClientBev;
}WAIT_CLIENT_LIST_OBJ, *WAIT_CLIENT_LIST_HANDLE;


//盒子与设备的连接状态
typedef enum _DEV_CONNECT_STATUS
{
	CONNECTED=1,	//已连接
	CONNECTING,		//连接中
	DISCONNECT,		//连接失败
	NONE			//未连接s
}DEV_CONNECT_STATUS;

typedef struct _tagDEV_LIST_INFO
{
	HB_CHAR pDevId[MAX_DEV_ID_LEN];	//设备ID
	HB_S32 iDevChnl;		//设备通道号
	HB_S32 iDevStreamType;	//设备主子码流
}DEV_INFO_OBJ, *DEV_INFO_HANDLE;

//具体记载设备信息的结构体
typedef struct _tagDEV_LIST
{
	HB_CHAR pDevId[MAX_DEV_ID_LEN];	//设备ID
	HB_S32 iDevChnl;		//设备通道号
	HB_S32 iDevStreamType;	//设备主子码流
	HB_CHAR arrDevIp[16];	//设备ip
	HB_S32	iDevRtspPort;		//设备rtsp端口
	HB_CHAR arrDevRtspMainUrl[512]; //设备rtsp地址
	HB_CHAR arrDevRtspSubUrl[512]; //设备rtsp地址
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

	HB_S32 iStartThreadFlag;//视频流收发线程启动标志位，1 已启动，0未启动
	pthread_t threadReadVideoId; //视频读取线程id
	pthread_t threadSendVideoId; //视频发送线程id
	DEV_CONNECT_STATUS enumDevConnectStatus; //设备连接状态
	list_t listRtspClient; //用户链表， CLIENT_LIST_OBJ类型结构体
	list_t listWaitClient; //等待连接的用户链表， WAIT_CLIENT_LIST_OBJ类型结构体
}DEV_LIST_OBJ, *DEV_LIST_HANDLE;

list_t listDevList;
pthread_rwlock_t rwlockMyLock;	 //全局读写锁

#endif /* SRC_DEV_LIST_H_ */
