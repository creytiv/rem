/**
 * @file aubuf.c  Audio Buffer
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <rem_aubuf.h>


#define AUBUF_DEBUG 0


/** Locked audio-buffer with almost zero-copy */
struct aubuf {
	struct list afl;
	struct lock *lock;
	size_t wish_sz;
	size_t cur_sz;
	size_t max_sz;
	bool filling;
	uint64_t ts;

#if AUBUF_DEBUG
	struct {
		size_t or;
		size_t ur;
	} stats;
#endif
};


struct auframe {
	struct le le;
	struct mbuf *mb;
};


static void auframe_destructor(void *arg)
{
	struct auframe *af = arg;

	list_unlink(&af->le);
	mem_deref(af->mb);
}


static void aubuf_destructor(void *arg)
{
	struct aubuf *ab = arg;

	list_flush(&ab->afl);
	mem_deref(ab->lock);
}


int aubuf_alloc(struct aubuf **abp, size_t min_sz, size_t max_sz)
{
	struct aubuf *ab;
	int err;

	if (!abp || !min_sz)
		return EINVAL;

	ab = mem_zalloc(sizeof(*ab), aubuf_destructor);
	if (!ab)
		return ENOMEM;

	err = lock_alloc(&ab->lock);
	if (err)
		goto out;

	ab->wish_sz = min_sz;
	ab->max_sz = max_sz;
	ab->filling = true;

 out:
	if (err)
		mem_deref(ab);
	else
		*abp = ab;

	return err;
}


int aubuf_append(struct aubuf *ab, struct mbuf *mb)
{
	struct auframe *af;

	if (!ab || !mb)
		return EINVAL;

	af = mem_zalloc(sizeof(*af), auframe_destructor);
	if (!af)
		return ENOMEM;

	af->mb = mem_ref(mb);

	lock_write_get(ab->lock);

	list_append(&ab->afl, &af->le, af);
	ab->cur_sz += mbuf_get_left(mb);

	if (ab->max_sz && ab->cur_sz > ab->max_sz) {
#if AUBUF_DEBUG
		++ab->stats.or;
		(void)re_printf("aubuf: %p overrun (cur=%u)\n",
				ab, ab->cur_sz);
#endif
		af = list_ledata(ab->afl.head);
		ab->cur_sz -= mbuf_get_left(af->mb);
		mem_deref(af);
	}

	lock_rel(ab->lock);

	return 0;
}


int aubuf_write(struct aubuf *ab, const uint8_t *p, size_t sz)
{
	struct mbuf *mb = mbuf_alloc(sz);
	int err;

	if (!mb)
		return ENOMEM;

	(void)mbuf_write_mem(mb, p, sz);
	mb->pos = 0;

	err = aubuf_append(ab, mb);

	mem_deref(mb);

	return err;
}


void aubuf_read(struct aubuf *ab, uint8_t *p, size_t sz)
{
	struct le *le;

	if (!ab || !p || !sz)
		return;

	lock_write_get(ab->lock);

	if (ab->cur_sz <= (ab->filling ? ab->wish_sz : 0)) {
#if AUBUF_DEBUG
		++ab->stats.ur;
		(void)re_printf("aubuf: %p underrun filling=%d\n",
				ab, ab->filling);
#endif
		ab->filling = true;
		memset(p, 0, sz);
		goto out;
	}

	ab->filling = false;

	le = ab->afl.head;

	while (le) {
		struct auframe *af = le->data;
		size_t n;

		le = le->next;

		n = min(mbuf_get_left(af->mb), sz);

		(void)mbuf_read_mem(af->mb, p, n);
		ab->cur_sz -= n;

		if (!mbuf_get_left(af->mb))
			mem_deref(af);

		if (n == sz)
			break;

		p  += n;
		sz -= n;
	}

 out:
	lock_rel(ab->lock);
}


/** Timed read */
int aubuf_get(struct aubuf *ab, uint32_t ptime, uint8_t *p, size_t sz)
{
	uint64_t now;
	int err = 0;

	if (!ab || !ptime)
		return EINVAL;

	lock_write_get(ab->lock);

	now = tmr_jiffies();
	if (!ab->ts)
		ab->ts = now;

	if (now < ab->ts) {
		err = ETIMEDOUT;
		goto out;
	}

	ab->ts += ptime;

 out:
	lock_rel(ab->lock);

	if (!err)
		aubuf_read(ab, p, sz);

	return err;
}


int aubuf_debug(struct re_printf *pf, const struct aubuf *ab)
{
	int err;

	if (!ab)
		return 0;

	lock_read_get(ab->lock);
	err = re_hprintf(pf, "wish_sz=%zu cur_sz=%zu filling=%d",
			 ab->wish_sz, ab->cur_sz, ab->filling);

#if AUBUF_DEBUG
	err |= re_hprintf(pf, " [overrun=%u underrun=%u]",
			  ab->stats.or, ab->stats.ur);
#endif

	lock_rel(ab->lock);

	return err;
}


size_t aubuf_cur_size(const struct aubuf *ab)
{
	size_t sz;

	if (!ab)
		return 0;

	lock_read_get(ab->lock);
	sz = ab->cur_sz;
	lock_rel(ab->lock);

	return sz;
}
