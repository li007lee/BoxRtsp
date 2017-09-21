/*
 * main.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "my_include.h"

#include "libavformat/avformat.h"
#include "listen_and_deal_cmd.h"

#include "dev_list.h"

int main()
{
	signal(SIGPIPE, SIG_IGN);

	av_register_all();
	avformat_network_init();

	//初始化设备链表头
	init_dev_list();

	start_listening();

	printf("############  main func exit !!!");
	return 0;
}
