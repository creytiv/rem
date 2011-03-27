/*
 * Audio Buffer
 */

struct aubuf;

int  aubuf_alloc(struct aubuf **abp, size_t min_sz, size_t max_sz);
int  aubuf_append(struct aubuf *ab, struct mbuf *mb);
int  aubuf_write(struct aubuf *ab, const uint8_t *p, size_t sz);
void aubuf_read(struct aubuf *ab, uint8_t *p, size_t sz);
int  aubuf_get(struct aubuf *ab, uint32_t ptime, uint8_t *p, size_t sz);
int  aubuf_debug(struct re_printf *pf, const struct aubuf *ab);
size_t aubuf_cur_size(const struct aubuf *ab);


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
