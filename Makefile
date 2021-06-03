#
# Makefile
#
# Copyright (C) 2010 Creytiv.com
#

# Main version number
VER_MAJOR := 1
VER_MINOR := 0
VER_PATCH := 0

# Development version, comment out on a release
# Increment for breaking changes (dev2, dev3...)
VER_PRE   := dev

# Libtool similar ABI versioning
# https://github.com/baresip/re/wiki/ABI-Versioning
ABI_CUR   := 0
ABI_REV   := 0
ABI_AGE   := 0

ABI_MAJOR := $(shell expr $(ABI_CUR) - $(ABI_AGE))

PROJECT   := rem
ifeq ($(VER_PRE),)
VERSION   := $(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)
else
VERSION   := $(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)-$(VER_PRE)
endif
OPT_SPEED := 1
LIBRE_MIN := 2.0.1-dev

ifndef LIBRE_MK
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
endif

include $(LIBRE_MK)

# Dependency Checks
LIBRE_PKG_PATH  := $(shell [ -f ../re/libre.pc ] && echo "../re/")

ifneq ($(PKG_CONFIG),)
LIBRE_PKG := $(shell PKG_CONFIG_PATH=$(LIBRE_PKG_PATH) \
	pkg-config --exists "libre >= $(LIBRE_MIN)" && echo "yes")

ifeq ($(LIBRE_PKG),)
$(error bad libre version, required version is ">= $(LIBRE_MIN)". \
	LIBRE_MK: $(LIBRE_MK))
endif
endif

# List of modules
MODULES += fir goertzel
MODULES += g711
MODULES += aubuf aufile auresamp autone dtmf
MODULES += au auconv aulevel auframe

ifneq ($(HAVE_LIBPTHREAD),)
MODULES += aumix vidmix
endif

MODULES += vid vidconv
MODULES += aac
MODULES += avc
MODULES += h264

LIBS    += -lm

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


# XXX
ifneq ($(HAVE_ARMV6),)
CFLAGS		+= -DHAVE_ARMV6=1
endif
ifneq ($(HAVE_NEON),)
CFLAGS		+= -DHAVE_NEON=1
endif


MODMKS	:= $(patsubst %,src/%/mod.mk,$(MODULES))
SHARED  := librem$(LIB_SUFFIX)
SHARED_SONAME  := $(SHARED).$(ABI_MAJOR)
SHARED_FILE    := $(SHARED).$(ABI_MAJOR).$(ABI_AGE).$(ABI_REV)
STATIC	:= librem.a

ifeq ($(OS),linux)
SH_LFLAGS      += -Wl,-soname,$(SHARED_SONAME)
endif

include $(MODMKS)


OBJS	?= $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(SRCS)))
OBJS	+= $(patsubst %.S,$(BUILD)/%.o,$(filter %.S,$(SRCS)))


all: $(SHARED) $(STATIC)


-include $(OBJS:.o=.d)


$(SHARED): $(OBJS) librem.pc
	@echo "  LD      $@"
	@$(LD) $(LFLAGS) $(SH_LFLAGS) $(OBJS) -L$(LIBRE_SO) -lre $(LIBS) -o $@


$(STATIC): $(OBJS) librem.pc
	@echo "  AR      $@"
	@$(AR) $(AFLAGS) $@ $(OBJS)
ifneq ($(RANLIB),)
	@$(RANLIB) $@
endif

librem.pc:
	@echo 'prefix='$(PREFIX) > librem.pc
	@echo 'exec_prefix=$${prefix}' >> librem.pc
	@echo 'libdir=$${prefix}/lib' >> librem.pc
	@echo 'includedir=$${prefix}/include/rem' >> librem.pc
	@echo '' >> librem.pc
	@echo 'Name: librem' >> librem.pc
	@echo 'Description: Audio and video processing media library' \
		>> librem.pc
	@echo 'Version: '$(VERSION) >> librem.pc
	@echo 'URL: https://github.com/baresip/rem' >> librem.pc
	@echo 'Libs: -L$${libdir} -lrem -lre' >> librem.pc
	@echo 'Cflags: -I$${includedir}' >> librem.pc

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
	@rm -rf $(SHARED) $(STATIC) librem.pc test.d test.o test $(BUILD)


install: $(SHARED) $(STATIC) librem.pc
	@mkdir -p $(DESTDIR)$(LIBDIR) $(DESTDIR)$(LIBDIR)/pkgconfig \
		$(DESTDIR)$(INCDIR)
	$(INSTALL) -m 0644 $(shell find include -name "*.h") \
		$(DESTDIR)$(INCDIR)
	$(INSTALL) -m 0755 $(SHARED) $(DESTDIR)$(LIBDIR)/$(SHARED_FILE)
	cd $(DESTDIR)$(LIBDIR) && ln -sf $(SHARED_FILE) $(SHARED) && \
		ln -sf $(SHARED_FILE) $(SHARED_SONAME)
	$(INSTALL) -m 0755 $(STATIC) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0644 librem.pc $(DESTDIR)$(LIBDIR)/pkgconfig

.PHONY: uninstall
uninstall:
	@rm -rf $(DESTDIR)$(INCDIR)
	@rm -f $(DESTDIR)$(LIBDIR)/$(SHARED)
	@rm -f $(DESTDIR)$(LIBDIR)/$(STATIC)
	@rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/librem.pc

-include test.d

test.o:	test.c
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)

test$(BIN_SUFFIX): test.o $(SHARED) $(STATIC)
	@echo "  LD      $@"
	@$(LD) $(LFLAGS) $< -L. -lrem -lre $(LIBS) -o $@
