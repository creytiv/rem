#include <string.h>
#include <re.h>
#include <rem_vid.h>
#include <rem_dsp.h>
#include <rem_vidconv.h>
#include "vconv.h"


static inline void copy2x2_yuyv422(uint8_t *y0, uint8_t *y1,
				   uint8_t *u, uint8_t *v,
				   const uint8_t *p0, const uint8_t *p1)
{
	y0[0] = p0[0];
	y0[1] = p0[2];
	y1[0] = p1[0];
	y1[1] = p1[2];
	u [0] = p0[1];
	v [0] = p0[3];
}


static inline void copy2x2_uyvy422(uint8_t *y0, uint8_t *y1,
				   uint8_t *u, uint8_t *v,
				   const uint8_t *p0, const uint8_t *p1)
{
	y0[0] = p0[1];
	y0[1] = p0[3];
	y1[0] = p1[1];
	y1[1] = p1[3];
	u [0] = p0[0];
	v [0] = p0[2];
}


/* native endian */
static inline void copy2x2_rgb32(uint8_t *y0, uint8_t *y1,
				 uint8_t *u, uint8_t *v,
				 const uint8_t *p0, const uint8_t *p1)
{
	uint32_t x0 = ((uint32_t *)p0)[0];
	uint32_t x1 = ((uint32_t *)p0)[1];
	uint32_t x2 = ((uint32_t *)p1)[0];
	uint32_t x3 = ((uint32_t *)p1)[1];

	/* Alpha-value is ignored */

	y0[0] = rgb2y(x0 >> 16, x0 >> 8, x0);
	y0[1] = rgb2y(x1 >> 16, x1 >> 8, x1);
	y1[0] = rgb2y(x2 >> 16, x2 >> 8, x2);
	y1[1] = rgb2y(x3 >> 16, x3 >> 8, x3);
	u[0]  = rgb2u(x0 >> 16, x0 >> 8, x0);
	v[0]  = rgb2v(x0 >> 16, x0 >> 8, x0);
}


static inline void copy2x2_argb(uint8_t *y0, uint8_t *y1,
				uint8_t *u, uint8_t *v,
				const uint8_t *p0, const uint8_t *p1)
{
	y0[0] = rgb2y(p0[1], p0[2], p0[3]);
	y0[1] = rgb2y(p0[5], p0[6], p0[7]);
	y1[0] = rgb2y(p1[1], p1[2], p1[3]);
	y1[1] = rgb2y(p1[5], p1[6], p1[7]);
	u[0]  = rgb2u(p0[1], p0[2], p0[3]);
	v[0]  = rgb2v(p0[1], p0[2], p0[3]);
}


static inline void copy2x2_nv12(uint8_t *y0, uint8_t *y1,
				uint8_t *u, uint8_t *v,
				const uint8_t *p0, const uint8_t *p1,
				const uint8_t *puv)
{
	y0[0] = p0[0];
	y0[1] = p0[1];
	y1[0] = p1[0];
	y1[1] = p1[1];
	u [0] = puv[0];
	v [0] = puv[1];
}


/**
 * Convert from generic Packed format to Planar YUV420P
 */
