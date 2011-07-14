/**
 * @file fir.c FIR -- Finite Impulse Response
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <stdlib.h>
#include <re.h>
#include <rem_fir.h>


/*
 * FIR -- Finite Impulse Response
 *
 * Inspiration:
 *
 *     http://sestevenson.files.wordpress.com/2009/10/firfixed.pdf
 */


void fir_init(struct fir *fir)
{
	memset(fir->insamp, 0, sizeof(fir->insamp));
}


void fir_process(struct fir *fir, const int16_t *coeffs,
		 const int16_t *input, int16_t *output,
		 size_t length, int filterLength)
{
	int32_t acc;
	const int16_t *coeffp;
	int16_t *inputp;
	size_t n;
	int k;

	/* put the new samples at the high end of the buffer */
	memcpy( &fir->insamp[filterLength - 1], input,
		length * sizeof(int16_t) );

	/* apply the filter to each input sample */
	for ( n = 0; n < length; n++ ) {

		coeffp = coeffs;
		inputp = &fir->insamp[filterLength - 1 + n];

		/* load rounding constant */
		acc = 1 << 14;

		/* perform the multiply-accumulate */
		for ( k = 0; k < filterLength; k++ ) {
			acc += (int32_t)(*coeffp++) * (int32_t)(*inputp--);
		}

		/* saturate the result */
		if ( acc > 0x3fffffff ) {
			acc = 0x3fffffff;
		}
		else if ( acc < -0x40000000 ) {
			acc = -0x40000000;
		}

		/* convert from Q30 to Q15 */
		output[n] = (int16_t)(acc >> 15);
	}

	/* shift input samples back in time for next time */
	memmove( &fir->insamp[0], &fir->insamp[length],
		 (filterLength - 1) * sizeof(int16_t) );
}
