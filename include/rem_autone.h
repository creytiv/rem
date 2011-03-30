/*
 * Audio Tones
 */

int autone_sine(struct mbuf *mb, uint32_t srate,
		uint32_t f1, int l1, uint32_t f2, int l2);
int autone_dtmf(struct mbuf *mb, uint32_t srate, int digit);
