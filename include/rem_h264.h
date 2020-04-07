/**
 * @file rem_h264.h Interface to H.264 header parser
 *
 * Copyright (C) 2010 Creytiv.com
 */


/** NAL unit types */
enum h264_nalu {
	H264_NALU_SLICE        = 1,
	H264_NALU_DPA          = 2,
	H264_NALU_DPB          = 3,
	H264_NALU_DPC          = 4,
	H264_NALU_IDR_SLICE    = 5,
	H264_NALU_SEI          = 6,
	H264_NALU_SPS          = 7,
	H264_NALU_PPS          = 8,
	H264_NALU_AUD          = 9,
	H264_NALU_END_SEQUENCE = 10,
	H264_NALU_END_STREAM   = 11,
	H264_NALU_FILLER_DATA  = 12,
	H264_NALU_SPS_EXT      = 13,
	H264_NALU_AUX_SLICE    = 19,

	H264_NALU_STAP_A       = 24,
	H264_NALU_STAP_B       = 25,
	H264_NALU_MTAP16       = 26,
	H264_NALU_MTAP24       = 27,
	H264_NALU_FU_A         = 28,
	H264_NALU_FU_B         = 29,
};


/**
 * H.264 NAL Header
 */
struct h264_nal_header {
	unsigned f:1;      /**< Forbidden zero bit (must be 0) */
	unsigned nri:2;    /**< nal_ref_idc                    */
	unsigned type:5;   /**< NAL unit type                  */
};


int h264_nal_header_encode(struct mbuf *mb, const struct h264_nal_header *hdr);
int h264_nal_header_decode(struct h264_nal_header *hdr, struct mbuf *mb);
const char *h264_nal_unit_name(enum h264_nalu nal_type);


/**
 * H.264 Sequence Parameter Set (SPS)
 */
struct h264_sps {
	uint8_t profile_idc;
	uint8_t level_idc;
	uint8_t seq_parameter_set_id;               /* 0-31 */
	uint8_t chroma_format_idc;                  /* 0-3 */

	unsigned log2_max_frame_num;
	unsigned pic_order_cnt_type;

	unsigned max_num_ref_frames;
	unsigned pic_width_in_mbs;
	unsigned pic_height_in_map_units;

	unsigned frame_crop_left_offset;            /* pixels */
	unsigned frame_crop_right_offset;           /* pixels */
	unsigned frame_crop_top_offset;             /* pixels */
	unsigned frame_crop_bottom_offset;          /* pixels */
};

struct vidsz;

int  h264_sps_decode(struct h264_sps *sps, const uint8_t *p, size_t len);
void h264_sps_resolution(const struct h264_sps *sps, struct vidsz *sz);
const char *h264_sps_chroma_format_name(uint8_t chroma_format_idc);
