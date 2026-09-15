TARGET = ss-native-codegen
OBJS = main.o
CFLAGS = -O2 -G0 -std=gnu99 -ffunction-sections -fdata-sections -fno-inline-functions
LDFLAGS = -Wl,--gc-sections
LIBS = -lpsppower
BUILD_PRX = 1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = SoulSilver native assembly bridge proof
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
main.o: generated.c
