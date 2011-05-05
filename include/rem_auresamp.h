

/*
 * Audio resampling
 */

struct auresamp;

int auresamp_alloc(struct auresamp **arp, uint32_t srate_in,
		   uint32_t srate_out);
int auresamp_process(struct auresamp *ar, struct mbuf *dst, struct mbuf *src);
