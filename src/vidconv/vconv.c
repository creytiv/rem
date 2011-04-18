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
