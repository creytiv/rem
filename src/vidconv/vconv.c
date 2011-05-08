#include <string.h>
#include <re.h>
#include <rem_vid.h>
#include <rem_dsp.h>
#include <rem_vidconv.h>
#include "vconv.h"


void vidconv_process(struct vidconv_ctx *ctx, struct vidframe *dst,
		     const struct vidframe *src, int flags)
{
	if (!dst || !src)
		return;

	if (!dst->data[0]) {
		re_printf("vidconv: invalid dst frame\n");
		return;
	}

	if (src->fmt == dst->fmt) {

		int i;

		for (i=0; i<4; i++) {

			if (dst->data[i] && src->data[i]) {
				memcpy(dst->data[i], src->data[i],
				       dst->linesize[i] * dst->size.h);
			}
		}
	}
	else if (src->fmt == VID_FMT_YUV420P) {

		vidconv_yuv420p_to_packed(dst, src, flags);
	}
	else if (dst->fmt == VID_FMT_YUV420P) {

		vidconv_packed_to_yuv420p(dst, src, flags);
	}
	else {

#ifdef USE_FFMPEG
		if (ctx && !ctx->sws)
			printf("vidconv: using swscale\n");

		vidconv_sws(ctx, dst, src);
#else
		printf("unsupported pixelformat\n");
#endif
	}
}


/**
 * Rotate frame by +/-90 degrees. Frame format must be YUV420P.
 * the size of the destination frame must the same as src but rotated
 *
 * Example:    640x320 ---> 320x640
 */
void vidconv_rotate(struct vidframe *dst, const struct vidframe *src,
		    int degrees)
{
	const uint8_t *ysrc0 = src->data[0];
	const uint8_t *ysrc1 = src->data[0] + src->linesize[0];
	const uint8_t *usrc = src->data[1];
	const uint8_t *vsrc = src->data[2];
	uint8_t *ydst0, *ydst1, *udst, *vdst;
	int ylsz, ulsz, vlsz;
	int w, h, doffs;

	if (!dst || !src)
		return;
	if (dst->fmt != VID_FMT_YUV420P || src->fmt != VID_FMT_YUV420P) {
		re_printf("rotate: invalid format\n");
		return;
	}

	if (dst->size.w != src->size.h || dst->size.h != src->size.w) {
		re_printf("rotate: dst invalid size\n");
		return;
	}

	if (degrees == 90) {

		ylsz = dst->linesize[0] * 2;
		ulsz = dst->linesize[1];
		vlsz = dst->linesize[2];
	}
	else if (degrees == -90) {

		ylsz = -dst->linesize[0] * 2;
		ulsz = -dst->linesize[1];
		vlsz = -dst->linesize[2];
	}
	else {
		re_printf("rotate: invalid degrees %s\n", degrees);
		return;
	}

	for (h = 0; h < src->size.h/2; h++) {

		if (degrees == 90) {

			doffs = (dst->size.h - 2*h - 2);

			ydst0 = dst->data[0] + doffs;
			ydst1 = dst->data[0] + doffs + dst->linesize[0];
			udst  = dst->data[1] + doffs/2;
			vdst  = dst->data[2] + doffs/2;
		}
		else {
			doffs = (dst->size.h - 2) / 2;

			ydst0 = dst->data[0]+2*h+ 2*doffs   *dst->linesize[0];
			ydst1 = dst->data[0]+2*h+(2*doffs+1)*dst->linesize[0];
			udst  = dst->data[1]+  h+   doffs   *dst->linesize[1];
			vdst  = dst->data[2]+  h+   doffs   *dst->linesize[2];
		}

		for (w = 0; w < src->size.w/2; w++) {

			/* rotate and copy block of 2x2 pixels */
			if (degrees == 90) {
				ydst0[1] = ysrc0[2*w];
				ydst1[1] = ysrc0[2*w + 1];
				ydst0[0] = ysrc1[2*w];
				ydst1[0] = ysrc1[2*w + 1];
			}
			else {
				ydst1[0] = ysrc0[2*w];
				ydst0[0] = ysrc0[2*w + 1];
				ydst1[1] = ysrc1[2*w];
				ydst0[1] = ysrc1[2*w + 1];
			}

			udst[0]  = usrc[w];
			vdst[0]  = vsrc[w];

			ydst0 += ylsz;
			ydst1 += ylsz;
			udst  += ulsz;
			vdst  += vlsz;
		}

		/* SRC operates on the "normal" picture */
		ysrc0 += src->linesize[0]*2;
		ysrc1 += src->linesize[0]*2;
		usrc  += src->linesize[1];
		vsrc  += src->linesize[2];
	}
}


