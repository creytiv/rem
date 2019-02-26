/**
 * @file h264/nal.c H.264 header parser
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <re_types.h>
#include <re_mbuf.h>
#include <rem_h264.h>


/**
 * Encode H.264 NAL header
 *
 * @param mb  Buffer to encode into
 * @param hdr H.264 NAL header to encode
 *
 * @return 0 if success, otherwise errorcode
 */
int h264_nal_header_encode(struct mbuf *mb, const struct h264_nal_header *hdr)
{
	uint8_t v;

	if (!mb || !hdr)
		return EINVAL;

	v = hdr->f<<7 | hdr->nri<<5 | hdr->type;

	return mbuf_write_u8(mb, v);
}


/**
 * Decode H.264 NAL header
 *
 * @param hdr H.264 NAL header to decode into
 * @param mb  Buffer to decode
 *
 * @return 0 if success, otherwise errorcode
 */
int h264_nal_header_decode(struct h264_nal_header *hdr, struct mbuf *mb)
{
	uint8_t v;

	if (!hdr || !mb)
		return EINVAL;
	if (mbuf_get_left(mb) < 1)
		return EBADMSG;

	v = mbuf_read_u8(mb);

	hdr->f    = v>>7 & 0x1;
	hdr->nri  = v>>5 & 0x3;
	hdr->type = v    & 0x1f;

	return 0;
}


/**
 * Get the name of an H.264 NAL unit
 *
 * @param nal_type NAL unit type
 *
 * @return A string containing the NAL unit name
 */
const char *h264_nal_unit_name(enum h264_nalu nal_type)
{
	switch (nal_type) {

	case H264_NALU_SLICE:        return "SLICE";
	case H264_NALU_DPA:          return "DPA";
	case H264_NALU_DPB:          return "DPB";
	case H264_NALU_DPC:          return "DPC";
	case H264_NALU_IDR_SLICE:    return "IDR_SLICE";
	case H264_NALU_SEI:          return "SEI";
	case H264_NALU_SPS:          return "SPS";
	case H264_NALU_PPS:          return "PPS";
	case H264_NALU_AUD:          return "AUD";
	case H264_NALU_END_SEQUENCE: return "END_SEQUENCE";
	case H264_NALU_END_STREAM:   return "END_STREAM";
	case H264_NALU_FILLER_DATA:  return "FILLER_DATA";
	case H264_NALU_SPS_EXT:      return "SPS_EXT";
	case H264_NALU_AUX_SLICE:    return "AUX_SLICE";
	case H264_NALU_STAP_A:       return "STAP-A";
	case H264_NALU_STAP_B:       return "STAP-B";
	case H264_NALU_MTAP16:       return "MTAP16";
	case H264_NALU_MTAP24:       return "MTAP24";
	case H264_NALU_FU_A:         return "FU-A";
	case H264_NALU_FU_B:         return "FU-B";
	}

	return "???";
}
