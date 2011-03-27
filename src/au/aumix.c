/**
 * @file aumix.c Audio Mixer
 *
 * Copyright (C) 2010 Creytiv.com
 */

#define _BSD_SOURCE 1
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <re.h>
#include <rem_au.h>


#define FRAME_SIZE (160)
#define SRATE (8000)
#define PTIME (FRAME_SIZE / (SRATE / 1000))


struct aumix {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct list srcl;
	pthread_t thread;
	int run;
};

struct aumix_source {
	struct le le;
	int16_t frame[FRAME_SIZE];
	struct aubuf *aubuf;
	struct aumix *mix;
	aumix_frame_h *fh;
	void *arg;
};


static void dummy_frame_handler(uint32_t ts, const uint8_t *buf, size_t sz,
				void *arg)
{
	(void)ts;
	(void)buf;
	(void)sz;
	(void)arg;
}


static void destructor(void *arg)
{
	struct aumix *mix = arg;

	if (mix->run == 2) {
		re_printf("waiting for aumix thread to terminate\n");
		pthread_mutex_lock(&mix->mutex);
		mix->run = 0;
		pthread_cond_signal(&mix->cond);
		pthread_mutex_unlock(&mix->mutex);

		pthread_join(mix->thread, NULL);
		re_printf("aumix thread joined\n");
	}

	list_flush(&mix->srcl);
}


static void source_destructor(void *arg)
{
	struct aumix_source *src = arg;

	pthread_mutex_lock(&src->mix->mutex);
	list_unlink(&src->le);
	pthread_mutex_unlock(&src->mix->mutex);

	mem_deref(src->aubuf);
}


static void *aumix_thread(void *arg)
{
	struct aumix *mix = arg;
	uint64_t ts = 0;

	re_printf("aumix thread start\n");

	pthread_mutex_lock(&mix->mutex);

	while (mix->run) {

		struct le *le;
		uint64_t now;

		if (!mix->srcl.head) {
			re_printf("aumix thread sleep\n");
			pthread_cond_wait(&mix->cond, &mix->mutex);
			ts = 0;
			re_printf("aumix thread wakeup\n");
		}
		else {
			pthread_mutex_unlock(&mix->mutex);
			(void)usleep(4000);
			pthread_mutex_lock(&mix->mutex);
		}

		now = tmr_jiffies();
		if (!ts)
			ts = now;

		if (ts > now)
			continue;

		for (le=mix->srcl.head; le; le=le->next) {

			struct aumix_source *src = le->data;

			aubuf_read(src->aubuf, (uint8_t *)src->frame,
				   sizeof(src->frame));
		}

		for (le=mix->srcl.head; le; le=le->next) {

			struct aumix_source *src = le->data;
			int16_t frame[FRAME_SIZE];
			struct le *cle;

			memset(frame, 0, sizeof(frame));

			for (cle=mix->srcl.head; cle; cle=cle->next) {

				struct aumix_source *csrc = cle->data;
				size_t i;
#if 1
				/* skip self */
				if (csrc == src)
					continue;
#endif
				for (i=0; i<FRAME_SIZE; i++)
					frame[i] += csrc->frame[i];
			}

			src->fh((uint32_t)ts * (SRATE/1000), (uint8_t *)frame,
				sizeof(frame), src->arg);
		}

		ts += PTIME;
	}

	pthread_mutex_unlock(&mix->mutex);

	re_printf("aumix thread stop\n");

	return NULL;
}


int aumix_alloc(struct aumix **mixp)
{
	struct aumix *mix;
	int err;

	if (!mixp)
		return EINVAL;

	mix = mem_zalloc(sizeof(*mix), destructor);
	if (!mix)
		return ENOMEM;

	err = pthread_mutex_init(&mix->mutex, NULL);
	if (err)
		goto out;

	err = pthread_cond_init(&mix->cond, NULL);
	if (err)
		goto out;

	mix->run = 1;

	err = pthread_create(&mix->thread, NULL, aumix_thread, mix);
	if (err)
		goto out;

	pthread_mutex_lock(&mix->mutex);
	mix->run = 2;
	pthread_mutex_unlock(&mix->mutex);

 out:
	if (err)
		mem_deref(mix);
	else
		*mixp = mix;

	return err;
}


int aumix_source_add(struct aumix_source **srcp, struct aumix *mix,
		     aumix_frame_h *fh, void *arg)
{
	struct aumix_source *src;
	int err;

	if (!srcp || !mix)
		return EINVAL;

	src = mem_zalloc(sizeof(*src), source_destructor);
	if (!src)
		return ENOMEM;

	src->mix = mix;
	src->fh  = fh ? fh : dummy_frame_handler;
	src->arg = arg;

	err = aubuf_alloc(&src->aubuf, 320 * 5, 320 * 10);
	if (err)
		goto out;

	pthread_mutex_lock(&mix->mutex);
	list_append(&mix->srcl, &src->le, src);
	pthread_cond_signal(&mix->cond);
	pthread_mutex_unlock(&mix->mutex);

 out:
	if (err)
		mem_deref(src);
	else
		*srcp = src;

	return err;
}


int aumix_source_put(struct aumix_source *src, struct mbuf *mb)
{
	if (!src || !mb)
		return EINVAL;

	return aubuf_append(src->aubuf, mb);
}
