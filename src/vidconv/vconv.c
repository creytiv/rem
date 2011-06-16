#include <string.h>
#include <re.h>
#include <rem_vid.h>
#include <rem_dsp.h>
#include <rem_vidconv.h>
#include "vconv.h"


#if 0
void vidconv_process(struct vidframe *dst, const struct vidframe *src,
		     int flags)
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
		re_printf("vidconv: unsupported %s -> %s\n",
			  vidfmt_name(src->fmt), vidfmt_name(dst->fmt));
	}
}
#endif


/*
 *
 * Speed matches swscale: SWS_BILINEAR
 *
 * todo: optimize (check out SWS_FAST_BILINEAR)
 *
 */

void vidconv_process(struct vidframe *dst, const struct vidframe *src,
		     struct vidrect *r)
{
	struct vidrect rdst;
	unsigned x, y, xd, yd, xs, ys, xs2, ys2, lsd, lss;
	unsigned id, is;
	double rw, rh;

	if (!dst || !src)
		return;

#if 1
	if (dst->fmt != VID_FMT_YUV420P)
		return;
#endif

	if (r) {
		if ((int)(r->w - r->x) > dst->size.w ||
		    (int)(r->h - r->y) > dst->size.h) {
			re_printf("rect out of bounds (%u x %u)\n",
				  dst->size.w, dst->size.h);
			return;
		}
	}
	else {
		rdst.x = rdst.y = 0;
		rdst.w = dst->size.w;
		rdst.h = dst->size.h;
		r = &rdst;
	}

	rw = (double)src->size.w / (double)r->w;
	rh = (double)src->size.h / (double)r->h;

	lsd = dst->linesize[0];
	lss = src->linesize[0];

	/* align 2 pixels */
	r->x &= ~1;
	r->y &= ~1;

	for (y=0; y<r->h; y+=2) {

		yd = y + r->y;

		for (x=0; x<r->w; x+=2) {

			xd  = x + r->x;

			xs  = (unsigned)(x * rw);
			xs2 = (unsigned)((x+1) * rw);
			ys  = (unsigned)(y * rh);
			ys2 = (unsigned)((y+1) * rh);

			id = xd + yd*lsd;

			switch (src->fmt) {

			case VID_FMT_YUV420P:
				dst->data[0][id]         = src->data[0][xs  + ys*lss];
				dst->data[0][id+1]       = src->data[0][xs2 + ys*lss];
				dst->data[0][id + lsd]   = src->data[0][xs  + ys2*lss];
				dst->data[0][id+1 + lsd] = src->data[0][xs2 + ys2*lss];
				break;

			case VID_FMT_YUYV422:
				dst->data[0][id]         = src->data[0][2*xs  + ys*lss];
				dst->data[0][id+1]       = src->data[0][2*xs  + ys*lss + 2];
				dst->data[0][id + lsd]   = src->data[0][2*xs  + ys2*lss];
				dst->data[0][id+1 + lsd] = src->data[0][2*xs  + ys2*lss + 2];
				break;

			default:
				re_printf("no src fmt %d\n", src->fmt);
				break;
			}

			/* align 2 pixels */
			xs &= ~1;
			ys &= ~1;

			id = xd/2 + yd*lsd/4;
			is = xs/2 + ys*lss/4;

			switch (src->fmt) {

			case VID_FMT_YUV420P:
				dst->data[1][id] = src->data[1][is];
				dst->data[2][id] = src->data[2][is];
				break;

			case VID_FMT_YUYV422:
				dst->data[1][id] = src->data[0][2*xs  + ys*lss + 1];
				dst->data[2][id] = src->data[0][2*xs  + ys*lss + 3];
				break;

			default:
				break;
			}
		}
	}
}
