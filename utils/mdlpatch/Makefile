#
# Makefile for mdlpatch - GoldSrc .mdl root-bone origin patcher
#
# This is a host tool (runs on the build machine, not in-game).
# Builds as native architecture (not forced to 32-bit).
#

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter
TARGET = mdlpatch

all: $(TARGET)

$(TARGET): mdlpatch.c
	$(CC) $(CFLAGS) -o $@ $< -lm

clean:
	rm -f $(TARGET)

.PHONY: all clean
