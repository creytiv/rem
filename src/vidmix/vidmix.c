/**
 * @file vidmix.c Video Mixer
 *
 * Copyright (C) 2010 Creytiv.com
 */

#define _BSD_SOURCE 1
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <re.h>
#include <rem_vid.h>
#include <rem_vidconv.h>
#include <rem_vidmix.h>


struct vidmix {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct list srcl;
	pthread_t thread;
	struct vidframe *frame;
	bool clear;
	bool focus;
	bool run;
	int fint;
};

struct vidmix_source {
	struct le le;
	pthread_mutex_t mutex;
	struct vidframe frame;
	struct vidmix *mix;
	vidmix_frame_h *fh;
	void *arg;
	bool focus;
};


static void clear_frame(struct vidframe *vf)
{
	vidframe_fill(vf, 0, 0, 0);
}


static void destructor(void *arg)
{
	struct vidmix *mix = arg;

	if (mix->run) {
		pthread_mutex_lock(&mix->mutex);
		mix->run = false;
		pthread_cond_signal(&mix->cond);
		pthread_mutex_unlock(&mix->mutex);

		pthread_join(mix->thread, NULL);
	}

	list_flush(&mix->srcl);
	mem_deref(mix->frame);
}


static void source_destructor(void *arg)
{
	struct vidmix_source *src = arg;
	struct vidmix *mix = src->mix;

	pthread_mutex_lock(&mix->mutex);
	list_unlink(&src->le);
	mix->clear = true;
	pthread_mutex_unlock(&mix->mutex);
}


static void source_mix(struct vidmix_source *src, unsigned n, unsigned rows,
		       unsigned idx, bool focus)
{
	struct vidframe *mframe, frame;
	struct vidrect rect;

	pthread_mutex_lock(&src->mutex);
	frame = src->frame;
	pthread_mutex_unlock(&src->mutex);

	if (!frame.data[0])
		return;

	mframe = src->mix->frame;

	if (focus) {

		n = max((n+1), 6)/2;

		if (src->focus) {
			rect.w = mframe->size.w * (n-1) / n;
			rect.h = mframe->size.h * (n-1) / n;
			rect.x = 0;
			rect.y = 0;
		}
		else {
			rect.w = mframe->size.w / n;
			rect.h = mframe->size.h / n;

			if (idx < n) {
				rect.x = mframe->size.w - rect.w;
				rect.y = rect.h * idx;
			}
			else if (idx < (n*2 - 1)) {
				rect.x = rect.w * (n*2 - 2 - idx);
				rect.y = mframe->size.h - rect.h;
			}
			else {
				return;
			}
		}
	}
	else {
		rect.w = mframe->size.w / rows;
		rect.h = mframe->size.h / rows;
		rect.x = rect.w * (idx % rows);
		rect.y = rect.h * (idx / rows);
	}

	vidconv_aspect(mframe, &frame, &rect);
}


static inline int calc_rows(uint32_t n)
{
       uint32_t rows;

       for (rows=2;; rows++)
               if (n <= (rows * rows))
                       return rows;
}


static void *vidmix_thread(void *arg)
{
	struct vidmix *mix = arg;
	uint64_t ts = 0;

	pthread_mutex_lock(&mix->mutex);

	while (mix->run) {

		unsigned n, rows, idx;
		struct le *le;
		uint64_t now;

		if (!mix->srcl.head) {
			pthread_cond_wait(&mix->cond, &mix->mutex);
			ts = 0;
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

		if (mix->clear) {
			clear_frame(mix->frame);
			mix->clear = false;
		}

		n = list_count(&mix->srcl);
		rows = calc_rows(n);

		for (le=mix->srcl.head, idx=0; le; le=le->next) {

			struct vidmix_source *src = le->data;

			source_mix(src, n, rows, idx, mix->focus);

			if (!src->focus)
				++idx;
		}

		for (le=mix->srcl.head; le; le=le->next) {

			struct vidmix_source *src = le->data;

			src->fh((uint32_t)ts * 90, mix->frame, src->arg);
		}

		ts += mix->fint;
	}

	pthread_mutex_unlock(&mix->mutex);

	return NULL;
}


/**
 * Allocate a new Video mixer
 *
 * @param mixp Pointer to allocated video mixer
 * @param sz   Size of video mixer frame
 * @param fps  Frame rate (frames per second)
 *
 * @return 0 for success, otherwise error code
 */
int vidmix_alloc(struct vidmix **mixp, const struct vidsz *sz, int fps)
{
	struct vidmix *mix;
	int err = 0;

	if (!mixp || !sz || !fps)
		return EINVAL;

	mix = mem_zalloc(sizeof(*mix), destructor);
	if (!mix)
		return ENOMEM;

	mix->fint = 1000/fps;

	err = vidframe_alloc(&mix->frame, VID_FMT_YUV420P, sz);
	if (err)
		goto out;

	clear_frame(mix->frame);

	err = pthread_mutex_init(&mix->mutex, NULL);
	if (err)
		goto out;

	err = pthread_cond_init(&mix->cond, NULL);
	if (err)
		goto out;

	mix->run = true;

	err = pthread_create(&mix->thread, NULL, vidmix_thread, mix);
	if (err) {
		mix->run = false;
		goto out;
	}

 out:
	if (err)
		mem_deref(mix);
	else
		*mixp = mix;

	return err;
}


/**
 * Put a video source in focus
 *
 * @param mix  Video mixer
 * @param fidx Frame index
 */
void vidmix_focus(struct vidmix *mix, unsigned fidx)
{
	struct le *le;
	unsigned idx;

	if (!mix)
		return;

	pthread_mutex_lock(&mix->mutex);

	for (le=mix->srcl.head, idx=1; le; le=le->next, idx++) {

		struct vidmix_source *src = le->data;

		src->focus = (idx == fidx);
	}

	mix->focus = (fidx > 0);
	mix->clear = true;

	pthread_mutex_unlock(&mix->mutex);
}


/**
 * Add a video source to the video mixer
 *
 * @param srcp Pointer to allocated video source
 * @param mix  Video mixer
 * @param fh   Mixer frame handler
 * @param arg  Handler argument
 *
 * @return 0 for success, otherwise error code
 */
int vidmix_source_add(struct vidmix_source **srcp, struct vidmix *mix,
		      vidmix_frame_h *fh, void *arg)
{
	struct vidmix_source *src;
	int err;

	if (!srcp || !mix || !fh)
		return EINVAL;

	src = mem_zalloc(sizeof(*src), source_destructor);
	if (!src)
		return ENOMEM;

	src->mix = mix;
	src->fh  = fh;
	src->arg = arg;

	err = pthread_mutex_init(&src->mutex, NULL);
	if (err)
		goto out;

	pthread_mutex_lock(&mix->mutex);
	list_append(&mix->srcl, &src->le, src);
	mix->clear = true;
	pthread_cond_signal(&mix->cond);
	pthread_mutex_unlock(&mix->mutex);

 out:
	if (err)
		mem_deref(src);
	else
		*srcp = src;

	return err;
}


/**
 * Put a video frame into the video mixer
 *
 * @param src   Video source
 * @param frame Video frame
 */
void vidmix_source_put(struct vidmix_source *src, const struct vidframe *frame)
{
	if (!src || !frame)
		return;

	pthread_mutex_lock(&src->mutex);
	/* todo: this crash if codec state is destroyed before source */
	src->frame = *frame;
	pthread_mutex_unlock(&src->mutex);
}
