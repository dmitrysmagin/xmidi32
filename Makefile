WTEMP   := $(shell cygpath -w /tmp 2>/dev/null)

CFLAGS  = -O2 -I src $(shell pkg-config --cflags sdl2)
LIBS    = $(shell pkg-config --libs sdl2)
SRCS    = src/xmi_play.c src/backend.c src/opl3.c src/xmidi32_api.c src/xmidi32_channel.c src/xmidi32_control.c \
          src/xmidi32_count.c src/xmidi32_find_seq.c src/xmidi32_flush.c src/xmidi32_globals.c src/xmidi32_init.c \
          src/xmidi32_meta.c src/xmidi32_midi.c src/xmidi32_note_on.c src/xmidi32_register.c src/xmidi32_reset.c \
          src/xmidi32_serve.c src/xmidi32_sysex.c src/xmidi32_tempo.c src/xmidi32_timbre.c src/xmidi32_utils.c \
          src/xmidi32_volume.c src/xmidi32_yamaha.c src/xmidi32_yamaha_tables.c src/fat_opl.c src/timbre_bank.c \
          src/sdl_audio.c

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
TARGET  = xmi_play
else ifeq ($(UNAME_S),Darwin)
TARGET  = xmi_play
else
TARGET  = xmi_play.exe
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	TMP='$(WTEMP)' TEMP='$(WTEMP)' gcc $(CFLAGS) $(SRCS) $(LIBS) -o $@

clean:
	rm -f $(TARGET)
