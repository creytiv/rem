/**
 * @file flv.c Flash Video File Format
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mem.h>
#include <re_mbuf.h>
#include <re_net.h>
#include <re_sa.h>
#include <rem_flv.h>


#define FLV_CONFIG_VERSION 1


static void destructor(void *data)
{
	struct avc_config_record *conf = data;

	mem_deref(conf->sps);
	mem_deref(conf->pps);
}


int flv_config_record_encode(struct mbuf *mb,
			     uint8_t profile_ind,
			     uint8_t profile_compat,
			     uint8_t level_ind,
			     uint16_t sps_length,
			     const uint8_t *sps,
			     uint16_t pps_length,
			     const uint8_t *pps)
{
	int err = 0;

	if (!mb || !sps || !pps)
		return EINVAL;

	err |= mbuf_write_u8(mb, FLV_CONFIG_VERSION);

	err |= mbuf_write_u8(mb, profile_ind);
	err |= mbuf_write_u8(mb, profile_compat);
	err |= mbuf_write_u8(mb, level_ind);

	err |= mbuf_write_u8(mb, 0xfc | (4-1));

	/* SPS */
	err |= mbuf_write_u8(mb, 0xe0 | 1);
	err |= mbuf_write_u16(mb, htons(sps_length));
	err |= mbuf_write_mem(mb, sps, sps_length);

	/* PPS */
	err |= mbuf_write_u8(mb, 1);
	err |= mbuf_write_u16(mb, htons(pps_length));
	err |= mbuf_write_mem(mb, pps, pps_length);

	return err;
}


int flv_config_record_decode(struct avc_config_record **confp, struct mbuf *mb)
{
	struct avc_config_record *conf;
	uint8_t v;
	size_t length_size;
	int err = 0;

	if (!confp || !mb)
		return EINVAL;

	conf = mem_zalloc(sizeof(*conf), destructor);
	if (!conf)
		return ENOMEM;

	if (mbuf_get_left(mb) < 1) {
		err = EBADMSG;
		goto out;
	}
	conf->version = mbuf_read_u8(mb);

	if (conf->version != FLV_CONFIG_VERSION) {
		err = EBADMSG;
		goto out;
	}

	if (mbuf_get_left(mb) < 3) {
		err = EBADMSG;
		goto out;
	}

	conf->profile_ind    = mbuf_read_u8(mb);
	conf->profile_compat = mbuf_read_u8(mb);
	conf->level_ind      = mbuf_read_u8(mb);

	if (mbuf_get_left(mb) < 1) {
		err = EBADMSG;
		goto out;
	}

	v = mbuf_read_u8(mb);
	conf->lengthsizeminusone = v & 0x03;
	length_size = conf->lengthsizeminusone + 1;

	if (length_size != 4) {
		err = EPROTO;
		goto out;
	}

	/* SPS */
	if (mbuf_get_left(mb) < 3) {
		err = EBADMSG;
		goto out;
	}
	v = mbuf_read_u8(mb);
	conf->sps_count = v & 0x1f;

	conf->sps_len = ntohs(mbuf_read_u16(mb));

	if (mbuf_get_left(mb) < conf->sps_len) {
		err = EBADMSG;
		goto out;
	}

	conf->sps = mem_alloc(conf->sps_len, NULL);
	if (!conf->sps) {
		err = ENOMEM;
		goto out;
	}

	err |= mbuf_read_mem(mb, conf->sps, conf->sps_len);

	/* PPS */
	if (mbuf_get_left(mb) < 3) {
		err = EBADMSG;
		goto out;
	}
	conf->pps_count = mbuf_read_u8(mb);
	conf->pps_len = ntohs(mbuf_read_u16(mb));

	if (mbuf_get_left(mb) < conf->pps_len) {
		err = EBADMSG;
		goto out;
	}

	conf->pps = mem_alloc(conf->pps_len, NULL);
	if (!conf->pps) {
		err = ENOMEM;
		goto out;
	}

	err |= mbuf_read_mem(mb, conf->pps, conf->pps_len);

 out:
	if (err)
		mem_deref(conf);
	else
		*confp = conf;

	return err;
}
