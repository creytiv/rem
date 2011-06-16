#include <string.h>
#include <re.h>
#include <rem_vid.h>
#include <rem_dsp.h>
#include <rem_vidconv.h>
#include "vconv.h"


typedef void (block_h)(uint8_t *d0, uint8_t *d1, uint8_t *d2, int lsd,
		       const uint8_t *s0, const uint8_t *s1,
		       const uint8_t *s2, int lss);


static void yuv420p_to_yuv420p(uint8_t *d0, uint8_t *d1, uint8_t *d2, int lsd,
			       const uint8_t *s0, const uint8_t *s1,
			       const uint8_t *s2, int lss)
{
	/* Y */
	d0[0]     = s0[0];
	d0[1]     = s0[1];
	d0[lsd  ] = s0[lss  ];
	d0[lsd+1] = s0[lss+1];

	/* U */
	d1[0] = s1[0];

	/* V */
	d2[0] = s2[0];
}


static void yuyv422_to_yuv420p(uint8_t *d0, uint8_t *d1, uint8_t *d2, int lsd,
			       const uint8_t *s0, const uint8_t *s1,
			       const uint8_t *s2, int lss)
{
	(void)s1;
	(void)s2;

	/* Y */
	d0[0]     = s0[0];
	d0[1]     = s0[2];
	d0[lsd  ] = s0[lss  ];
	d0[lsd+1] = s0[lss+2];

	/* U */
	d1[0] = s0[1];

	/* V */
	d2[0] = s0[3];
}


/** Pixel conversion table:  [src][dst] */
static block_h *conv_table[2][2] = {
	{yuv420p_to_yuv420p, NULL},
	{yuyv422_to_yuv420p, NULL}
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
	unsigned x, y, xd, yd, xs, ys, xs2, ys2, lsd, lss;
	unsigned id, is;
	double rw, rh;
	block_h *blockh;

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
	blockh = conv_table[src->fmt][dst->fmt];
	if (!blockh) {
		re_printf("vidconv: no pixel block handler found for"
			  " %s -> %s\n", vidfmt_name(src->fmt),
			  vidfmt_name(dst->fmt));
		return;
	}

	rw = (double)src->size.w / (double)r->w;
	rh = (double)src->size.h / (double)r->h;

	lsd = dst->linesize[0];
	lss = src->linesize[0];

	/* align 2 pixels */
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

			// todo: hack, different steps for each format
			if (src->fmt == VID_FMT_YUYV422)
				xs *= 2;

			id = xd/2        + yd*lsd/4;
			is = (xs & ~1)/2 + (ys & ~1)*lss/4;

			blockh(&dst->data[0][xd + yd*lsd],
			       &dst->data[1][id],
			       &dst->data[2][id],
			       lsd,
			       &src->data[0][xs + ys*lss],
			       &src->data[1][is],
			       &src->data[2][is],
			       lss);
		}
	}
}
