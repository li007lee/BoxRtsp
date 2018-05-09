/*
 * main.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "my_include.h"

#include "libavformat/avformat.h"
#include "libevent_server.h"
#include "dev_opt.h"
#include "dev_list.h"

//char cServerIp[16] = {0};
HB_CHAR	 glBoxSn[32] = {0}; //盒子序列号


int main(int argc, char **argv)
{
	int ret = 0;

//	signal(SIGPIPE, SIG_IGN);
	sigset_t signal_mask;
	sigemptyset (&signal_mask);
	sigaddset (&signal_mask, SIGPIPE);
	ret = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
	if (ret != 0)
	{
		TRACE_ERR("###### block sigpipe error!\n");
		return HB_FAILURE;
	}

	get_sys_sn(glBoxSn, sizeof(glBoxSn));
//	strncpy(cServerIp, argv[1], 16);

	av_register_all();
	avformat_network_init();

	list_init(&listDevList);
	pthread_rwlock_init(&rwlockMyLock, NULL);

	printf("----------------------------------------------------------------\n");
	printf("|                        BoxRtsp Started                       |\n");
	printf("|              CompileTime : %s %s              |\n", __DATE__, __TIME__);
	printf("----------------------------------------------------------------\n");
	libevent_server_main_listen();

	return 0;
}
