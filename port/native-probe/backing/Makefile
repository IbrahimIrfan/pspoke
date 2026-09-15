TARGET = native-backing-proof
OBJS = main.o storage.o memory_layout.o device_registers.o
INCDIR = include ../../native-graphics/alias-proof/include ../generated ../../native-graphics/libntr/include
CFLAGS = -DSDL_MAIN_HANDLED -O2 -G0 -DSDK_PORT -DSDK_X86 -DSDK_TS -DSDK_4M -DSDK_FINALROM -DNNS_FINALROM -DSDK_VERSION_MAJOR=4 -DSDK_BUILD_PSP
BUILD_PRX = 1
PSP_LARGE_MEMORY = 1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Native PSP backing proof
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
