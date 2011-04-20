/**
 * @file rem_vidconv.h  Video colorspace conversion
 *
 * Copyright (C) 2010 Creytiv.com
 */


enum {
	VIDCONV_HFLIP     = 1<<0,
	VIDCONV_VFLIP     = 1<<1,
	VIDCONV_ROTATE90  = 1<<2,
	VIDCONV_ROTATE270 = 1<<3
};

struct vidconv_ctx {
	bool inited;
	void *sws;

	int16_t CRV[256];
	int16_t CGU[256];
	int16_t CGV[256];
	int16_t CBU[256];
};


void vidconv_process(struct vidconv_ctx *ctx, struct vidframe *dst,
		     const struct vidframe *src, int flags);


void vidconv_rotate(struct vidframe *dst, const struct vidframe *src,
		    int degrees);
