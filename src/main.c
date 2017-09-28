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
	signal(SIGPIPE, SIG_IGN);
//	signal(SIGSEGV, SIG_IGN)

	av_register_all();
	avformat_network_init();

//	初始化设备链表头
	init_dev_list();

	libevent_server_main_listen();

	return 0;
}
