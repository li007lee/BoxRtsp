/*
 * dev_opt.h
 *
 *  Created on: 2017年9月25日
 *      Author: lijian
 */

#ifndef SRC_DEV_OPT_H_
#define SRC_DEV_OPT_H_

#include "my_include.h"

#include "libevent_server.h"

HB_VOID test_dev_connection(DEV_LIST_HANDLE pDevNode);
HB_VOID *read_video_data_from_dev_task(HB_VOID *arg);
//获取盒子到序列号
HB_S32 get_sys_sn(HB_CHAR *sn, HB_S32 sn_size);

#endif /* SRC_DEV_OPT_H_ */
