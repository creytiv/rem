struct vidsz {
	int w, h;
};

struct vidframe {
	uint8_t *data[4];
	uint16_t linesize[4];
	struct vidsz size;
	bool     valid;
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


void vidframe_init(struct vidframe *vf, const struct vidsz *sz,
		   uint8_t *data[4], int linesize[4]);
void vidframe_yuv420p_init(struct vidframe *vf, uint8_t *ptr,
			   const struct vidsz *sz);
void vidframe_rgb32_init(struct vidframe *vf, const struct vidsz *sz,
			 uint8_t *buf);
struct vidframe *vidframe_alloc(const struct vidsz *sz);
int vidframe_alloc_filled(struct vidframe **vfp, const struct vidsz *sz,
                          uint32_t r, uint32_t g, uint32_t b);
int  vidframe_print(struct re_printf *pf, const struct vidframe *vf);
void vidrect_init(struct vidrect *rect, int x, int y, int w, int h, int r);
void vidframe_copy_offset(struct vidframe *fd, const struct vidframe *fs,
			  int x, int y);
void vidframe_alphablend_area(const struct vidframe *fd, int x, int y,
			      int width, int height, double alpha);
