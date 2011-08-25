/**
 * @file rem_fir.h  Finite Impulse Response (FIR) functions
 *
 * Copyright (C) 2010 Creytiv.com
 */


/* maximum number of inputs that can be handled in one function call */
#define FIR_MAX_INPUT_LEN   160

/* maximum length of filter than can be handled */
#define FIR_MAX_FLT_LEN     63

/* buffer to hold all of the input samples */
#define FIR_BUFFER_LEN      (FIR_MAX_FLT_LEN - 1 + FIR_MAX_INPUT_LEN)

#define FIR_MAX_CHANNELS    2


/** FIR filter state */
struct fir {
	int16_t insamp[FIR_MAX_CHANNELS][FIR_BUFFER_LEN];
};


void fir_init(struct fir *fir);
void fir_process(struct fir *fir, const int16_t *coeffs,
		 const int16_t *input, int16_t *output,
		 size_t length, int filterLength, int channels);
