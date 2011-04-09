/* Basic video types */

enum vidfmt {
	VID_FMT_NONE    = -1,
	VID_FMT_YUV420P =  0,
	VID_FMT_YUYV422,
	VID_FMT_UYVY422,
	VID_FMT_RGB32,
	VID_FMT_ARGB,
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
	struct vidpt origin;
	struct vidsz size;
	int r;
};

static inline bool vidsz_cmp(const struct vidsz *a, const struct vidsz *b)
{
	if (!a || !b)
		return false;

	if (a == b)
		return true;

	return a->w == b->w && a->h == b->h;
}


void vidframe_init(struct vidframe *vf, const struct vidsz *sz,
		   enum vidfmt fmt, void *data[4], int linesize[4]);
void vidframe_init_buf(struct vidframe *vf, const struct vidsz *sz,
		       enum vidfmt fmt, uint8_t *buf);
int  vidframe_alloc(struct vidframe **vfp, const struct vidsz *sz,
		    enum vidfmt fmt);
void vidframe_fill(struct vidframe *vf, uint32_t r, uint32_t g, uint32_t b);
