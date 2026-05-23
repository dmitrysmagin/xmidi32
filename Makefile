TARGET      := xmi_play.exe
DUMP_TARGET := dump_wav.exe
CC = gcc
CFLAGS = -Wall -Wextra -Os -g -I. -Isrc
LDFLAGS =
LIBS =

SDL_CONFIG  := sdl2-config
SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags | sed 's/-Dmain=SDL_main//')
SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)

ifeq ($(OS),Windows_NT)
  ALL_LDFLAGS := $(SDL_LDFLAGS) -lm -mconsole
else
  ALL_LDFLAGS := $(SDL_LDFLAGS) -lm
endif

SRC_FILES = $(wildcard src/*.c)
GLUE_FILES = opl3.c backend.c sdl_audio.c timbre_bank.c sample_ad.c sample_opl.c
OBJ_FILES = $(SRC_FILES:.c=.o) $(GLUE_FILES:.c=.o)

$(TARGET): xmi_play.o $(OBJ_FILES)
	printf '%s\n' $^ > link.rsp
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ @link.rsp $(ALL_LDFLAGS)
	rm -f link.rsp

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c -o $@ $<

$(DUMP_TARGET): dump_wav.o $(OBJ_FILES)
	printf '%s\n' $^ > link.rsp
	$(CC) $(CFLAGS) -o $@ @link.rsp $(LDFLAGS) -lm
	rm -f link.rsp

clean:
	rm -f $(TARGET) xmi_play.o $(DUMP_TARGET) dump_wav.o $(OBJ_FILES) link.rsp

.PHONY: clean
