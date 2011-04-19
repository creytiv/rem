#include <re.h>
#include <rem_vid.h>
#include <rem_vidconv.h>
#ifdef USE_FFMPEG
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#endif
#include "vconv.h"


#if LIBSWSCALE_VERSION_MINOR >= 9
#define SRCSLICE_CAST (const uint8_t **)
#else
#define SRCSLICE_CAST (uint8_t **)
#endif


static inline enum PixelFormat vidfmt2ffmpeg(enum vidfmt fmt)
{
	switch (fmt) {

	case VID_FMT_NONE:     return PIX_FMT_NONE;
	case VID_FMT_YUV420P:  return PIX_FMT_YUV420P;
	case VID_FMT_YUYV422:  return PIX_FMT_YUYV422;
	case VID_FMT_UYVY422:  return PIX_FMT_UYVY422;
	case VID_FMT_RGB32:    return PIX_FMT_RGB32;
	case VID_FMT_RGB565:   return PIX_FMT_RGB565LE;  // XXX

	default:
		re_printf("sws: no fmt %s\n", vidfmt_name(fmt));
		return PIX_FMT_NONE;
	}
}


void vidconv_sws(struct vidconv_ctx *ctx, struct vidframe *dst,
		 const struct vidframe *src)
{
	AVPicture avdst, avsrc;
	int i;

	if (!ctx->sws) {
		ctx->sws = sws_getContext(src->size.w, src->size.h,
					  vidfmt2ffmpeg(src->fmt),
					  dst->size.w, dst->size.h,
					  vidfmt2ffmpeg(dst->fmt),
					  SWS_BICUBIC, NULL, NULL, NULL);
		if (!ctx->sws)
			return;
	}

	for (i=0; i<4; i++) {
		avsrc.data[i]     = src->data[i];
		avsrc.linesize[i] = src->linesize[i];
		avdst.data[i]     = dst->data[i];
		avdst.linesize[i] = dst->linesize[i];
	}

	sws_scale(ctx->sws, SRCSLICE_CAST avsrc.data, avsrc.linesize, 0,
		  src->size.h, avdst.data, avdst.linesize);
}
