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


int flv_config_record_encode(struct mbuf *mb,
			     uint8_t profile_ind,
			     uint8_t profile_compat,
			     uint8_t level_ind,
			     uint16_t spsLength,
			     uint8_t *sps,
			     uint16_t ppsLength,
			     uint8_t *pps)
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
	err |= mbuf_write_u16(mb, htons(spsLength));
	err |= mbuf_write_mem(mb, sps, spsLength);

	/* PPS */
	err |= mbuf_write_u8(mb, 1);
	err |= mbuf_write_u16(mb, htons(ppsLength));
	err |= mbuf_write_mem(mb, pps, ppsLength);

	return err;
}


int flv_config_record_decode(struct avc_config_record **confp, struct mbuf *mb)
{
	struct avc_config_record *conf;
	uint8_t v;
	size_t lengthSize;
	int err = 0;

	if (!confp || !mb)
		return EINVAL;

	conf = mem_zalloc(sizeof(*conf), NULL);
	if (!conf)
		return ENOMEM;

	conf->version = mbuf_read_u8(mb);

	if (conf->version != FLV_CONFIG_VERSION) {
		err = EBADMSG;
		goto out;
	}

	conf->profile_ind    = mbuf_read_u8(mb);
	conf->profile_compat = mbuf_read_u8(mb);
	conf->level_ind      = mbuf_read_u8(mb);

	v = mbuf_read_u8(mb);
	conf->lengthsizeminusone = v & 0x03;
	lengthSize = conf->lengthsizeminusone + 1;

	if (lengthSize != 4) {
		err = EPROTO;
		goto out;
	}

	/* SPS */
	v = mbuf_read_u8(mb);
	conf->sps_count = v & 0x1f;

	conf->sps_len = ntohs(mbuf_read_u16(mb));

	conf->sps = mem_alloc(conf->sps_len, NULL);

	err |= mbuf_read_mem(mb, conf->sps, conf->sps_len);

	/* PPS */
	conf->pps_count = mbuf_read_u8(mb);
	conf->pps_len = ntohs(mbuf_read_u16(mb));

	conf->pps = mem_alloc(conf->pps_len, NULL);

	err |= mbuf_read_mem(mb, conf->pps, conf->pps_len);

 out:
	if (err)
		mem_deref(conf);
	else
		*confp = conf;

	return err;
}
