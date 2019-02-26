/**
 * @file rem_aac.h Advanced Audio Coding
 *
 * Copyright (C) 2010 Creytiv.com
 */


/*
 * AAC Header
 */

struct aac_header {
	unsigned sample_rate;
	unsigned channels;
	unsigned frame_size;
};

int aac_header_decode(struct aac_header *hdr, const uint8_t *p, size_t len);
