/**
 * @file rem_vidconv.h  Video colorspace conversion
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** Vidconv flags */
enum {
	VIDCONV_HFLIP     = 1<<0,
	VIDCONV_VFLIP     = 1<<1,
	VIDCONV_ROTATE90  = 1<<2,
	VIDCONV_ROTATE180 = VIDCONV_HFLIP | VIDCONV_VFLIP,
	VIDCONV_ROTATE270 = VIDCONV_ROTATE180 | VIDCONV_ROTATE90
};


void vidconv_process(struct vidframe *dst, const struct vidframe *src,
		     struct vidrect *r);


// todo: temp
void vidscale(struct vidframe *dst, const struct vidframe *src,
	      struct vidrect *r);
