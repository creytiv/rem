/**
 * @file rem_aubuf.h Audio Buffer
 *
 * Copyright (C) 2010 Creytiv.com
 */

struct aubuf;

int  aubuf_alloc(struct aubuf **abp, size_t min_sz, size_t max_sz);
int  aubuf_append(struct aubuf *ab, struct mbuf *mb);
int  aubuf_write(struct aubuf *ab, const uint8_t *p, size_t sz);
void aubuf_read(struct aubuf *ab, uint8_t *p, size_t sz);
int  aubuf_get(struct aubuf *ab, uint32_t ptime, uint8_t *p, size_t sz);
void aubuf_flush(struct aubuf *ab);
int  aubuf_debug(struct re_printf *pf, const struct aubuf *ab);
size_t aubuf_cur_size(const struct aubuf *ab);
