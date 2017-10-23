/*
 * main.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "my_include.h"

#include "libavformat/avformat.h"
#include "libevent_server.h"
#include "dev_list.h"

int main()
{
	int ret = 0;
	signal(SIGPIPE, SIG_IGN);
	sigset_t signal_mask;
	sigemptyset (&signal_mask);
	sigaddset (&signal_mask, SIGPIPE);
	ret = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
	if (ret != 0)
	{
		TRACE_ERR("###### block sigpipe error!\n");
		return HB_FAILURE;
	}

	av_register_all();
	avformat_network_init();

//	初始化设备链表头
	init_dev_list();

	printf("==============================================================\n");
	printf("=========================BoxRtsp Started======================\n");
	printf("==============================================================\n");
	libevent_server_main_listen();

	return 0;
}
