# Makefile for ghost.kpm - GhostPatch Kernel Stealth Module

ifeq ($(OS), Windows_NT)
    PLATFORM := windows-x86_64
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S), Darwin)
        PLATFORM := darwin-x86_64
    else
        PLATFORM := linux-x86_64
    endif
endif

ifndef TARGET_COMPILE
	NDK_PATH ?= $(HOME)/Library/Android/sdk/ndk/android-ndk-r25c
	export TARGET_COMPILE=$(NDK_PATH)/toolchains/llvm/prebuilt/$(PLATFORM)/bin/
endif

ifndef KP_DIR
    KP_DIR = ../KernelPatch-0.11.3
endif

CC = $(TARGET_COMPILE)aarch64-linux-android31-clang
LD = $(TARGET_COMPILE)ld.lld
STRIP = $(TARGET_COMPILE)llvm-strip

INCLUDE_DIRS := . include patch/include linux/include linux/arch/arm64/include linux/tools/arch/arm64/include

INCLUDE_FLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(KP_DIR)/kernel/$(dir))

CFLAGS = $(INCLUDE_FLAGS) -Wall -O2 -fno-PIC -fno-asynchronous-unwind-tables \
         -fno-stack-protector -fno-unwind-tables -U_FORTIFY_SOURCE \
         -fno-common -fvisibility=hidden

LDFLAGS = -s

SRCS = src/ghost_main.c
OBJS = $(SRCS:.c=.o)

all: ghost.kpm

ghost.kpm: $(OBJS)
	$(CC) $(LDFLAGS) -r -o $@ $^
	$(STRIP) -g --strip-unneeded --strip-debug \
		--remove-section=.comment --remove-section=.note.GNU-stack $@
	@echo ""
	@echo "=== ghost.kpm built successfully ==="
	@ls -lh ghost.kpm

%.o: %.c
	$(CC) $(CFLAGS) -Thello.lds -c -o $@ $<

clean:
	rm -f $(OBJS) ghost.kpm

.PHONY: all clean
