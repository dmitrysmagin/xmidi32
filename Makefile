.DEFAULT_GOAL := all

TARGET      := xmi_play.exe
DUMP_TARGET := dump_wav.exe
CC = gcc
CFLAGS = -Wall -Wextra -Os -g -I. -Isrc
LDFLAGS =
LIBS =

SDL_CFLAGS  := $(shell pkg-config --cflags sdl2 | sed 's/-Dmain=SDL_main//')
SDL_LDFLAGS := $(shell pkg-config --libs sdl2)

ALL_LDFLAGS := $(SDL_LDFLAGS) -lm
ifdef SYSTEMROOT
  ALL_LDFLAGS += -mconsole
endif

SRC_FILES = $(filter-out src/sdltest.c src/dump_wav.c,$(wildcard src/*.c))
OBJ_FILES = $(SRC_FILES:.c=.o)

all: $(TARGET)

BUILD_TMP := $(CURDIR)/.build_tmp
TMP_ENV   := TMP=$(BUILD_TMP) TEMP=$(BUILD_TMP) TMPDIR=$(BUILD_TMP)

$(BUILD_TMP):
	mkdir -p $(BUILD_TMP)

$(TARGET): $(OBJ_FILES) | $(BUILD_TMP)
	printf '%s\n' $^ > link.rsp
	$(TMP_ENV) $(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ @link.rsp $(ALL_LDFLAGS)
	rm -f link.rsp

%.o: %.c | $(BUILD_TMP)
	$(TMP_ENV) $(CC) $(CFLAGS) $(SDL_CFLAGS) -c -o $@ $<

$(DUMP_TARGET): src/dump_wav.o $(filter-out src/xmi_play.o,$(OBJ_FILES)) | $(BUILD_TMP)
	printf '%s\n' $^ > link.rsp
	$(TMP_ENV) $(CC) $(CFLAGS) -o $@ @link.rsp $(LDFLAGS) -lm
	rm -f link.rsp

clean:
	rm -f $(TARGET) $(DUMP_TARGET) $(OBJ_FILES) src/dump_wav.o link.rsp
	rm -rf $(BUILD_TMP)

.PHONY: all clean
