#BOX_TYPE?=pc_x86
#BOX_TYPE?=pc_x86_64
BOX_TYPE?=hisi_v100

ifeq ($(BOX_TYPE), pc_x86)
CC=gcc
STRIP=strip
CFLAGS = -Wall
INC_PATH = -I./inc
LIBS :=  -L./lib/X86 \
			-levent -levent_pthreads -lsqlite3 -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = BoxRtsp_X86
DEST_BIN = /mnt/hgfs/nfs_dir/share_dir/hb/BoxRtsp/bin
endif

ifeq ($(BOX_TYPE), pc_x86_64)
CC=gcc
STRIP=strip
CFLAGS = -Wall -g
INC_PATH = -I./inc
LIBS :=  -L./lib/X86_64 \
			-levent -levent_pthreads -lsqlite3 -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = BoxRtsp_X86_64
DEST_BIN = /mnt/hgfs/share_dir/nfs_dir/BoxRtsp/bin
endif

ifeq ($(BOX_TYPE), hisi_v100)
CC=arm-hisiv100nptl-linux-gcc
STRIP=arm-hisiv100nptl-linux-strip
CFLAGS = -Wall -O2
INC_PATH = -I./inc
LIBS :=  -L./lib/hisiv100 \
			-levent -levent_pthreads -lsqlite3 -lavformat \
			-lavcodec -lavfilter -lswscale -lavutil -lswresample -lm -ldl -lpthread -lrt

APPBIN = BoxRtsp_hisi100
DEST_BIN = /mnt/hgfs/nfs_dir/share_dir/hb/BoxRtsp/bin
endif



SRCS = $(wildcard ./src/*.c)
OBJS = $(patsubst %.c,%.o,$(notdir ${SRCS}))

all:
	$(CC) $(CFLAGS) $(SRCS) $(INC_PATH) $(LIBS) -o $(APPBIN)
	$(STRIP) $(APPBIN)
	cp $(APPBIN) $(DEST_BIN)/$(APPBIN)
	mv $(APPBIN) ./bin/
clean:
	rm -rf $(OBJS) ./bin/$(APPBIN)
