#include <string.h>
#include <re.h>
#include <rem_vid.h>
#include <rem_dsp.h>
#include <rem_vidconv.h>
#include "vconv.h"


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
