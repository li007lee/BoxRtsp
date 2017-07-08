/*
 * main.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "my_include.h"

#include "libavformat/avformat.h"
#include "listen_and_deal_cmd.h"

int main()
{
	signal(SIGPIPE, SIG_IGN);

	av_register_all();
	avformat_network_init();

	start_listening();

	printf("############  main func exit !!!");
	return 0;
}