/*
 * Example: convert 320 x 240 to 400 x 240
 *
 *
 *                            400
 *           .-------------------------------------.
 *           |     |                         |     |
 *           |     |                         |     |
 *           |     |                         |     |
 *           |     |                         |     |
 *           |black|                         |black| 240
 *           |band |                         |band |
 *           |left |                         |right|
 *           |     |                         |     |
 *           |     |         320             |     |
 *           |     |                         |     |
 *           '-------------------------------------'
 *
 * Find middle X:  dest width / 2 = 400 / 2 = 200
 *
 * Find band width: (dest width - src width) / 2 = (400 - 320) / 2 = 40
 *
 * for-loops operate on destination frame
 */
void vidconv_cropping(struct vidframe *dst, const struct vidframe *src)
{
	const uint8_t *ysrc0 = src->data[0];
	const uint8_t *ysrc1 = src->data[0] + src->linesize[0];
	const uint8_t *usrc = src->data[1];
	const uint8_t *vsrc = src->data[2];
	uint8_t *ydst0, *ydst1, *udst, *vdst;
	int bandl, bandr, bandt, bandb;
	int w, h;

#if 0
	if (src->size.w > dst->size.w || src->size.h > dst->size.h) {
		re_printf("invalid size\n");
		return;
	}
#endif

	ydst0 = dst->data[0];
	ydst1 = dst->data[0] + dst->linesize[0];
	udst  = dst->data[1];
	vdst  = dst->data[2];

	/* calculate band offsets */
	bandl = (dst->size.w - src->size.w) / 2;
	bandr = dst->size.w - bandl;
	bandt = (dst->size.h - src->size.h) / 2;
	bandb = dst->size.h - bandt;

	/* 2 lines */
	for (h = 0; h < dst->size.h; h += 2) {

		int sw = 0;

		if (h < bandt || h >= bandb) {

			memset(ydst0, rgb2y(0, 0, 0), dst->linesize[0]);
			memset(ydst1, rgb2y(0, 0, 0), dst->linesize[0]);
			memset(udst,  rgb2u(0, 0, 0), dst->linesize[1]);
			memset(vdst,  rgb2v(0, 0, 0), dst->linesize[2]);
		}
		else {

			/* 2 pixels */
			for (w = 0; w < dst->size.w; w += 2) {

				if (w < bandl || w >= bandr) {

					ydst0[w]     = rgb2y(0x00, 0x00, 0x00);
					ydst0[w + 1] = rgb2y(0x00, 0x00, 0x00);
					ydst1[w]     = rgb2y(0x00, 0x00, 0x00);
					ydst1[w + 1] = rgb2y(0x00, 0x00, 0x00);
					udst[w/2]    = rgb2u(0x00, 0x00, 0x00);
					vdst[w/2]    = rgb2v(0x00, 0x00, 0x00);
				}
				else {

					ydst0[w]     = ysrc0[sw];
					ydst0[w + 1] = ysrc0[sw + 1];
					ydst1[w]     = ysrc1[sw];
					ydst1[w + 1] = ysrc1[sw + 1];
					udst[w/2]    = usrc[sw/2];
					vdst[w/2]    = vsrc[sw/2];

					sw += 2;
				}
			}

			ysrc0 += src->linesize[0] * 2;
			ysrc1 += src->linesize[0] * 2;
			usrc  += src->linesize[1];
			vsrc  += src->linesize[2];
		}

		ydst0 += dst->linesize[0] * 2;
		ydst1 += dst->linesize[0] * 2;
		udst  += dst->linesize[1];
		vdst  += dst->linesize[2];
	}
}
