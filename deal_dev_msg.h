/*
 * deal_dev_msg.h
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#ifndef DEAL_DEV_MSG_H_
#define DEAL_DEV_MSG_H_


#include "my_include.h"

#include "event.h"
#include "event2/listener.h"

#include "list.h"
#include "dev_list.h"
#include "listen_and_deal_cmd.h"

//测试设备连通性，并获取sdp信息
HB_S32 test_dev_connection(CLIENT_INFO_HANDLE p_CommunicateTags);

//从设备读取视频流线程
HB_VOID *read_video_data_from_dev_task(HB_VOID *param);
void read_dev_sdp_cb(struct bufferevent *connect_dev_bev, void *arg);

#endif /* DEAL_DEV_MSG_H_ */
