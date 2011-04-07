/**
 * @file vutil.c  Video utility functions
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <rem_types.h>


/**
 * Calculate the picture size with YUV420P pixel format
 */
static uint32_t yuv420p_size(const struct vidsz *sz)
{
	return sz ? (sz->w * sz->h * 3 / 2) : 0;
}


void vidframe_init(struct vidframe *vf, const struct vidsz *sz,
		   uint8_t *data[4], int linesize[4])
{
	int i;

	if (!vf || !sz || !data || !linesize)
		return;

	for (i=0; i<4; i++) {
		vf->data[i]     = data[i];
		vf->linesize[i] = linesize[i];
	}

	vf->size = *sz;
	vf->valid = true;
}


void vidframe_yuv420p_init(struct vidframe *vf, uint8_t *buf,
			   const struct vidsz *sz)
{
	int w, h;

	if (!vf || !buf || !sz)
		return;

	w = (sz->w + 1) >> 1;
	vf->linesize[0] = sz->w;
	vf->linesize[1] = w;
	vf->linesize[2] = w;
	vf->linesize[3] = 0;

	h = (sz->h + 1) >> 1;
	vf->data[0] = buf;
	vf->data[1] = vf->data[0] + vf->linesize[0] * sz->h;
	vf->data[2] = vf->data[1] + vf->linesize[1] * h;
	vf->data[3] = NULL;

	vf->size = *sz;
	vf->valid = true;
}


void vidframe_rgb32_init(struct vidframe *vf, const struct vidsz *sz,
			 uint8_t *buf)
{
	if (!vf || !buf || !sz)
		return;

	memset(vf, 0, sizeof(*vf));

	vf->data[0] = buf;
	vf->linesize[0] = sz->w * 4; /* RGB32 */
	vf->size = *sz;
	vf->valid = true;
}


struct vidframe *vidframe_alloc(const struct vidsz *sz)
{
	struct vidframe *vf;

	if (!sz || !sz->w || !sz->h)
		return NULL;

	vf = mem_zalloc(sizeof(*vf) + yuv420p_size(sz), NULL);
	if (!vf)
		return NULL;

	vidframe_yuv420p_init(vf, (uint8_t *) (vf + 1), sz);

	return vf;
}


int vidframe_print(struct re_printf *pf, const struct vidframe *vf)
{
	if (!vf)
		return 0;

	if (!vf->valid)
		return re_hprintf(pf, "INVALID VIDFRAME");

	return re_hprintf(pf, "%ux%u linesize=%u:%u:%u:%u data=%p:%p:%p:%p",
			  vf->size.w, vf->size.h,
			  vf->linesize[0], vf->linesize[1],
			  vf->linesize[2], vf->linesize[3],
			  vf->data[0], vf->data[1], vf->data[2], vf->data[3]);
}


int vidframe_alloc_filled(struct vidframe **vfp, const struct vidsz *sz,
                          uint32_t r, uint32_t g, uint32_t b)
{
#define PREC 8
#define RGB2Y(r, g, b) (((66 * (r) + 129 * (g) + 25 * (b)) >> PREC) + 16)
#define RGB2U(r, g, b) (((-37 * (r) + -73 * (g) + 112 * (b)) >> PREC) + 128)
#define RGB2V(r, g, b) (((112 * (r) + -93 * (g) + -17 * (b)) >> PREC) + 128)

	struct vidframe *vf;

	if (!sz)
		return EINVAL;

	vf = vidframe_alloc(sz);
	if (!vf)
		return ENOMEM;

	/* Y */
	memset(vf->data[0], RGB2Y(r, g, b), vf->size.h * vf->linesize[0]);
	/* U */
	memset(vf->data[1], RGB2U(r, g, b),(vf->size.h/2) * vf->linesize[1]);
	/* V */
	memset(vf->data[2], RGB2V(r, g, b), (vf->size.h/2) * vf->linesize[2]);

	vf->valid = true;

	*vfp = vf;

	return 0;
}


void vidrect_init(struct vidrect *rect, int x, int y, int w, int h, int r)
{
	if (!rect)
		return;

	rect->origin.x = x;
	rect->origin.y = y;
	rect->size.w = w;
	rect->size.h = h;
	rect->r = r;
}


/*
 * Copy a frame into another frame, with offset positions
 *
 * NOTE: make sure that the offset position does not exceed the size!!!
 */
void vidframe_copy_offset(struct vidframe *fd, const struct vidframe *fs,
			  int x, int y)
{
	uint8_t *y0, *y1, *up, *vp;
	uint8_t *ups, *vps;
	int h;

	if (!fd || !fs)
		return;

	y0 = fd->data[0] + x + y * fd->linesize[0];
	y1 = y0 + fd->linesize[0];
	up = fd->data[1] + x/2 + y/2 * fd->linesize[1];
	vp = fd->data[2] + x/2 + y/2 * fd->linesize[2];

	ups = fs->data[1];
	vps = fs->data[2];

	for (h=0; h<fs->size.h; h+=2) {

		/* First line */
		memcpy(y0, fs->linesize[0] * h + fs->data[0], fs->linesize[0]);

		/* Second line */
		memcpy(y1, fs->linesize[0] * (h+1) + fs->data[0],
		       fs->linesize[0]);

		/* U */
		memcpy(up, ups, fs->linesize[1]);

		/* V */
		memcpy(vp, vps, fs->linesize[2]);

		y0 += fd->linesize[0]*2;
		y1 += fd->linesize[0]*2;
		up += fd->linesize[1];
		vp += fd->linesize[2];

		ups += fs->linesize[1];
		vps += fs->linesize[2];
	}
}


void vidframe_alphablend_area(const struct vidframe *fd, int x, int y,
			      int width, int height, double alpha)
{
	uint8_t *y0;
	int w, h;

	if (!fd)
		return;

	y0 = fd->data[0] + x + y * fd->linesize[0];

	for (h=0; h<height; h++) {

		for (w=0; w<width; w++) {

			y0[w] *= alpha;
		}

		y0 += fd->linesize[0];
	}
}
