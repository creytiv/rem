/**
 * @file h264/sps.c H.264 SPS parser
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <rem_h264.h>
#include <rem_vid.h>


enum {
	MAX_SPS_COUNT          = 32,
	MAX_LOG2_MAX_FRAME_NUM = 12,
	MACROBLOCK_SIZE        = 16,
	MAX_PIXELS             = 1048576,
};


struct getbitcontext {
	const uint8_t *buffer;
	size_t pos;
	size_t size;
};


static void getbit_init(struct getbitcontext *s, const uint8_t *buffer,
			size_t bit_size)
{
	s->buffer = buffer;
	s->pos    = 0;
	s->size   = bit_size;
}


static size_t getbit_get_left(const struct getbitcontext *gb)
{
	return gb->size - gb->pos;
}


static unsigned get_bit(struct getbitcontext *gb)
{
	const uint8_t *p = gb->buffer;
	unsigned tmp;

	if (gb->pos >= gb->size) {
		re_fprintf(stderr, "sps: get_bit: read past end\n");
		return 0;
	}

	tmp = ((*(p + (gb->pos >> 0x3))) >> (0x7 - (gb->pos & 0x7))) & 0x1;

	++gb->pos;

	return tmp;
}


static unsigned get_bits(struct getbitcontext *gb, uint8_t bits)
{
	unsigned value = 0;
	uint8_t i;

	for (i = 0; i < bits; i++) {
		value = (value << 1) | (get_bit(gb) ? 1 : 0);
	}

	return value;
}


static int get_ue_golomb(struct getbitcontext *gb, unsigned *valp)
{
	unsigned zeros = 0;
	unsigned info;
	int i;

	while (1) {

		if (getbit_get_left(gb) < 1)
			return ENODATA;

		if (0 == get_bit(gb))
			++zeros;
		else
			break;
	}

	info = 1 << zeros;

	for (i = zeros - 1; i >= 0; i--) {

		if (getbit_get_left(gb) < 1)
			return ENODATA;

		info |= get_bit(gb) << i;
	}

	if (valp)
		*valp = info - 1;

	return 0;
}


/**
 * Decode a Sequence Parameter Set (SPS) bitstream
 *
 * @param sps  Decoded H.264 SPS
 * @param p    SPS bitstream to decode, excluding NAL header
 * @param len  Number of bytes
 *
 * @return 0 if success, otherwise errorcode
 */
