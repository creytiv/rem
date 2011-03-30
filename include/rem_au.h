

/*
 * Audio Tones
 */

int autone_sine(struct mbuf *mb, uint32_t srate,
		uint32_t f1, int l1, uint32_t f2, int l2);
int autone_dtmf(struct mbuf *mb, uint32_t srate, int digit);


/*
 * Audio Mixer
 */

struct aumix;
struct aumix_source;

typedef void (aumix_frame_h)(uint32_t ts, const uint8_t *buf, size_t sz,
			     void *arg);

int aumix_alloc(struct aumix **mixp);
int aumix_source_add(struct aumix_source **srcp, struct aumix *mix,
		     aumix_frame_h *fh, void *arg);
int aumix_source_put(struct aumix_source *src, struct mbuf *mb);


/*
 * Audio resampling
 */

struct auresamp;

int auresamp_alloc(struct auresamp **arp, int channels, double ratio);
int auresamp_process(struct auresamp *ar, struct mbuf *mb);
