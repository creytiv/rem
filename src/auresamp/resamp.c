#include <string.h>
#include <stdlib.h>
#include <re.h>
#include <rem_auresamp.h>
#include "fir.h"


struct auresamp {
	struct fir fir;
	const int16_t *coeffv;
	int coeffn;
	int channels;
	double ratio;
};


/*
 * FIR filter with cutoff 8000Hz, samplerate 16000Hz
 */
static const int16_t fir_lowpass[31] = {
   -55,      0,     96,      0,   -220,      0,    461,      0, 
  -877,      0,   1608,      0,  -3176,      0,  10342,  16410, 
 10342,      0,  -3176,      0,   1608,      0,   -877,      0, 
   461,      0,   -220,      0,     96,      0,    -55, 
};


static inline void resample(struct auresamp *ar, int16_t *dst,
			    const int16_t *src, size_t nsamp_dst)
{
	double p = 0;

	while (nsamp_dst--) {

		*dst++ = src[(int)p];
		p += 1/ar->ratio;
	}
}


int auresamp_alloc(struct auresamp **arp, int channels,
		   uint32_t srate_in, uint32_t srate_out)
{
	struct auresamp *ar;

	if (!arp || channels != 1 || !srate_in || !srate_out)
		return EINVAL;

	ar = mem_zalloc(sizeof(*ar), NULL);
	if (!ar)
		return ENOMEM;

	ar->channels = channels;
	ar->ratio = 1.0 * srate_out / srate_in;

	re_printf("allocate resampler: %uHz ---> %uHz (ratio=%f)\n",
		  srate_in, srate_out, ar->ratio);

	fir_init(&ar->fir);

	if (ar->ratio < 1) {

		ar->coeffv = fir_lowpass;
		ar->coeffn = (int)ARRAY_SIZE(fir_lowpass);
	}
	else if (ar->ratio > 1) {

		ar->coeffv = fir_lowpass;
		ar->coeffn = (int)ARRAY_SIZE(fir_lowpass);
	}

	*arp = ar;

	return 0;
}


int auresamp_process(struct auresamp *ar, struct mbuf *mb)
{
	const size_t nsamp = mbuf_get_left(mb) / 2;
	const size_t nsamp_out = nsamp * (ar ? ar->ratio : 0);
	const size_t sz = nsamp_out * sizeof(int16_t);
	size_t pos;
	int16_t *buf;

	if (!ar || !mb)
		return EINVAL;

	if (ar->ratio == 1)
		return 0;

	buf = mem_alloc(sz, NULL);
	if (!buf)
		return ENOMEM;

	pos = mb->pos;

	if (ar->ratio > 1) {

		resample(ar, buf, (void *)mbuf_buf(mb), nsamp_out);

		mb->pos = pos;

		auresamp_lowpass(ar, mb);
	}
	else {
		/* decimation: low-pass filter, then downsample */

		auresamp_lowpass(ar, mb);

		mb->pos = pos;

		resample(ar, buf, (void *)mbuf_buf(mb), nsamp_out);
	}

	/* replace buffer */
	mem_deref(mb->buf);
	mb->buf = (void *)buf;

	mb->pos = pos;
	mb->end = mb->size = sz;

	return 0;
}


int auresamp_lowpass(struct auresamp *ar, struct mbuf *mb)
{
	const int16_t *src;
	int16_t *dst;
	int nsamp;

	if (!ar || !mb)
		return EINVAL;

	nsamp = (int)(mbuf_get_left(mb) / sizeof(int16_t));
	src = dst = (void *)mbuf_buf(mb);

	while (nsamp > 0) {

		int len = min(nsamp, FIR_MAX_INPUT_LEN);

		fir_process(&ar->fir, ar->coeffv, src, dst, len, ar->coeffn);

		src   += len;
		dst   += len;
		nsamp -= len;
	}

	return 0;
}
