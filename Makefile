#BOX_TYPE?=pc_x86
#BOX_TYPE?=pc_x86_64
#BOX_TYPE?=hisi_v100
BOX_TYPE?=hisi_v300

ifeq ($(BOX_TYPE), pc_x86)
CC=gcc
STRIP=strip
CFLAGS = -Wall
INC_PATH = -I./inc -I./inc/common -I./inc/libevent -I./inc/ffmpeg4.0.1
LIBS :=  -L./lib/X86 -L./lib/X86/ffmpeg4.0.1 -L./lib/X86/libevent \
			-levent -levent_pthreads -lsqlite3 -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = BoxRtsp_X86
DEST_BIN = /mnt/hgfs/ShareDir/share/work/BoxRtsp/bin
endif

ifeq ($(BOX_TYPE), pc_x86_64)
CC=gcc
STRIP=strip
CFLAGS = -Wall -g
INC_PATH = -I./inc -I./inc/common -I./inc/libevent -I./inc/ffmpeg4.0.1
LIBS :=  -L./lib/X86_64 -L./lib/X86_64/ffmpeg4.0.1 -L./lib/X86_64/libevent \
			-levent -levent_pthreads -lsqlite3 -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = BoxRtsp_X86_64
DEST_BIN = /mnt/hgfs/ShareDir/share/work/BoxRtsp/bin
endif

ifeq ($(BOX_TYPE), hisi_v100)
CC=arm-hisiv100nptl-linux-gcc
STRIP=arm-hisiv100nptl-linux-strip
CFLAGS = -Wall -O2
INC_PATH = -I./inc -I./inc/common -I./inc/libevent -I./inc/ffmpeg4.0.1
LIBS :=  -L./lib/hisiv100 -L./lib/hisiv100/ffmpeg4.0.1 -L./lib/hisiv100/libevent \
			-levent -levent_pthreads -lsqlite3 -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = box_rtsp_v100
DEST_BIN = /mnt/hgfs/ShareDir/share/work/BoxRtsp/bin
endif

ifeq ($(BOX_TYPE), hisi_v300)
CC=arm-hisiv300-linux-gcc
STRIP=arm-hisiv300-linux-strip
CFLAGS = -Wall -O2
INC_PATH = -I./inc -I./inc/common -I./inc/libevent -I./inc/ffmpeg4.0.1
LIBS := -L./lib/hisiv300 -L./lib/hisiv300/ffmpeg4.0.1 -L./lib/hisiv300/libevent \
			-levent -levent_pthreads -lsqlite3 -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = box_rtsp_v300
DEST_BIN = /mnt/hgfs/ShareDir/share/work/BoxRtsp/bin
endif



SRCS = $(wildcard ./src/*.c ./src/common/*.c)
OBJS = $(patsubst %.c,%.o,$(notdir ${SRCS}))

all:
	$(CC) $(CFLAGS) $(SRCS) $(INC_PATH) $(LIBS) -o $(APPBIN)
	$(STRIP) $(APPBIN)
	cp $(APPBIN) $(DEST_BIN)/$(APPBIN)
	mv $(APPBIN) ./bin/
clean:
	rm -rf $(OBJS) ./bin/$(APPBIN) 
