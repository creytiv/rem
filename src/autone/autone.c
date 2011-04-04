/**
 * @file autone.c  Audio Tones
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <math.h>
#include <re.h>
#include <rem_autone.h>
#include <rem_dsp.h>


#define SCALE (32767)
#define DTMF_AMP  (5)

#if !defined (M_PI)
#define M_PI 3.14159265358979323846264338327
#endif


static inline uint32_t digit2lo(int digit)
{
	switch (digit) {

	case '1': case '2': case '3': case 'A': return 697;
	case '4': case '5': case '6': case 'B': return 770;
	case '7': case '8': case '9': case 'C': return 852;
	case '*': case '0': case '#': case 'D': return 941;
	default: return 0;
	}
}


static inline uint32_t digit2hi(int digit)
{
	switch (digit) {

	case '1': case '4': case '7': case '*': return 1209;
	case '2': case '5': case '8': case '0': return 1336;
	case '3': case '6': case '9': case '#': return 1477;
	case 'A': case 'B': case 'C': case 'D': return 1633;
	default: return 0;
	}
}


int autone_sine(struct mbuf *mb, uint32_t srate,
		uint32_t f1, int l1, uint32_t f2, int l2)
{
	double d1, d2;
	uint32_t i;
	int err = 0;

	if (!mb || !srate)
		return EINVAL;

	d1 = 1.0f * f1 / srate;
	d2 = 1.0f * f2 / srate;

	for (i=0; i<srate; i++) {
		int16_t s1 = SCALE * l1 / 100.0f * sin(2 * M_PI * d1 * i);
		int16_t s2 = SCALE * l2 / 100.0f * sin(2 * M_PI * d2 * i);

		err |= mbuf_write_u16(mb, sadd16(s1, s2));
	}

	return err;
}


int autone_dtmf(struct mbuf *mb, uint32_t srate, int digit)
{
	return autone_sine(mb, srate,
			   digit2lo(digit), DTMF_AMP,
			   digit2hi(digit), DTMF_AMP);
}
