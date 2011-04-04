

/*
 * Audio resampling
 */

struct auresamp;

int auresamp_alloc(struct auresamp **arp, int channels, double ratio);
int auresamp_process(struct auresamp *ar, struct mbuf *mb);


/// XXX temp
int auresamp_lowpass(struct auresamp *ar, struct mbuf *mb);
