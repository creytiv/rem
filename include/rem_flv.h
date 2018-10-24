/**
 * @file rem_flv.h Flash Video File Format
 *
 * Copyright (C) 2010 Creytiv.com
 */


/*
 * Audio
 */

enum {
	FLV_AUCODEC_PCM       = 0,
	FLV_AUCODEC_ALAW      = 7,
	FLV_AUCODEC_ULAW      = 8,
	FLV_AUCODEC_AAC       = 10,
};

enum {
	FLV_SRATE_5500HZ  = 0,
	FLV_SRATE_11000HZ = 1,
	FLV_SRATE_22000HZ = 2,
	FLV_SRATE_44000HZ = 3,
};

enum aac_packet_type {
	AAC_SEQUENCE_HEADER = 0,
	AAC_RAW             = 1
};


/*
 * Video
 */

enum {
	FLV_VIDFRAME_KEY            = 1,
	FLV_VIDFRAME_INTER          = 2,
	FLV_VIDFRAME_DISP_INTER     = 3,
	FLV_VIDFRAME_GENERATED_KEY  = 4,
	FLV_VIDFRAME_VIDEO_INFO_CMD = 5,
};

enum {
	FLV_VIDCODEC_H263  = 2,
	FLV_VIDCODEC_H264  = 7,
	FLV_VIDCODEC_MPEG4 = 9,
};

enum avc_packet_type {
	AVC_SEQUENCE = 0,
	AVC_NALU     = 1,
	AVC_EOS      = 2
};

struct avc_config_record {
	uint8_t version;
	uint8_t profile_ind;
	uint8_t profile_compat;
	uint8_t level_ind;
	uint8_t lengthsizeminusone;
	uint8_t sps_count;
	uint16_t sps_len;
	uint8_t *sps;
	uint8_t pps_count;
	uint16_t pps_len;
	uint8_t *pps;
};


int flv_config_record_encode(struct mbuf *mb,
			     uint8_t profile_ind,
			     uint8_t profile_compat,
			     uint8_t level_ind,
			     uint16_t sps_length,
			     const uint8_t *sps,
			     uint16_t pps_length,
			     const uint8_t *pps);
int flv_config_record_decode(struct avc_config_record **confp,
			     struct mbuf *mb);
