

/*
 * Video Mixer
 */

struct vidmix;
struct vidmix_source;

typedef void (vidmix_frame_h)(uint32_t ts, const struct vidframe *frame,
			      void *arg);

int  vidmix_alloc(struct vidmix **vmp, const struct vidsz *sz, int fps);
int  vidmix_source_add(struct vidmix_source **srcp, struct vidmix *vm,
		       vidmix_frame_h *fh, void *arg);
void vidmix_source_put(struct vidmix_source *src,
		       const struct vidframe *frame);
void vidmix_source_focus(struct vidmix_source *src, unsigned fidx);
