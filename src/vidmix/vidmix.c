/**
 * @file vidmix.c Video Mixer
 *
 * Copyright (C) 2010 Creytiv.com
 */

#define _BSD_SOURCE 1
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#ifdef USE_FFMPEG
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#endif
#include <re.h>
#include <rem_vid.h>
#include <rem_vidmix.h>


#if LIBSWSCALE_VERSION_MINOR >= 9
#define SRCSLICE_CAST (const uint8_t **)
#else
#define SRCSLICE_CAST (uint8_t **)
#endif


struct vidmix {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct list srcl;
	pthread_t thread;
	struct vidframe *frame;
	uint32_t ncols;
	uint32_t nrows;
	int fint;
	int run;
};

struct vidmix_source {
	struct le le;
#ifdef USE_FFMPEG
	struct SwsContext *sws;
#else
	void *sws;
#endif
	struct vidmix *mix;
	vidmix_frame_h *fh;
	void *arg;
	struct vidsz sz_src;
	struct vidsz sz_dst;
};


static inline int calc_rows(uint32_t n)
{
	uint32_t rows;

	for (rows=1;; rows++)
		if (n <= (rows * rows))
			return rows;
}


static inline uint32_t calc_xpos(const struct vidmix *mix, uint32_t idx)
{
	return idx % mix->ncols;
}


static inline uint32_t calc_ypos(const struct vidmix *mix, uint32_t idx)
{
	return idx / mix->nrows;
}


static void clear_frame(struct vidframe *vf)
{
	vidframe_fill(vf, 0, 0, 0);
}


static void update_frame(struct vidmix *mix, uint32_t ncols, uint32_t nrows,
			 bool clear)
{
	if (mix->ncols == ncols && mix->nrows == nrows && !clear)
		return;

	mix->ncols = ncols;
	mix->nrows = nrows;

	clear_frame(mix->frame);
}


static void destructor(void *data)
{
	struct vidmix *mix = data;

	if (mix->run == 2) {
		re_printf("waiting for vidmix thread to terminate\n");
		pthread_mutex_lock(&mix->mutex);
		mix->run = 0;
		pthread_cond_signal(&mix->cond);
		pthread_mutex_unlock(&mix->mutex);

		pthread_join(mix->thread, NULL);
		re_printf("vidmix thread joined\n");
	}

	list_flush(&mix->srcl);
	mem_deref(mix->frame);
}


static void source_destructor(void *data)
{
	struct vidmix_source *src = data;
	struct vidmix *mix = src->mix;
	uint32_t rows;

	pthread_mutex_lock(&mix->mutex);
	list_unlink(&src->le);
	pthread_mutex_unlock(&mix->mutex);

#ifdef USE_FFMPEG
	if (src->sws)
		sws_freeContext(src->sws);
#endif

	rows = calc_rows(list_count(&mix->srcl));

	update_frame(mix, rows, rows, true);
}


static void *vidmix_thread(void *arg)
{
	struct vidmix *mix = arg;
	uint64_t ts = 0;

	re_printf("vidmix thread start\n");

	pthread_mutex_lock(&mix->mutex);

	while (mix->run) {

		struct le *le;
		uint64_t now;

		if (!mix->srcl.head) {
			re_printf("vidmix thread sleep\n");
			pthread_cond_wait(&mix->cond, &mix->mutex);
			ts = 0;
			re_printf("vidmix thread wakeup\n");
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

			struct vidmix_source *src = le->data;

			src->fh((uint32_t)ts * 90, mix->frame, src->arg);
		}

		ts += mix->fint;
	}

	pthread_mutex_unlock(&mix->mutex);

	re_printf("vidmix thread stop\n");

	return NULL;
}


