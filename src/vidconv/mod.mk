#
# mod.mk
#
# Copyright (C) 2010 Creytiv.com
#

SRCS	+= vidconv/vconv.c
SRCS	+= vidconv/pack2yuv.c

ifneq ($(USE_FFMPEG),)
SRCS	+= vidconv/sws.c
endif
