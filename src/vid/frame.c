#include <string.h>
#include <re.h>
#include <rem_vid.h>


static uint32_t vidsize(const struct vidsz *sz, enum vidfmt fmt)
{
	if (!sz)
		return 0;

	switch (fmt) {

	case VID_FMT_YUV420P: return sz->w * sz->h * 3 / 2;
	case VID_FMT_RGB32:   return sz->w * sz->h * 4;
	default:              return 0;
	}
}


void vidframe_init(struct vidframe *vf, const struct vidsz *sz,
		   enum vidfmt fmt, void *data[4], int linesize[4])
{
	int i;

	if (!vf || !sz || !data || !linesize)
		return;

	for (i=0; i<4; i++) {
		vf->data[i]     = data[i];
		vf->linesize[i] = linesize[i];
	}

	vf->size = *sz;
	vf->fmt = fmt;
}


void vidframe_init_buf(struct vidframe *vf, const struct vidsz *sz,
		       enum vidfmt fmt, uint8_t *buf)
{
	int w, h;

	if (!vf || !sz || !buf)
		return;

	memset(vf->linesize, 0, sizeof(vf->linesize));
	memset(vf->data, 0, sizeof(vf->data));

	switch (fmt) {

	case VID_FMT_YUV420P:
		w = (sz->w + 1) >> 1;
		vf->linesize[0] = sz->w;
		vf->linesize[1] = w;
		vf->linesize[2] = w;

		h = (sz->h + 1) >> 1;
		vf->data[0] = buf;
		vf->data[1] = vf->data[0] + vf->linesize[0] * sz->h;
		vf->data[2] = vf->data[1] + vf->linesize[1] * h;
		break;

	case VID_FMT_RGB32:
		vf->linesize[0] = sz->w * 4;
		vf->data[0] = buf;
		break;

	default:
		return;
	}

	vf->size = *sz;
	vf->fmt = fmt;
}


int vidframe_alloc(struct vidframe **vfp, const struct vidsz *sz,
		   enum vidfmt fmt)
{
	struct vidframe *vf;

	if (!sz || !sz->w || !sz->h)
		return EINVAL;

	vf = mem_zalloc(sizeof(*vf) + vidsize(sz, fmt), NULL);
	if (!vf)
		return ENOMEM;

	vidframe_init_buf(vf, sz, fmt, (uint8_t *)(vf + 1));

	*vfp = vf;

	return 0;
}


void vidframe_fill(struct vidframe *vf, uint32_t r, uint32_t g, uint32_t b)
{
	uint8_t *p;
	int h, i;

	if (!vf)
		return;

	switch (vf->fmt) {

	case VID_FMT_YUV420P:
		h = vf->size.h;

		memset(vf->data[0], rgb2y(r, g, b), h * vf->linesize[0]);
		memset(vf->data[1], rgb2u(r, g, b), h/2 * vf->linesize[1]);
		memset(vf->data[2], rgb2v(r, g, b), h/2 * vf->linesize[2]);
		break;

	case VID_FMT_RGB32:
		p = vf->data[0];
		for (i=0; i<vf->linesize[0] * vf->size.h; i+=4) {
			*p++ = r;
			*p++ = g;
			*p++ = b;
			*p++ = 0;
		}
		break;

	default:
		break;
	}
}


const char *vidfmt_name(enum vidfmt fmt)
{
	switch (fmt) {

	case VID_FMT_NONE:    return "(NONE)";
	case VID_FMT_YUV420P: return "YUV420P";
	case VID_FMT_UYVY422: return "UYVY422";
	case VID_FMT_YUYV422: return "YUYV422";
	case VID_FMT_RGB32:   return "RGB32";
	case VID_FMT_ARGB:    return "ARGB";
	default:              return "???";
	}
}
