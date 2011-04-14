

/*
 * Audio resampling
 */

struct auresamp;

int auresamp_alloc(struct auresamp **arp, int channels,
		   uint32_t srate_in, uint32_t srate_out);
int auresamp_process(struct auresamp *ar, struct mbuf *dst, struct mbuf *src);


/// XXX temp
int auresamp_lowpass(struct auresamp *ar, int16_t *buf, size_t nsamp);
