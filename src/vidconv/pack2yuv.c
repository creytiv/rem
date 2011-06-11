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
