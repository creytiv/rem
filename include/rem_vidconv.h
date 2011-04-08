/**
 * @file rem_vidconv.h  Video colorspace conversion
 *
 * Copyright (C) 2010 Creytiv.com
 */


void vidconv_process(struct vidframe *dst, const struct vidframe *src,
		     int rotate, bool hflip, bool vflip);
