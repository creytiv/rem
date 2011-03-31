#include <string.h>
#include <stdlib.h>
#include <re.h>
#include <rem_auresamp.h>


enum { FIR_SIZE = 16 };


struct auresamp {
	int16_t hist[FIR_SIZE + 2];
	int channels;
	double ratio;
};


static const int16_t fireven[FIR_SIZE] = {
	  -1,   26,   89, -26,
	 -15,   77, -175, 404,
	 777,  -16,  -67,  81,
	 -71,   54,   66,   3
};

static const int16_t firodd[FIR_SIZE] = {
	   3,   66,  54, -71,
	  81,  -67, -16, 777,
	 404, -175,  77, -15,
	 -26,   89,  26,  -1
};


static inline int16_t saturate_s16(int32_t a)
{
	if (a > INT16_MAX)
		return INT16_MAX;
	else if (a < INT16_MIN)
		return INT16_MIN;
	else
		return a;
}


static inline void upsample(struct auresamp *ar, int16_t *dst,
			    const int16_t *src, size_t nsamp)
{
	int ratio = (int)ar->ratio;
	int i;

	while (nsamp--) {

		int32_t sumv[ratio];
		const int16_t *hp = ar->hist, *hp_max = hp + FIR_SIZE;
		const int16_t *cpe = fireven, *cpo = firodd;

		memset(sumv, 0, sizeof(sumv));

		memmove(ar->hist + 1, ar->hist, (FIR_SIZE - 1) * 2);
		ar->hist[0] = *src++;

		for (;hp < hp_max; hp++) {

			for (i=0; i<ratio/2; i++) {

				sumv[2*i]   += (int32_t)(*cpe) * (int32_t)*hp;
				sumv[2*i+1] += (int32_t)(*cpo) * (int32_t)*hp;
			}

			++cpe;
			++cpo;
		}

		for (i=0; i<ratio; i++)
			*dst++ = saturate_s16(sumv[i] >> 10);
	}
}


static inline void downsample(struct auresamp *ar, int16_t *dst,
			      const int16_t *src, size_t nsamp)
{
	size_t i;

	for (i=0; i<nsamp; i += 1/ar->ratio) {

		*dst++ = src[i];
	}
}


int auresamp_alloc(struct auresamp **arp, int channels, double ratio)
{
	struct auresamp *ar;

	if (!arp || channels != 1 || ratio == 0)
		return EINVAL;

	re_printf("allocate resampler: ratio=%f\n", ratio);

	ar = mem_zalloc(sizeof(*ar), NULL);
	if (!ar)
		return ENOMEM;

	ar->channels = channels;
	ar->ratio = ratio;

	*arp = ar;

	return 0;
}


int auresamp_process(struct auresamp *ar, struct mbuf *mb)
{
	const size_t nsamp = mbuf_get_left(mb) / 2;
	const size_t nsamp_out = nsamp * (ar ? ar->ratio : 0);
	int16_t buf[nsamp_out];
	size_t pos;

	// todo: alloc new mb->buf and replace

	if (!ar || !mb)
		return EINVAL;

	pos = mb->pos;

	if (ar->ratio > 1) {

		upsample(ar, buf, (void *)mbuf_buf(mb), nsamp);
	}
	else {
		downsample(ar, buf, (void *)mbuf_buf(mb), nsamp);
	}

	mb->pos = mb->end = pos;

	return mbuf_write_mem(mb, (void *)buf, nsamp_out * 2);
}
