

/*
 * Audio Mixer
 */

struct aumix;
struct aumix_source;

typedef void (aumix_frame_h)(uint32_t ts, const uint8_t *buf, size_t sz,
			     void *arg);

int aumix_alloc(struct aumix **mixp, uint32_t srate, int ch, uint32_t ptime);
int aumix_source_add(struct aumix_source **srcp, struct aumix *mix,
		     aumix_frame_h *fh, void *arg);
int aumix_source_put(struct aumix_source *src, struct mbuf *mb);