void vidconv_packed_to_yuv420p(struct vidframe *dst,
			       const struct vidframe *src, int flags)
{
	const uint8_t *p0, *p1, *puv;
	uint8_t *y0 = dst->data[0];
	uint8_t *y1 = dst->data[0] + dst->linesize[0];
	uint8_t *u = dst->data[1], *v = dst->data[2];
	int px, puvx, h, w;

	/* vertical flip -- read source in opposite direction */
	if (flags & VIDCONV_VFLIP) {
		p0  = src->data[0] + src->linesize[0] * (dst->size.h - 1);
		p1  = src->data[0] + src->linesize[0] * (dst->size.h - 2);
		puv = src->data[1] + src->linesize[1] * (dst->size.h/2 - 1);

		px   = -src->linesize[0] * 2;
		puvx = -src->linesize[1];
	}
	else {
		p0  = src->data[0];
		p1  = src->data[0] + src->linesize[0];
		puv = src->data[1];

		px   = src->linesize[0] * 2;
		puvx = src->linesize[1];
	}

	/* 2 lines */
	for (h=0; h<dst->size.h/2; h++) {

		int pw = 0;

		/* 2 pixels */
		for (w=0; w<dst->size.w/2; w++) {

			/*
			 * todo: add hflip, each 2xpixel must be
			 *       horizontally swapped:
			 *
			 * .-------.             .-------.
			 * | 1 | 2 |             | 2 | 1 |
			 * |---+---|    =====>   |---+---|
			 * | 3 | 4 |             | 4 | 3 |
			 * '-------'             '-------'
			 */
			switch (src->fmt) {

			case VID_FMT_YUYV422:

				copy2x2_yuyv422(&y0[2*w], &y1[2*w],
						&u[w], &v[w],
						&p0[pw], &p1[pw]);

				pw += 4;
				break;

			case VID_FMT_UYVY422:

				copy2x2_uyvy422(&y0[2*w], &y1[2*w],
						&u[w], &v[w],
						&p0[pw], &p1[pw]);

				pw += 4;
				break;

			case VID_FMT_RGB32:

				copy2x2_rgb32(&y0[2*w], &y1[2*w], &u[w], &v[w],
					      &p0[pw], &p1[pw]);

				pw += 8; /* STEP */
				break;

			case VID_FMT_ARGB:

				copy2x2_argb(&y0[2*w], &y1[2*w], &u[w], &v[w],
					     &p0[pw], &p1[pw]);

				pw += 8; /* STEP */
				break;

			case VID_FMT_NV12:

				copy2x2_nv12(&y0[2*w], &y1[2*w],
					     &u[w], &v[w],
					     &p0[pw], &p1[pw], &puv[pw]);

				pw += 2;
				break;

			default:
				re_printf("pack2yuv: no fmt %d\n", src->fmt);
				return;
			}
		}

		p0  += px;
		p1  += px;
		puv += puvx;

		y0 += (dst->linesize[0] * 2);
		y1 += (dst->linesize[0] * 2);
		u  += dst->linesize[1];
		v  += dst->linesize[2];
	}
}


void vidconv_packed_to_yuv420p_test(struct vidframe *dst,
				    const struct vidframe *src, int flags)
{
	uint8_t *ydst, *udst, *vdst;
	const uint8_t *psrc;
	int bandl, bandr, bandt, bandb;
	int px, w, h;

	re_printf("packed test: %d x %d   ---->   %d x %d\n",
		  src->size.w, src->size.h,
		  dst->size.w, dst->size.h);

	ydst = dst->data[0];
	udst = dst->data[1];
	vdst = dst->data[2];

	/* vertical flip -- read source in opposite direction */
	if (flags & VIDCONV_VFLIP) {
		psrc = src->data[0] + src->linesize[0] * (src->size.h - 1);
		px   = -src->linesize[0];
	}
	else {
		psrc = src->data[0];
		px   = src->linesize[0];
	}

	/* calculate band offsets */
	bandl = (dst->size.w - src->size.w) / 2;
	bandr = dst->size.w - bandl;
	bandt = (dst->size.h - src->size.h) / 2;
	bandb = dst->size.h - bandt;

	re_printf("bands: left=%d right=%d top=%d bottom=%d\n",
		  bandl, bandr, bandt, bandb);

	/* for-loop operates on the destination frame */

	for (h = 0; h < dst->size.h; h++) {

		int sw;

		if (flags & VIDCONV_HFLIP)
			sw = src->linesize[0]; // XXX use dst instead?
		else
			sw = 0;

		if (h < bandt || h >= bandb) {

			/* clear pixels */
			memset(ydst, rgb2y(0, 0, 0), dst->linesize[0]);

			if (!(h & 1)) {
				memset(udst, rgb2u(0, 0, 0), dst->linesize[1]);
				memset(vdst, rgb2v(0, 0, 0), dst->linesize[2]);
			}

			goto next;
		}

		for (w = 0; w < dst->size.w; w++) {

			int step;

			if (w < bandl || w >= bandr) {

				/* clear pixel */

				ydst[w] = rgb2y(0x00, 0x00, 0x00);

				if (!(h & 1) && !(w & 1)) {
					udst[w/2] = rgb2u(0x00, 0x00, 0x00);
					vdst[w/2] = rgb2v(0x00, 0x00, 0x00);
				}

				continue;
			}

			switch (src->fmt) {

			case VID_FMT_YUYV422:

				ydst[w] = psrc[sw];

				if (!(h & 1) && !(w & 1)) {
					udst[w/2] = psrc[sw + 1];
					vdst[w/2] = psrc[sw + 3];
				}

				step = 2;
				break;

			default:
				re_printf("no src fmt %d\n", src->fmt);
				break;
			}

			if (flags & VIDCONV_HFLIP)
				sw -= step;
			else
				sw += step;
		}

		psrc += px;

	next:
		ydst += dst->linesize[0];

		if (h & 1) {
			udst += dst->linesize[1];
			vdst += dst->linesize[2];
		}
	}
}
