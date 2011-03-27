#
# Makefile
#
# Copyright (C) 2010 Creytiv.com
#

# Master version number
VER_MAJOR := 0
VER_MINOR := 1
VER_PATCH := 0

PROJECT   := rem
VERSION   := 0.1.0

LIBRE_MK  := $(shell [ -f ../re/mk/re.mk ] && \
	echo "../re/mk/re.mk")
ifeq ($(LIBRE_MK),)
LIBRE_MK  := $(shell [ -f /usr/share/re/re.mk ] && \
	echo "/usr/share/re/re.mk")
endif
ifeq ($(LIBRE_MK),)
LIBRE_MK  := $(shell [ -f /usr/local/share/re/re.mk ] && \
	echo "/usr/local/share/re/re.mk")
endif

include $(LIBRE_MK)

# List of modules
MODULES += au vid

INSTALL := install
ifeq ($(DESTDIR),)
PREFIX  := /usr/local
else
PREFIX  := /usr
endif
ifeq ($(LIBDIR),)
LIBDIR  := $(PREFIX)/lib
endif
INCDIR  := $(PREFIX)/include/rem
CFLAGS	+= -I$(LIBRE_INC) -Iinclude

MODMKS	:= $(patsubst %,src/%/mod.mk,$(MODULES))
SHARED  := librem$(LIB_SUFFIX)
STATIC	:= librem.a

LIBS += -lswscale -lavcodec


include $(MODMKS)


OBJS	?= $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(SRCS)))
OBJS	+= $(patsubst %.S,$(BUILD)/%.o,$(filter %.S,$(SRCS)))


all: $(SHARED) $(STATIC)


-include $(OBJS:.o=.d)


$(SHARED): $(OBJS)
	@echo "  LD      $@"
	@$(LD) $(LFLAGS) $(SH_LFLAGS) $^ -L$(LIBRE_SO) -lre $(LIBS) -o $@


$(STATIC): $(OBJS)
	@echo "  AR      $@"
	@$(AR) $(AFLAGS) $@ $^
ifneq ($(RANLIB),)
	@$(RANLIB) $@
endif

$(BUILD)/%.o: src/%.c $(BUILD) Makefile $(MK) $(MODMKS)
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)


$(BUILD)/%.o: src/%.S $(BUILD) Makefile $(MK) $(MODMKS)
	@echo "  AS      $@"
	@$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)


$(BUILD): Makefile $(MK) $(MODMKS)
	@mkdir -p $(patsubst %,$(BUILD)/%,$(sort $(dir $(SRCS))))
	@touch $@


.PHONY: clean
clean:
	@rm -rf $(SHARED) $(STATIC) $(BUILD)/


install: $(SHARED) $(STATIC)
	@mkdir -p $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCDIR) $(DESTDIR)$(MKDIR)
	$(INSTALL) -m 0644 $(shell find include -name "*.h") \
		$(DESTDIR)$(INCDIR)
	$(INSTALL) -m 0755 $(SHARED) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0755 $(STATIC) $(DESTDIR)$(LIBDIR)

-include test.d

test.o:	test.c
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)

test$(BIN_SUFFIX): test.o $(SHARED) $(STATIC)
	@echo "  LD      $@"
	@$(LD) $(LFLAGS) $< -L. -lrem -lre $(LIBS) -o $@
