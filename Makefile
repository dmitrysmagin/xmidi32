MSYS := $(patsubst %/bin/,%,$(dir $(shell which gcc)))
CC = gcc
CFLAGS = -Wall -Wextra -Os -g -I. -Isrc
LDFLAGS =
LIBS =

SDL_CFLAGS = -I$(MSYS)/include -mconsole
SDL_LIBS = -L$(MSYS)/lib -lmingw32 -lSDL2main -lSDL2

SRC_FILES = $(wildcard src/*.c)
GLUE_FILES = opl3.c backend.c sdl_audio.c timbre_bank.c sample_ad.c sample_opl.c

OBJ_FILES = $(SRC_FILES:.c=.o) $(GLUE_FILES:.c=.o)

xmi_play.exe: xmi_play.o $(OBJ_FILES)
	printf '%s\n' $^ > link.rsp
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ @link.rsp $(LDFLAGS) $(SDL_LIBS) -lm
	rm -f link.rsp

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c -o $@ $<

dump_wav.exe: dump_wav.o $(OBJ_FILES)
	printf '%s\n' $^ > link.rsp
	$(CC) $(CFLAGS) -o $@ @link.rsp $(LDFLAGS) -lm
	rm -f link.rsp

clean:
	rm -f xmi_play xmi_play.exe xmi_play.o dump_wav.exe dump_wav.o $(OBJ_FILES) link.rsp

.PHONY: clean
