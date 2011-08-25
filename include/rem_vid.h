/**
 * @file rem_vid.h Basic video types
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** Pixel format */
enum vidfmt {
	VID_FMT_YUV420P =  0, /* planar YUV  4:2:0   12bpp                 */
	VID_FMT_YUYV422,      /* packed YUV  4:2:2   16bpp                 */
	VID_FMT_UYVY422,      /* packed YUV  4:2:2   16bpp                 */
	VID_FMT_RGB32,        /* packed RGBA 8:8:8:8 32bpp (native endian) */
	VID_FMT_ARGB,         /* packed RGBA 8:8:8:8 32bpp (big endian)    */
	VID_FMT_RGB565,       /* packed RGB  5:6:5   16bpp (native endian) */
	VID_FMT_RGB555,       /* packed RGB  5:5:5   16bpp (native endian) */
	VID_FMT_NV12,         /* planar YUV  4:2:0   12bpp UV interleaved  */
	/* marker */
	VID_FMT_N
};

struct vidfmt_compdesc {
	unsigned plane_index:2;
	unsigned step:3;
};

struct vidfmt_desc {
	const char *name;
	uint8_t planes;
	uint8_t compn;
	struct vidfmt_compdesc compv[4];
};

enum vidorient {
	VIDORIENT_PORTRAIT,
	VIDORIENT_PORTRAIT_UPSIDEDOWN,
	VIDORIENT_LANDSCAPE_LEFT,
	VIDORIENT_LANDSCAPE_RIGHT,
};

struct vidsz {
	int w, h;
};

struct vidframe {
	uint8_t *data[4];
	uint16_t linesize[4];
	struct vidsz size;
	enum vidfmt fmt;
};

struct vidpt {
	int x;
	int y;
};

struct vidrect {
	int x, y, w, h;
};

static inline bool vidsz_cmp(const struct vidsz *a, const struct vidsz *b)
{
	if (!a || !b)
		return false;

	if (a == b)
		return true;

	return a->w == b->w && a->h == b->h;
}


static inline bool vidrect_cmp(const struct vidrect *a,
			       const struct vidrect *b)
{
	if (!a || !b)
		return false;

	if (a == b)
		return true;

	return a->x == b->x && a->y == b->y && a->w == b->w && a->h == b->h;
}


static inline int rgb2y(uint8_t r, uint8_t g, uint8_t b)
{
	return ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
}


static inline int rgb2u(uint8_t r, uint8_t g, uint8_t b)
{
	return ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
}


static inline int rgb2v(uint8_t r, uint8_t g, uint8_t b)
{
	return ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
}


void vidframe_init(struct vidframe *vf, enum vidfmt fmt,
		   const struct vidsz *sz, void *data[4], int linesize[4]);
void vidframe_init_buf(struct vidframe *vf, enum vidfmt fmt,
		       const struct vidsz *sz, uint8_t *buf);
int  vidframe_alloc(struct vidframe **vfp, enum vidfmt fmt,
		    const struct vidsz *sz);
void vidframe_fill(struct vidframe *vf, uint32_t r, uint32_t g, uint32_t b);
size_t vidframe_size(const struct vidframe *vf);


const char *vidfmt_name(enum vidfmt fmt);


static inline bool vidframe_isvalid(const struct vidframe *f)
{
	return f ? f->data[0] != NULL : false;
}


extern const struct vidfmt_desc vidfmt_descv[VID_FMT_N];
