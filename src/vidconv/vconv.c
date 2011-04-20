#include <string.h>
#include <re.h>
#include <rem_vid.h>
#include <rem_dsp.h>
#include <rem_vidconv.h>
#include "vconv.h"


#define P 14

#define COEF_RV ((int32_t) (1.370705f * (float)(1 << P)))
#define COEF_GU ((int32_t) (-0.337633f * (float)(1 << P)))
#define COEF_GV ((int32_t) (-0.698001f * (float)(1 << P)))
#define COEF_BU ((int32_t) (1.732446f * (float)(1 << P)))


#define ERV(a) (COEF_RV * ((a) - 128))
#define EGU(a) (COEF_GU * ((a) - 128))
#define EGV(a) (COEF_GV * ((a) - 128))
#define EBU(a) (COEF_BU * ((a) - 128))


static void init_table(struct vidconv_ctx *ctx)
{
	int i;

	for (i = 0; i < 256; ++i) {
		ctx->CRV[i] = ERV(i) >> P;
		ctx->CGU[i] = EGU(i) >> P;
		ctx->CGV[i] = EGV(i) >> P;
		ctx->CBU[i] = EBU(i) >> P;
	}

	ctx->inited = true;
}


void vidconv_process(struct vidconv_ctx *ctx, struct vidframe *dst,
		     const struct vidframe *src, int rotate, int flags)
{
	if (!ctx || !dst || !src)
		return;

	if (!dst->data[0]) {
		re_printf("vidconv: invalid dst frame\n");
		return;
	}

	/* unused for now */
	if (rotate) {
		re_printf("vidconv: rotate not added yet!\n");
		return;
	}

	if (!ctx->inited) {

		init_table(ctx);

#if 1
		re_printf("vidconv: %s:%dx%d ---> %s:%dx%d"
			  " (hflip=%d vflip=%d)\n",
			  vidfmt_name(src->fmt), src->size.w, src->size.h,
			  vidfmt_name(dst->fmt), dst->size.w, dst->size.h,
			  !!(flags & VIDCONV_HFLIP),
			  !!(flags & VIDCONV_VFLIP));
#endif
		ctx->inited = true;
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

		vidconv_yuv420p_to_packed(ctx, dst, src, flags);
	}
	else if (dst->fmt == VID_FMT_YUV420P) {

		vidconv_packed_to_yuv420p(dst, src, flags);
	}
	else {

#ifdef USE_FFMPEG
		if (!ctx->sws)
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

		ysrc0 += src->linesize[0]*2;
		ysrc1 += src->linesize[0]*2;
		usrc  += src->linesize[1];
		vsrc  += src->linesize[2];
	}
}
