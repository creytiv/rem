#include <re.h>
#include <rem_types.h>
#include <rem_vidconv.h>
#include <rem_orc.h>
#include <orc/orc.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>


int rem_init(void)
{
	orc_init();

	orc_debug_set_level(ORC_DEBUG_INFO);

	vidconv_init();

	return 0;
}


/**
 * Convert from YUYV to YUV420P
 */
void vidconv_yuyv_to_yuv420p(struct vidframe *dst, const struct vidframe *src)
{
	const uint8_t *p = src->data[0];
	uint8_t *y = dst->data[0], *u = dst->data[1], *v = dst->data[2];
	int h, w, l;

	for (h=0; h<src->size.h; h++) {

		/* Y */
		for (w=0; w<src->size.w; w++) {
			y[w] = p[2*w];
		}

		if (!(h & 1)) {

			/* U */
			for (l=0; l<dst->linesize[1]; l++) {
				u[l] = p[4*l + 1];
			}

			/* V */
			for (l=0; l<dst->linesize[2]; l++) {
				v[l] = p[4*l + 3];
			}

			u += dst->linesize[1];
			v += dst->linesize[2];
		}

		p += src->linesize[0];
		y += dst->linesize[0];
	}
}


/**
 * Convert from YUYV to YUV420P
 */
void vidconv_yuyv_to_yuv420p_b(uint16_t *y1, int y1_linesize,
			       uint16_t *y2, int y2_linesize,
			       uint8_t *u, int u_linesize,
			       uint8_t *v, int v_linesize,
			       const uint32_t *p1, int p1_linesize,
			       const uint32_t *p2, int p2_linesize,
			       int hwidth, int hheight)
{
	int h, w;

	// every 2nd line
	for (h=0; h<hheight; h++) {

		for (w=0; w<hwidth; w++) {

			/* Y1 */
			y1[w]   = (p1[w] >>  0) & 0xff;
			y1[w]  |= ((p1[w] >> 16) & 0xff) << 8;

			/* Y2 */
			y2[w]   = (p2[w] >>  0) & 0xff;
			y2[w]  |= ((p2[w] >> 16) & 0xff) << 8;

			/* U and V */
			u[w] = (p1[w] >>  8) & 0xff;
			v[w] = (p1[w] >> 24) & 0xff;
		}

		p1 += p1_linesize * 2; // jump 2 lines
		p2 += p2_linesize * 2;
		y1 += y1_linesize * 2;
		y2 += y2_linesize * 2;
		u  += u_linesize;
		v  += v_linesize;
	}
}


// Convenience wrapper
void vidconv_yuyv_to_yuv420p_orc(struct vidframe *dst,
				 const struct vidframe *src)
{
	// stride is in "bytes"

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

	sws_scale(sws, avsrc.data, avsrc.linesize, 0, src->size.h,
		  avdst.data, avdst.linesize);
}
