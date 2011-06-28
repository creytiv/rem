#include <string.h>
#include <re.h>
#include <rem_vid.h>
#include <rem_dsp.h>
#include <rem_vidconv.h>
#include "vconv.h"


typedef void (line_h)(int xoffs, unsigned width, double rw,
		      int yd, int ys, int ys2,
		      uint8_t *dd0, uint8_t *dd1, uint8_t *dd2,
		      int lsd,
		      const uint8_t *sd0, const uint8_t *sd1,
		      const uint8_t *sd2, int lss);


static void yuv420p_to_yuv420p(int xoffs, unsigned width, double rw,
			       int yd, int ys, int ys2,
			       uint8_t *dd0, uint8_t *dd1, uint8_t *dd2,
			       int lsd,
			       const uint8_t *ds0, const uint8_t *ds1,
			       const uint8_t *ds2, int lss
			       )
{
	unsigned x, xd, xs, xs2;
	unsigned id, is;
	double xsf = 0, xs2f = rw;

	for (x=0; x<width; x+=2) {

		xd  = x + xoffs;

		xs  = (unsigned)xsf;
		xs2 = (unsigned)xs2f;

		id = xd + yd*lsd;

		dd0[id]         = ds0[xs  + ys*lss];
		dd0[id+1]       = ds0[xs2 + ys*lss];
		dd0[id + lsd]   = ds0[xs  + ys2*lss];
		dd0[id+1 + lsd] = ds0[xs2 + ys2*lss];

		id = xd/2    + yd*lsd/4;
		is = (xs>>1) + (ys>>1)*lss/2;

		dd1[id] = ds1[is];
		dd2[id] = ds2[is];

		xsf  += 2*rw;
		xs2f += 2*rw;
	}
}


static void yuyv422_to_yuv420p(int xoffs, unsigned width, double rw,
			       int yd, int ys, int ys2,
			       uint8_t *dd0, uint8_t *dd1, uint8_t *dd2,
			       int lsd,
			       const uint8_t *sd0, const uint8_t *sd1,
			       const uint8_t *sd2, int lss
			       )
{
	unsigned x, xd, xs;
	unsigned id, is, is2;
	double xsf = 0, xs2f = 1;

	(void)sd1;
	(void)sd2;

	for (x=0; x<width; x+=2) {

		xd  = x + xoffs;

		xs  = (unsigned)(xsf * 2);

		id  = xd + yd*lsd;
		is  = xs + ys*lss;
		is2 = xs  + ys2*lss;

		dd0[id]         = sd0[is];
		dd0[id+1]       = sd0[is + 2];
		dd0[id + lsd]   = sd0[is2];
		dd0[id+1 + lsd] = sd0[is2 + 2];

		id = xd/2 + yd*lsd/4;

		dd1[id] = sd0[is + 1];
		dd2[id] = sd0[is + 3];

		xsf  += 2*rw;
		xs2f += 2*rw;
	}
}


/** Pixel conversion table:  [src][dst] */
static line_h *conv_table[2][2] = {
	{yuv420p_to_yuv420p,      NULL},
	{yuyv422_to_yuv420p,      NULL}
};


/*
 *
 * Speed matches swscale: SWS_BILINEAR
 *
 * todo: optimize (check out SWS_FAST_BILINEAR)
 *
 */

void vidconv_process(struct vidframe *dst, const struct vidframe *src,
		     struct vidrect *r)
{
	struct vidrect rdst;
	unsigned y, yd, ys, ys2, lsd, lss;
	const uint8_t *ds0, *ds1, *ds2;
	uint8_t *dd0, *dd1, *dd2;
	double rw, rh;
	line_h *lineh;

	if (!dst || !src)
		return;

	if (r) {
		if ((int)(r->w - r->x) > dst->size.w ||
		    (int)(r->h - r->y) > dst->size.h) {
			re_printf("rect out of bounds (%u x %u)\n",
				  dst->size.w, dst->size.h);
			return;
		}
	}
	else {
		rdst.x = rdst.y = 0;
		rdst.w = dst->size.w;
		rdst.h = dst->size.h;
		r = &rdst;
	}

	/* Lookup conversion function */
	lineh = conv_table[src->fmt][dst->fmt];
	if (!lineh) {
		re_printf("vidconv: no pixel handler found for"
			  " %s -> %s\n", vidfmt_name(src->fmt),
			  vidfmt_name(dst->fmt));
		return;
	}

	rw = (double)src->size.w / (double)r->w;
	rh = (double)src->size.h / (double)r->h;

	lsd = dst->linesize[0];
	lss = src->linesize[0];

	dd0 = dst->data[0];
	dd1 = dst->data[1];
	dd2 = dst->data[2];

	ds0 = src->data[0];
	ds1 = src->data[1];
	ds2 = src->data[2];

	/* align 2 pixels */
	r->x &= ~1;
	r->y &= ~1;

	for (y=0; y<r->h; y+=2) {

		yd  = y + r->y;

		ys  = (unsigned)(y * rh);
		ys2 = (unsigned)((y+1) * rh);

		lineh(r->x, r->w, rw, yd, ys, ys2,
		      dd0, dd1, dd2, lsd,
		      ds0, ds1, ds2, lss);
	}
}


/*
 *
 * Speed matches swscale: SWS_BILINEAR
 *
 * todo: optimize (check out SWS_FAST_BILINEAR)
 *
 *
 * TODO: reference function from rendezvous, to be removed
 */

void vidconv_scale(struct vidframe *dst, const struct vidframe *src,
		   struct vidrect *r)
{
	unsigned x, y, xd, yd, xs, ys, xs2, ys2, lsd, lss;
	const uint8_t *ds0, *ds1, *ds2;
	uint8_t *dd0, *dd1, *dd2;
	unsigned id, is;
	double rw, rh;

	rw = (double)src->size.w / (double)r->w;
	rh = (double)src->size.h / (double)r->h;

	lsd = dst->linesize[0];
	lss = src->linesize[0];

	dd0 = dst->data[0];
	dd1 = dst->data[1];
	dd2 = dst->data[2];

	ds0 = src->data[0];
	ds1 = src->data[1];
	ds2 = src->data[2];

	r->x &= ~1;
	r->y &= ~1;

	for (y=0; y<r->h; y+=2) {

		yd  = y + r->y;

		ys  = (unsigned)(y * rh);
		ys2 = (unsigned)((y+1) * rh);

		for (x=0; x<r->w; x+=2) {

			xd  = x + r->x;

			xs  = (unsigned)(x * rw);
			xs2 = (unsigned)((x+1) * rw);

			id = xd + yd*lsd;

			dd0[id]         = ds0[xs  + ys*lss];
			dd0[id+1]       = ds0[xs2 + ys*lss];
			dd0[id + lsd]   = ds0[xs  + ys2*lss];
			dd0[id+1 + lsd] = ds0[xs2 + ys2*lss];

			id = xd/2    + yd*lsd/4;
			is = (xs>>1) + (ys>>1)*lss/2;

			dd1[id] = ds1[is];
			dd2[id] = ds2[is];
		}
	}
}
