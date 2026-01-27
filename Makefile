SHELL := /bin/bash

OW_ROOT ?= /Users/dmitry/open-watcom-v2/rel
LIBPATH ?= $(OW_ROOT)/lib286

WCC   := bwcc
WLINK := bwlink
WDIS  := bwdis


INCDIR  ?= inc
PROJECT ?= openpole
TARGET ?= _openpol.exe
OUT ?= $(PROJECT).exe
OBJDIR ?= build
SDIR   ?= s
MAPFILE = $(SDIR)/$(PROJECT).map

MODE ?= debug

CFLAGS_BASE  = -bt=dos -mm -fpc -i=$(OW_ROOT)/h -i=$(INCDIR)
LDFLAGS_BASE =  libpath $(LIBPATH) libpath $(LIBPATH)/dos format DOS

CFLAGS_DEBUG    = -d3 -od -dDBGON -wx -w3
LDFLAGS_DEBUG   = option stack=4096 option map debug watcom all

CFLAGS_RELEASE  = -d0 -onatx -zp4 -wx -dRELEASE
LDFLAGS_RELEASE = option stack=2048 option statics option map

ifeq ($(MODE),release)
  CFLAGS  := $(CFLAGS_BASE)  $(CFLAGS_RELEASE)
  LDFLAGS := $(LDFLAGS_BASE) $(LDFLAGS_RELEASE)
else
  CFLAGS  := $(CFLAGS_BASE)  $(CFLAGS_DEBUG)
  LDFLAGS := $(LDFLAGS_BASE) $(LDFLAGS_DEBUG)
endif

SRCS = \
	openpole.c \
	src/stackpat.c \
	src/globals.c \
	src/pole.c \
	src/sound.c \
	src/girl.c \
	src/resources.c \
	src/sys_delay.c \
	src/pole_dialog.c \
	src/rle_draw.c \
	src/ega_text.c \
	src/baraban.c \
	src/yakubovich.c \
	src/input.c \
	src/score.c \
	src/words.c \
	src/ui_keymap.c \
	src/ui_choice.c \
	src/ui_answer.c \
	src/ui_hof.c \
	src/debuglog.c \
	src/ega_draw.c \
	src/random.c \
	src/cstr.c \
	src/startscreen.c \
	src/bgi_mouse.c \
	src/bgi_palette.c \
	src/boss_mode.c \
	src/sale_info.c \
	src/moneybox.c \
	src/prize.c \
	src/letter.c \
	src/opponents.c \
	src/end_screen.c \

OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

.PHONY: all
all: $(TARGET) disasm post

$(OBJDIR) $(SDIR):
	@mkdir -p "$@"

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@mkdir -p "$(dir $@)"
	$(WCC) $(strip $(CFLAGS)) -fo=$@ $<

$(TARGET): $(OBJS) | $(SDIR)
	$(WLINK) name $@ option map=$(MAPFILE) $(strip $(LDFLAGS)) file { $(OBJS) }

.PHONY: disasm
disasm: $(OBJS) | $(SDIR)
	@for src in $(SRCS); do \
	  obj="$(OBJDIR)/$${src%.c}.o"; \
	  base="$$(basename "$${src%.c}")"; \
	  $(WDIS) -l="s/$${base}.s" -e -p -fu -s=$$src $$obj; \
	done

.PHONY: post
post: $(TARGET) | $(SDIR)
	@if [ -f "$(TARGET)" ]; then \
	  if [ -f "build_tools/remove_FPU_opcodes.py" ]; then \
	    python3 ./build_tools/remove_FPU_opcodes.py "$(TARGET)" --out "$(OUT)" || true; \
	  fi; \
	  hex="$$(od -An -tx2 -N 2 -j 8 "$(TARGET)" | tr -cd '0-9a-fA-F')"; \
	  [ -z "$$hex" ] && hex=0; \
	  p=$$((16#$$hex)); \
	  printf ""$(OUT)" 0x%x paragraphs — so map 0000:0000 starts from 0000:%04x in file\n" "$$p" "$$((p*16))"; \
	fi

.PHONY: debug 
debug:
	$(MAKE) MODE=debug all

.PHONY: release
release: clean
	$(MAKE) MODE=release all

.PHONY: clean
clean:
	rm -rf "$(OBJDIR)" "$(SDIR)"
	rm -f "$(TARGET)" "$(OUT)"
	rm -f *.o *.obj *.err *.map *.s

.PHONY: rebuild
rebuild: clean all