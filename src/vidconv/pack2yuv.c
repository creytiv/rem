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


/**
 * Convert from generic Packed format to Planar YUV420P
 */
void vidconv_packed_to_yuv420p(struct vidframe *dst,
			       const struct vidframe *src, int flags)
{
	const uint8_t *p0, *p1;
	uint8_t *y0 = dst->data[0];
	uint8_t *y1 = dst->data[0] + dst->linesize[0];
	uint8_t *u = dst->data[1], *v = dst->data[2];
	int px, h, w;

	re_printf("packed %s to YUV420P\n", vidfmt_name(src->fmt));

	/* vertical flip -- read source in opposite direction */
	if (flags & VIDCONV_VFLIP) {
		p0 = (src->data[0] + src->linesize[0] * (dst->size.h - 1));
		p1 = (src->data[0] + src->linesize[0] * (dst->size.h - 2));
		px = -src->linesize[0] * 2;
	}
	else {
		p0 = src->data[0];
		p1 = src->data[0] + src->linesize[0];
		px = src->linesize[0] * 2;
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

			case VID_FMT_ARGB:

				copy2x2_argb(&y0[2*w], &y1[2*w], &u[w], &v[w],
					     &p0[pw], &p1[pw]);

				pw += 8; /* STEP */
				break;

			default:
				re_printf("no fmt %d\n", src->fmt);
				return;
			}
		}

		p0 += px;
		p1 += px;

		y0 += (dst->linesize[0] * 2);
		y1 += (dst->linesize[0] * 2);
		u  += dst->linesize[1];
		v  += dst->linesize[2];
	}
}
