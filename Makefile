
BOX_TYPE?=pc_x86
#BOX_TYPE?=hisi_v100

ifeq ($(BOX_TYPE), pc_x86)
CC=gcc
STRIP=strip
CFLAGS = -Wall -O2
INC_PATH = -I./inc/X86_64 -I./inc/X86_64/libevent -I./inc/X86_64/sqlite
LIBS :=  -L./lib/X86_64 \
			-levent -levent_pthreads -lsqlite3 -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = BoxRtsp_X86_64
DEST_BIN = /mnt/hgfs/share_dir/nfs_dir
endif

ifeq ($(BOX_TYPE), hisi_v100)
CC=arm-hisiv100nptl-linux-gcc
STRIP=arm-hisiv100nptl-linux-strip
CFLAGS = -Wall -O2
INC_PATH = -I./inc/hisiv100
LIBS :=  -L./lib/hisiv100 \
			-levent -levent_pthreads -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = BoxRtsp_hisi100_test
DEST_BIN = /mnt/hgfs/share_dir/nfs_dir
endif


#APPBIN = box_main_bak_200
#CC= /root/work/software/arm-hisiv200-linux/arm-hisiv200-linux/bin/arm-hisiv200-linux-gnueabi-gcc
#CC = /root/work/software/arm-hisiv100nptl-linux/arm-hisiv100-linux/bin/arm-hisiv100-linux-uclibcgnueabi-gcc
#INC_PATH = -I./inc -I./inc/xml
#LIBS =  -L./lib/hisi100 -lmd5gen -lremote_debug -lxml   -levent -lupgrade_100  -lpthread -lrt
#LIB_PATH :=  -L./lib/hisi200  -Llib/hisi200/elevator -Llib/hisi200/encrypt
#LIBS =  -L./lib/hisi200 -lmd5gen -lremote_debug -lxml   -levent  -lencrypt -lzf_xvr_server_api_v1 -lsqlite3 -lpthread -lrt
#CFLAGS = -Wall -g -Wl,-rpath /mnt/mtd/test/lib




SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

all:
	$(CC) $(SRCS) $(CFLAGS) $(INC_PATH) $(LIBS) -o $(APPBIN)
	$(STRIP) $(APPBIN)
	cp $(APPBIN) $(DEST_BIN)/$(APPBIN)
clean:
	rm -rf $(OBJS) $(APPBIN)