int h264_sps_decode(struct h264_sps *sps, const uint8_t *p, size_t len)
{
	struct getbitcontext gb;
	uint8_t profile_idc;
	unsigned seq_parameter_set_id;
	unsigned log2_max_frame_num_minus4;
	unsigned frame_mbs_only_flag;
	unsigned chroma_format_idc = 1;
	bool direct_8x8_inference_flag;
	bool frame_cropping_flag;
	unsigned mb_w_m1;
	unsigned mb_h_m1;
	int err;

	if (!sps || !p || !len)
		return EINVAL;

	memset(sps, 0, sizeof(*sps));

	getbit_init(&gb, p, len*8);

	if (getbit_get_left(&gb) < 24)
		return ENODATA;

	profile_idc = get_bits(&gb, 8);
	(void)get_bits(&gb, 8);
	sps->level_idc = get_bits(&gb, 8);

	err = get_ue_golomb(&gb, &seq_parameter_set_id);
	if (err)
		return err;

	if (seq_parameter_set_id >= MAX_SPS_COUNT) {
		re_fprintf(stderr, "h264: sps: sps_id %u out of range\n",
			   seq_parameter_set_id);
		return EBADMSG;
	}

	if (profile_idc == 100 ||
	    profile_idc == 110 ||
	    profile_idc == 122 ||
	    profile_idc == 244 ||
	    profile_idc ==  44 ||
	    profile_idc ==  83 ||
	    profile_idc ==  86 ||
	    profile_idc == 118 ||
	    profile_idc == 128 ||
	    profile_idc == 138 ||
	    profile_idc == 144) {

		unsigned seq_scaling_matrix_present_flag;

		err = get_ue_golomb(&gb, &chroma_format_idc);
		if (err)
			return err;
		if (chroma_format_idc == 3) {

			if (getbit_get_left(&gb) < 1)
				return ENODATA;

			/* separate_colour_plane_flag */
			(void)get_bits(&gb, 1);
		}

		/* bit_depth_luma/chroma */
		err  = get_ue_golomb(&gb, NULL);
		err |= get_ue_golomb(&gb, NULL);
		if (err)
			return err;

		if (getbit_get_left(&gb) < 2)
			return ENODATA;

		/* qpprime_y_zero_transform_bypass_flag */
		get_bits(&gb, 1);

		seq_scaling_matrix_present_flag = get_bits(&gb, 1);
		if (seq_scaling_matrix_present_flag) {
			re_fprintf(stderr, "sps: seq_scaling_matrix"
				   " not supported\n");
			return ENOTSUP;
		}
	}

	err = get_ue_golomb(&gb, &log2_max_frame_num_minus4);
	if (err)
		return err;
	if (log2_max_frame_num_minus4 > MAX_LOG2_MAX_FRAME_NUM) {
		re_fprintf(stderr, "h264: sps: log2_max_frame_num_minus4"
			   " out of range (0-12): %d\n",
			   log2_max_frame_num_minus4);
		return EBADMSG;
	}

	err = get_ue_golomb(&gb, &sps->pic_order_cnt_type);
	if (err)
		return err;

	if (sps->pic_order_cnt_type == 0) {

		/* log2_max_pic_order_cnt_lsb */
		err = get_ue_golomb(&gb, NULL);
		if (err)
			return err;
	}
	else if (sps->pic_order_cnt_type == 2) {
	}
	else {
		re_fprintf(stderr, "h264: sps: WARNING:"
			   " unknown pic_order_cnt_type (%u)\n",
			   sps->pic_order_cnt_type);
		return ENOTSUP;
	}

	err = get_ue_golomb(&gb, &sps->max_num_ref_frames);
	if (err)
		return err;

	if (getbit_get_left(&gb) < 1)
		return ENODATA;

	/* gaps_in_frame_num_value_allowed_flag */
	(void)get_bits(&gb, 1);

	err  = get_ue_golomb(&gb, &mb_w_m1);
	err |= get_ue_golomb(&gb, &mb_h_m1);
	if (err)
		return err;

	if (getbit_get_left(&gb) < 1)
		return ENODATA;
	frame_mbs_only_flag = get_bits(&gb, 1);

	if (mb_w_m1 >= MAX_PIXELS || mb_h_m1 >= MAX_PIXELS) {
		re_fprintf(stderr, "h264: sps: width/height overflow\n");
		return EBADMSG;
	}

	sps->pic_width_in_mbs        = mb_w_m1 + 1;
	sps->pic_height_in_map_units = mb_h_m1 + 1;

	sps->pic_height_in_map_units *= 2 - frame_mbs_only_flag;

	if (!frame_mbs_only_flag) {

		if (getbit_get_left(&gb) < 1)
			return ENODATA;

		/* mb_adaptive_frame_field_flag */
		(void)get_bits(&gb, 1);
	}

	if (getbit_get_left(&gb) < 1)
		return ENODATA;

	direct_8x8_inference_flag = get_bits(&gb, 1);
	(void)direct_8x8_inference_flag;

	if (getbit_get_left(&gb) < 1)
		return ENODATA;

	frame_cropping_flag = get_bits(&gb, 1);

	if (frame_cropping_flag) {

		unsigned frame_crop_left_offset;
		unsigned frame_crop_right_offset;
		unsigned frame_crop_top_offset;
		unsigned frame_crop_bottom_offset;

		int vsub   = (chroma_format_idc == 1) ? 1 : 0;
		int hsub   = (chroma_format_idc == 1 ||
			      chroma_format_idc == 2) ? 1 : 0;
		int sx = 1 << hsub;
		int sy = (2 - frame_mbs_only_flag) << vsub;

		err  = get_ue_golomb(&gb, &frame_crop_left_offset);
		err |= get_ue_golomb(&gb, &frame_crop_right_offset);
		err |= get_ue_golomb(&gb, &frame_crop_top_offset);
		err |= get_ue_golomb(&gb, &frame_crop_bottom_offset);
		if (err)
			return err;

		sps->frame_crop_left_offset   = sx * frame_crop_left_offset;
		sps->frame_crop_right_offset  = sx * frame_crop_right_offset;
		sps->frame_crop_top_offset    = sy * frame_crop_top_offset;
		sps->frame_crop_bottom_offset = sy * frame_crop_bottom_offset;
	}

	/* success */
	sps->profile_idc = profile_idc;
	sps->seq_parameter_set_id = seq_parameter_set_id;
	sps->chroma_format_idc = chroma_format_idc;
	sps->log2_max_frame_num = log2_max_frame_num_minus4 + 4;

	re_printf("sps: done. read %zu bits, %zu bits left\n",
		  gb.pos, getbit_get_left(&gb));

	return 0;
}


void h264_sps_resolution(const struct h264_sps *sps, struct vidsz *sz)
{
	unsigned width, height;

	if (!sps || !sz)
		return;

	width = MACROBLOCK_SIZE * sps->pic_width_in_mbs
		- sps->frame_crop_left_offset
		- sps->frame_crop_right_offset;

	height = MACROBLOCK_SIZE * sps->pic_height_in_map_units
		- sps->frame_crop_top_offset
		- sps->frame_crop_bottom_offset;

	sz->w = width;
	sz->h = height;
}


void h264_sps_print(const struct h264_sps *sps)
{
	if (!sps)
		return;

	re_printf("--- SPS ---\n");
	re_printf("profile_idc          %u\n", sps->profile_idc);
	re_printf("level_idc            %u\n", sps->level_idc);
	re_printf("seq_parameter_set_id %u\n", sps->seq_parameter_set_id);
	re_printf("chroma_format_idc    %u (%s)\n",
		  sps->chroma_format_idc,
		  h264_sps_chroma_format_name(sps->chroma_format_idc));
	re_printf("\n");

	re_printf("log2_max_frame_num         %u\n",
		  sps->log2_max_frame_num);
	re_printf("pic_order_cnt_type         %u\n",
		  sps->pic_order_cnt_type);
	re_printf("\n");

	re_printf("max_num_ref_frames                   %u\n",
		  sps->max_num_ref_frames);
	re_printf("pic_width_in_mbs                     %u\n",
		  sps->pic_width_in_mbs);
	re_printf("pic_height_in_map_units              %u\n",
		  sps->pic_height_in_map_units);
	re_printf("\n");
}


const char *h264_sps_chroma_format_name(unsigned chroma_format_idc)
{
	switch (chroma_format_idc) {

	case 0: return "monochrome";
	case 1: return "YUV420";
	case 2: return "YUV422";
	case 3: return "YUV444";
	default: return "???";
	}
}
