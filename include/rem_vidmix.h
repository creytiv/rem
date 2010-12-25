struct vidmix;
struct vidmix_source;

/** Called for each update of the "Main" frame */
typedef void (vidmix_frame_h)(const struct vidframe *frame, void *arg);

int vidmix_alloc(struct vidmix **vmp, const struct vidsz *sz, int maxfps,
		 vidmix_frame_h *fh, void *arg);
int vidmix_source_add(struct vidmix_source **srcp, struct vidmix *vm);
int vidmix_source_put(struct vidmix_source *src, const struct vidframe *frame);
