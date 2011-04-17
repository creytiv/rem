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


static inline void yuv2rgb(uint8_t *rgb, uint8_t y, int ruv, int guv, int buv)
{
	*rgb++ = saturate_u8(y + buv);
	*rgb++ = saturate_u8(y + guv);
	*rgb++ = saturate_u8(y + ruv);
	*rgb++ = 0;
}


static void yuv420p_to_rgb32(struct vidconv_ctx *ctx, struct vidframe *dst,
			     const struct vidframe *src, bool vflip)
{
	const uint16_t *y  = (uint16_t *)src->data[0];
	const uint16_t *y2 = (uint16_t *)(src->data[0] + src->linesize[0]);
	const uint8_t *u = src->data[1], *v = src->data[2];
	uint8_t *p, *p1, *p2;
	int pinc;
	int h, w, j;

	p = dst->data[0];

	if (vflip)
		pinc = -dst->linesize[0] * 2;
	else
		pinc = dst->linesize[0] * 2;

	/* vertical flip -- read source in opposite direction */
	if (vflip) {
		p1 = (p + dst->linesize[0] * (dst->size.h - 1));
		p2 = (p + dst->linesize[0] * (dst->size.h - 2));
	}
	else {
		p1 = p;
		p2 = p + dst->linesize[0];
	}

	/* 2 lines */
	for (h = 0; h < dst->size.h/2; h++) {

		int _u, _v;
		int ruv, guv, buv;

		/* 2 pixels */
		for (w = 0; w < dst->size.w/2; w++) {

			j = w * 8;

			_u = u[w];
			_v = v[w];

			ruv = ctx->CRV[_v];
			guv = ctx->CGV[_v] + ctx->CGU[_u];
			buv = ctx->CBU[_u];

			yuv2rgb(&p1[j + 0], y[w] >> 8, ruv, guv, buv);
			yuv2rgb(&p1[j + 4], y[w] & 0xff, ruv, guv, buv);
			yuv2rgb(&p2[j + 0], y2[w] >> 8, ruv, guv, buv);
			yuv2rgb(&p2[j + 4], y2[w] & 0xff, ruv, guv, buv);
		}

		y  += src->linesize[0];
		y2 += src->linesize[0];
		u  += src->linesize[1];
		v  += src->linesize[2];

		p1 += pinc;
		p2 += pinc;
	}
}


void vidconv_process(struct vidconv_ctx *ctx, struct vidframe *dst,
		     const struct vidframe *src, int rotate, int flags)
{
	bool hflip, vflip;

	if (!ctx || !dst || !src)
		return;

	/* unused for now */
	if (rotate) {
		re_printf("vidconv: rotate not added yet!\n");
		return;
	}

	hflip = !!(flags & VIDCONV_HFLIP);
	vflip = !!(flags & VIDCONV_VFLIP);

	if (!ctx->inited) {

		init_table(ctx);

#if 1
		re_printf("vidconv: %s:%dx%d ---> %s:%dx%d"
			  " (hflip=%d vflip=%d)\n",
			  vidfmt_name(src->fmt), src->size.w, src->size.h,
			  vidfmt_name(dst->fmt), dst->size.w, dst->size.h,
			  hflip, vflip);
#endif
		ctx->inited = true;
	}

	if (src->fmt == dst->fmt) {

		int i;

		for (i=0; i<4; i++) {
			dst->data[i]     = src->data[i];
			dst->linesize[i] = src->linesize[i];
		}
	}
	else if (src->fmt == VID_FMT_YUV420P && dst->fmt == VID_FMT_RGB32) {

		yuv420p_to_rgb32(ctx, dst, src, vflip);
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