int vidmix_alloc(struct vidmix **mixp, const struct vidsz *sz, int fps)
{
	struct vidmix *mix;
	int err = 0;

	if (!mixp || !sz || !fps)
		return EINVAL;

	mix = mem_zalloc(sizeof(*mix), destructor);
	if (!mix)
		return ENOMEM;

	mix->fint  = 1000/fps;
	mix->ncols = 1;
	mix->nrows = 1;

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

	mix->run = 1;

	err = pthread_create(&mix->thread, NULL, vidmix_thread, mix);
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


int vidmix_source_add(struct vidmix_source **srcp, struct vidmix *mix,
		      vidmix_frame_h *fh, void *arg)
{
	struct vidmix_source *src;
	uint32_t n;

	if (!srcp || !mix || !fh)
		return EINVAL;

	src = mem_zalloc(sizeof(*src), source_destructor);
	if (!src)
		return ENOMEM;

	src->mix  = mix;
	src->fh   = fh;
	src->arg  = arg;

	pthread_mutex_lock(&mix->mutex);
	list_append(&mix->srcl, &src->le, src);
	pthread_cond_signal(&mix->cond);
	pthread_mutex_unlock(&mix->mutex);

	n = list_count(&mix->srcl);
	update_frame(mix, calc_rows(n), calc_rows(n), false);

	*srcp = src;

	return 0;
}


int vidmix_source_put(struct vidmix_source *src, const struct vidframe *frame)
{
#ifdef USE_FFMPEG
	AVPicture pict_src, pict_dst;
	uint32_t i;
	int ret;
#endif
	uint32_t x, y, idx, n;
	struct vidframe *mframe;
	struct vidmix *mix;
	struct vidsz sz;
	struct le *le;

	if (!src || !frame)
		return EINVAL;

	mix = src->mix;
	mframe = mix->frame;

	n = calc_rows(list_count(&mix->srcl));

	sz.w = mframe->size.w / n;
	sz.h = mframe->size.h / n;

	if (!vidsz_cmp(&src->sz_src, &frame->size) ||
	    !vidsz_cmp(&src->sz_dst, &sz) || !src->sws) {

#ifdef USE_FFMPEG
		if (src->sws)
			sws_freeContext(src->sws);

		src->sws = sws_getContext(frame->size.w, frame->size.h,
					  PIX_FMT_YUV420P,
					  sz.w, sz.h,
					  PIX_FMT_YUV420P,
					  SWS_BICUBIC, NULL, NULL, NULL);
		if (!src->sws)
			return ENOMEM;
#endif

		src->sz_src = frame->size;
		src->sz_dst = sz;
	}

#ifdef USE_FFMPEG
	for (i=0; i<4; i++) {
		pict_src.data[i]     = frame->data[i];
		pict_src.linesize[i] = frame->linesize[i];
	}
#endif

	/* find my place */
	for (le=mix->srcl.head, idx=0; le; le=le->next, idx++)
		if (le->data == src)
			break;

	x = calc_xpos(mix, idx) * mframe->size.w / mix->ncols;
	y = calc_ypos(mix, idx) * mframe->size.h / mix->nrows;

#ifdef USE_FFMPEG
	pict_dst.data[0] = mframe->data[0] + x   + y   * mframe->linesize[0];
	pict_dst.data[1] = mframe->data[1] + x/2 + y/4 * mframe->linesize[0];
	pict_dst.data[2] = mframe->data[2] + x/2 + y/4 * mframe->linesize[0];
	pict_dst.data[3] = NULL;
	pict_dst.linesize[0] = mframe->linesize[0];
	pict_dst.linesize[1] = mframe->linesize[1];
	pict_dst.linesize[2] = mframe->linesize[2];
	pict_dst.linesize[3] = 0;

	ret = sws_scale(src->sws, SRCSLICE_CAST pict_src.data,
			pict_src.linesize, 0, frame->size.h,
			pict_dst.data, pict_dst.linesize);
	if (ret <= 0)
		return EINVAL;
#else
	return ENOSYS;
#endif

	return 0;
}
