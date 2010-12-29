#include <re.h>
#include <rem_types.h>
#include <rem_vidconv.h>
#include <rem_orc.h>
#include <orc/orc.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>


#if LIBSWSCALE_VERSION_MINOR >= 9
#define SRCSLICE_CAST (const uint8_t **)
#else
#define SRCSLICE_CAST (uint8_t **)
#endif


int rem_init(void)
{
	orc_init();

	orc_debug_set_level(ORC_DEBUG_INFO);

	vidconv_init();

	return 0;
}


/**
 * Convert from packed YUYV422 to planar YUV420P
 */
void vidconv_yuyv_to_yuv420p(struct vidframe *dst, const struct vidframe *src)
{
	const uint32_t *p1 = (uint32_t *)src->data[0];
	const uint32_t *p2 = (uint32_t *)(src->data[0] + src->linesize[0]);
	uint16_t *y  = (uint16_t *)dst->data[0];
	uint16_t *y2 = (uint16_t *)(dst->data[0] + dst->linesize[0]);
	uint8_t *u = dst->data[1], *v = dst->data[2];
	int h, w;

	/* todo: fix endianness assumptions */

	/* 2 lines */
	for (h=0; h<dst->size.h/2; h++) {

		for (w=0; w<dst->size.w/2; w++) {

			/* Y1 */
			y[w]   =   p1[w] & 0xff;
			y[w]  |= ((p1[w] >> 16) & 0xff) << 8;

			/* Y2 */
			y2[w]   =   p2[w] & 0xff;
			y2[w]  |= ((p2[w] >> 16) & 0xff) << 8;

			/* U and V */
			u[w] = (p1[w] >>  8) & 0xff;
			v[w] = (p1[w] >> 24) & 0xff;
		}

		p1 += src->linesize[0] / 2;
		p2 += src->linesize[0] / 2;

		y  += dst->linesize[0];
		y2 += dst->linesize[0];
		u  += dst->linesize[1];
		v  += dst->linesize[2];
	}
}


/* Convenience wrapper */
void vidconv_yuyv_to_yuv420p_orc(struct vidframe *dst,
				 const struct vidframe *src)
{
	/* stride is in "bytes" */

	yuyv422_to_yuv420p((uint16_t *)dst->data[0],
			   dst->linesize[0]*2,
			   (uint16_t *)(dst->data[0] + dst->linesize[0]),
			   dst->linesize[0]*2,
			   dst->data[1], dst->linesize[1],
			   dst->data[2], dst->linesize[2],
			   (uint32_t *)src->data[0],
			   src->linesize[0]*2,
			   (uint32_t *)(src->data[0] + src->linesize[0]),
			   src->linesize[0]*2,
			   src->size.w / 2,
			   src->size.h / 2);
}


void vidconv_yuyv_to_yuv420p_sws(struct vidframe *dst,
				 const struct vidframe *src)
{
	static struct SwsContext *sws = NULL;
	AVPicture avdst, avsrc;
	int i;

	if (!sws) {
		sws = sws_getContext(src->size.w, src->size.h,
				     PIX_FMT_YUYV422,
				     dst->size.w, dst->size.h,
				     PIX_FMT_YUV420P,
				     SWS_BICUBIC, NULL, NULL, NULL);
	}

	for (i=0; i<4; i++) {
		avsrc.data[i]     = src->data[i];
		avsrc.linesize[i] = src->linesize[i];
		avdst.data[i]     = dst->data[i];
		avdst.linesize[i] = dst->linesize[i];
	}

	sws_scale(sws, SRCSLICE_CAST avsrc.data, avsrc.linesize, 0,
		  src->size.h, avdst.data, avdst.linesize);
}
