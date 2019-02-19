/**
 * @file h264.c H.264 header parser
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <re_types.h>
#include <re_mbuf.h>
#include <rem_h264.h>


int h264_header_encode(struct mbuf *mb, const struct h264_nal_header *hdr)
{
	uint8_t v;

	if (!mb || !hdr)
		return EINVAL;

	v = hdr->f<<7 | hdr->nri<<5 | hdr->type<<0;

	return mbuf_write_u8(mb, v);
}


int h264_header_decode(struct h264_nal_header *hdr, struct mbuf *mb)
{
	uint8_t v;

	if (!hdr || !mb)
		return EINVAL;
	if (mbuf_get_left(mb) < 1)
		return EBADMSG;

	v = mbuf_read_u8(mb);

	hdr->f    = v>>7 & 0x1;
	hdr->nri  = v>>5 & 0x3;
	hdr->type = v>>0 & 0x1f;

	return 0;
}


const char *h264_nal_unit_name(int nal_type)
{
	switch (nal_type) {

	case H264_NALU_SLICE:       return "SLICE";
	case H264_NALU_DPA:         return "DPA";
	case H264_NALU_DPB:         return "DPB";
	case H264_NALU_DPC:         return "DPC";
	case H264_NALU_IDR_SLICE:   return "IDR_SLICE";
	case H264_NALU_SEI:         return "SEI";
	case H264_NALU_SPS:         return "SPS";
	case H264_NALU_PPS:         return "PPS";
	case H264_NALU_AUD:         return "AUD";
	case H264_NALU_FILLER_DATA: return "FILLER";

	case H264_NALU_FU_A:        return "FU-A";
	case H264_NALU_FU_B:        return "FU-B";
	}

	return "???";
}
