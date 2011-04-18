#include <re.h>
#include <rem_vid.h>
#include <rem_dsp.h>
#include <rem_vidconv.h>
#include "vconv.h"


static inline void yuv2rgb(uint8_t *rgb, uint8_t y, int ruv, int guv, int buv)
{
	*rgb++ = saturate_u8(y + buv);
	*rgb++ = saturate_u8(y + guv);
	*rgb++ = saturate_u8(y + ruv);
	*rgb++ = 0;
}


/* native endian */
static inline void yuv2rgb565(uint8_t *rgb, uint8_t y,
			      int ruv, int guv, int buv)
{
	uint16_t *p = (uint16_t *)rgb;

	*p  = (saturate_u8(y + ruv) & 0xf8) << 8;
	*p |= (saturate_u8(y + guv) & 0xfc) << 3;
	*p |=  saturate_u8(y + buv) >> 3;
}


/**
 * Convert from Planar YUV420P to generic Packed format
 */
void vidconv_yuv420p_to_packed(struct vidconv_ctx *ctx, struct vidframe *dst,
			       const struct vidframe *src, int flags)
{
	const uint8_t *y0 = src->data[0];
	const uint8_t *y1 = src->data[0] + src->linesize[0];
	const uint8_t *u = src->data[1], *v = src->data[2];
	uint8_t *p0, *p1;
	int px, h, w;

	/* vertical flip -- read source in opposite direction */
	if (flags & VIDCONV_VFLIP) {
		p0 = (dst->data[0] + dst->linesize[0] * (dst->size.h - 1));
		p1 = (dst->data[0] + dst->linesize[0] * (dst->size.h - 2));
		px = -dst->linesize[0] * 2;
	}
	else {
		p0 = dst->data[0];
		p1 = dst->data[0] + dst->linesize[0];
		px = dst->linesize[0] * 2;
	}

	/* 2 lines */
	for (h=0; h<dst->size.h/2; h++) {

		int _u, _v;
		int ruv, guv, buv;

		int pw = 0;

		/* 2 pixels */
		for (w=0; w<dst->size.w/2; w++) {

			switch (dst->fmt) {

			case VID_FMT_ARGB:
			case VID_FMT_RGB32:

				_u = u[w];
				_v = v[w];

				ruv = ctx->CRV[_v];
				guv = ctx->CGV[_v] + ctx->CGU[_u];
				buv = ctx->CBU[_u];

				yuv2rgb(&p0[pw + 0], y0[2*w], ruv, guv, buv);
				yuv2rgb(&p0[pw + 4], y0[2*w+1], ruv, guv, buv);
				yuv2rgb(&p1[pw + 0], y1[2*w], ruv, guv, buv);
				yuv2rgb(&p1[pw + 4], y1[2*w+1], ruv, guv, buv);

				pw += 8; /* STEP */
				break;

			case VID_FMT_RGB565:

				_u = u[w];
				_v = v[w];

				ruv = ctx->CRV[_v];
				guv = ctx->CGV[_v] + ctx->CGU[_u];
				buv = ctx->CBU[_u];

				yuv2rgb565(&p0[pw+0], y0[2*w], ruv, guv, buv);
				yuv2rgb565(&p0[pw+2], y0[2*w+1],ruv, guv, buv);
				yuv2rgb565(&p1[pw+0], y1[2*w], ruv, guv, buv);
				yuv2rgb565(&p1[pw+2], y1[2*w+1],ruv, guv, buv);

				pw += 4; /* STEP */
				break;

			default:
				re_printf("no fmt %d\n", src->fmt);
				return;
			}
		}

		y0 += (src->linesize[0] * 2);
		y1 += (src->linesize[0] * 2);
		u  += src->linesize[1];
		v  += src->linesize[2];
		p0 += px;
		p1 += px;
	}
}
