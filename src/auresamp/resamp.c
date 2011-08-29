/**
 * @file resamp.c Audio Resampler
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <stdlib.h>
#include <re.h>
#include <rem_fir.h>
#include <rem_auresamp.h>


typedef void (resample_h)(struct auresamp *ar, int16_t *dst,
			  const int16_t *src, size_t nsamp_dst);


struct auresamp {
	struct fir fir;
	const int16_t *coeffv;
	int coeffn;
	double ratio;
	uint8_t ch_in;
	uint8_t ch_out;
	resample_h *resample;
};


/*
 * FIR filter with cutoff 4000Hz, samplerate 48000Hz
 */
static const int16_t fir_lowpass_48_4[31] = {
	-42,    -35,    -26,      0,     61,    173,    349,    595,
	907,   1271,   1663,   2052,   2403,   2683,   2864,   2927,
       2864,   2683,   2403,   2052,   1663,   1271,    907,    595,
	349,    173,     61,      0,    -26,    -35,    -42,
};


/*
 * FIR filter with cutoff 8000Hz, samplerate 48000Hz
 */
static const int16_t fir_lowpass_48_8[31] = {
	55,     57,     47,      0,   -109,   -279,   -459,   -553,
      -436,      0,    800,   1908,   3161,   4323,   5146,   5443,
      5146,   4323,   3161,   1908,    800,      0,   -436,   -553,
      -459,   -279,   -109,      0,     47,     57,     55,
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


static inline void resample_mono2stereo(struct auresamp *ar, int16_t *dst,
					const int16_t *src, size_t nsamp_dst)
{
	double p = 0;

	while (nsamp_dst--) {

		const int i = (int)p;

		*dst++ = src[i];  /* Left channel */
		*dst++ = src[i];  /* Right channel */

		p += 1/ar->ratio;
	}
}


static inline void resample_stereo2mono(struct auresamp *ar, int16_t *dst,
					const int16_t *src, size_t nsamp_dst)
{
	double p = 0;

	while (nsamp_dst--) {

		const int i = ((int)p) & ~1;

		*dst++ = (src[i] + src[i+1]) / 2;

		p += 1/ar->ratio * 2;
	}
}


static inline void resample_stereo(struct auresamp *ar, int16_t *dst,
				   const int16_t *src, size_t nsamp_dst)
{
	double p = 0;

	while (nsamp_dst--) {

		const int i = ((int)p) & ~1;

		*dst++ = src[i];    /* Left channel */
		*dst++ = src[i+1];  /* Right channel */

		p += 1/ar->ratio * 2;
	}
}


static void auresamp_lowpass(struct auresamp *ar, int16_t *buf, size_t nsamp,
			     int channels)
{
	while (nsamp > 0) {

		size_t len = min(nsamp, FIR_MAX_INPUT_LEN);

		fir_process(&ar->fir, ar->coeffv, buf, buf, len, ar->coeffn,
			    channels);

		buf   += (len*channels);
		nsamp -= len;
	}
}


int auresamp_alloc(struct auresamp **arp, uint32_t srate_in, uint8_t ch_in,
		   uint32_t srate_out, uint8_t ch_out)
{
	struct auresamp *ar;
	int err = 0;

	if (!arp || !srate_in || !srate_out)
		return EINVAL;

	ar = mem_zalloc(sizeof(*ar), NULL);
	if (!ar)
		return ENOMEM;

	ar->ratio = 1.0 * srate_out / srate_in;
	ar->ch_in = ch_in;
	ar->ch_out = ch_out;

	fir_init(&ar->fir);

	if (ch_in == 1 && ch_out == 1)
		ar->resample = resample;
	else if (ch_in == 1 && ch_out == 2)
		ar->resample = resample_mono2stereo;
	else if (ch_in == 2 && ch_out == 1)
		ar->resample = resample_stereo2mono;
	else if (ch_in == 2 && ch_out == 2)
		ar->resample = resample_stereo;
	else {
		err = EINVAL;
		goto out;
	}

	if (srate_in == 8000 || srate_out == 8000) {

		ar->coeffv = fir_lowpass_48_4;
		ar->coeffn = (int)ARRAY_SIZE(fir_lowpass_48_4);

		(void)re_printf("auresamp: using 4000 Hz cutoff\n");
	}
	else {

		ar->coeffv = fir_lowpass_48_8;
		ar->coeffn = (int)ARRAY_SIZE(fir_lowpass_48_8);

		(void)re_printf("auresamp: using 8000 Hz cutoff\n");
	}

 out:
	if (err)
		mem_deref(ar);
	else
		*arp = ar;

	return err;
}


int auresamp_process(struct auresamp *ar, struct mbuf *dst, struct mbuf *src)
{
	size_t ns, nd, sz;
	int16_t *s, *d;

	if (!ar || !dst || !src)
		return EINVAL;

	ns = mbuf_get_left(src) / 2 / ar->ch_in;
	nd = (size_t)(ns * ar->ratio);
	sz = nd * 2 * ar->ch_out;

	if (mbuf_get_space(dst) < sz) {

		int err;

		err = mbuf_resize(dst, dst->pos + sz);
		if (err)
			return err;
	}

	s = (void *)mbuf_buf(src);
	d = (void *)mbuf_buf(dst);

	if (ar->ratio > 1) {

		ar->resample(ar, d, s, nd);
		auresamp_lowpass(ar, d, nd, ar->ch_out);
	}
	else if (ar->ratio < 1) {

		/* decimation: low-pass filter, then downsample */

		auresamp_lowpass(ar, s, ns, ar->ch_in);
		ar->resample(ar, d, s, nd);
	}
	else {
		ar->resample(ar, d, s, nd);
	}

	dst->end = dst->pos += sz;

	return 0;
}
