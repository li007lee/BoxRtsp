/*
 * common.h
 *
 *  Created on: 2018年5月10日
 *      Author: root
 */

#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

#include "my_include.h"

typedef struct _gl_param
{
	HB_S32 iMacSnLen; //盒子序列号长度加1 ，因为序列号后面有个短横线。例子：261777649232466-DS-2CD1201D-I320170526AACH766877798
#ifdef HAND_SERVER_IP
	HB_CHAR cServerIp[16] = {0};
#endif
}GLOBLE_PARAM;



#endif /* SRC_COMMON_H_ */
