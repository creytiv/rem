#include <string.h>
#include <math.h>
#include <re.h>
#include <rem.h>


/*
 * TODO: what is the best way of calculating the size of the main frame ?
 */


/*
 * Video Mixer with alpha-blending
 *
 *   .---------------------.
 *   |          |          |
 *   |  index0  |  index1  |
 *   |          |          |
 *   |----------+----------|
 *   |          |          |
 *   |  index2  |  index3  |
 *   |          |          |
 *   '---------------------'
 *
 */


/** YUV420p video mixer */
struct vidmix {
	struct list sourcel;  /**< List of video sources */
	struct vidsz slotsz;
	struct vidframe *frame;
	int maxfps;
	int ncols;
	int nrows;
	vidmix_frame_h *fh;
	void *arg;
};

struct vidmix_source {
	struct le le;
	struct vidmix *vm;
	struct tmr tmr;
	int index;
	double alpha;
	bool terminated;
};


/**
 * Calculate how many rows is needed to fit N slots
 *
 * N   cols  rows    slots:
 *
 * 1    1     1        1
 * 2    2     2        4
 * 3    2     2        4
 * 4    2     2        4
 * 5    3     3        9
 */
static inline int calc_rows(int n)
{
	int rows;

	for (rows = 1; rows < 10; rows++) {

		if (n <= (rows * rows))
			return rows;
	}

	return -1;
}


static inline int calc_xpos(const struct vidmix *vm, int i)
{
	return i % vm->ncols;
}


static inline int calc_ypos(const struct vidmix *vm, int i)
{
	return i / vm->nrows;
}


static void frame_fill(struct vidframe *vf, uint32_t r, uint32_t g, uint32_t b)
{
#define PREC 8
#define RGB2Y(r, g, b) (((66 * (r) + 129 * (g) + 25 * (b)) >> PREC) + 16)
#define RGB2U(r, g, b) (((-37 * (r) + -73 * (g) + 112 * (b)) >> PREC) + 128)
#define RGB2V(r, g, b) (((112 * (r) + -93 * (g) + -17 * (b)) >> PREC) + 128)

	/* Y */
	memset(vf->data[0], RGB2Y(r, g, b), vf->size.h * vf->linesize[0]);
	/* U */
	memset(vf->data[1], RGB2U(r, g, b),(vf->size.h/2) * vf->linesize[1]);
	/* V */
	memset(vf->data[2], RGB2V(r, g, b), (vf->size.h/2) * vf->linesize[2]);

	vf->valid = true;
}


static void clear_frame(struct vidframe *vf)
{
	frame_fill(vf, 0, 0, 0);
}


static struct vidframe *frame_alloc(const struct vidsz *sz)
{
	struct vidframe *f;

	f = mem_zalloc(sizeof(*f) + (sz->w * sz->h * 2), NULL);
	if (!f)
		return NULL;

	re_printf("vidframe_alloc: %d x %d ---> %u bytes\n",
		  sz->w, sz->h, sizeof(*f) + (sz->w * sz->h * 2));

	f->linesize[0] = sz->w;
	f->linesize[1] = sz->w / 2;
	f->linesize[2] = sz->w / 2;

	f->data[0] = sizeof(*f) + (uint8_t *)f;
	f->data[1] = f->data[0] + f->linesize[0] * sz->h;
	f->data[2] = f->data[1] + f->linesize[1] * sz->h;

	f->size = *sz;
	f->valid = true;

	return f;
}


static void update_frame(struct vidmix *vm, int ncols, int nrows)
{
	struct vidsz sz;

	if (vm->ncols == ncols && vm->nrows == nrows)
		return;

	vm->ncols = ncols;
	vm->nrows = nrows;

	sz.w = vm->slotsz.w * vm->ncols;
	sz.h = vm->slotsz.h * vm->nrows;

	mem_deref(vm->frame);
	vm->frame = frame_alloc(&sz);
	clear_frame(vm->frame);
}


static void destructor(void *data)
{
	struct vidmix *vm = data;
	struct le *le = vm->sourcel.head;

	while (le) {
		struct vidmix_source *src = le->data;

		le = le->next;

		src->terminated = true;
		mem_deref(src);
	}

	mem_deref(vm->frame);
}


static void tmr_handler(void *arg)
{
	struct vidmix_source *src = arg;
	struct vidmix *vm = src->vm;

	if (src->alpha > 0.0f) {
		int slot_width = vm->frame->size.w / vm->ncols;
		int slot_height = vm->frame->size.h / vm->nrows;
		int xpos = calc_xpos(vm, src->index);
		int ypos = calc_ypos(vm, src->index);

		/* Fade-out frame */
		src->alpha -= 0.01;
		vidframe_alphablend_area(vm->frame,
					 xpos * slot_width,
					 ypos * slot_height,
					 slot_width,
					 slot_height,
					 src->alpha);

		tmr_start(&src->tmr, 50, tmr_handler, src);
	}
	else {
		mem_deref(src);
	}
}


static void source_destructor(void *data)
{
	struct vidmix_source *src = data;
	struct vidmix *vm = src->vm;
	int n;

	re_printf("vidmix: source removed: index=%d\n", src->index);

	if (!src->terminated) {
		src->terminated = true;

		mem_ref(src);

		tmr_start(&src->tmr, 1, tmr_handler, src);
		return;
	}

	list_unlink(&src->le);
	tmr_cancel(&src->tmr);

	n = list_count(&vm->sourcel);
	update_frame(vm, calc_rows(n), calc_rows(n));
}


int vidmix_alloc(struct vidmix **vmp, const struct vidsz *sz, int maxfps,
		 vidmix_frame_h *fh, void *arg)
{
	struct vidmix *vm;

	if (!vmp || !sz || !fh)
		return EINVAL;

	vm = mem_zalloc(sizeof(*vm), destructor);
	if (!vm)
		return ENOMEM;

	list_init(&vm->sourcel);

	vm->slotsz = *sz;
	vm->maxfps = maxfps;
	vm->ncols = 1;
	vm->nrows = 1;
	vm->fh = fh;
	vm->arg = arg;

	vm->frame = frame_alloc(sz);
	clear_frame(vm->frame);

	*vmp = vm;

	re_printf("vidmix_alloc: %u x %u\n", sz->w, sz->h);

	return 0;
}


static struct vidmix_source *find_source(const struct vidmix *vm, int i)
{
	struct le *le;

	for (le = vm->sourcel.head; le; le = le->next) {
		struct vidmix_source *src = le->data;

		if (src->index == i)
			return src;
	}

	return NULL;
}


static int find_free_slot(const struct vidmix *vm)
{
	int i;

	for (i=0; i<(vm->ncols * vm->nrows); i++) {

		if (!find_source(vm, i))
			return i;
	}

	return -1; /* not found */
}


int vidmix_source_add(struct vidmix_source **srcp, struct vidmix *vm)
{
	struct vidmix_source *src;
	int n;

	if (!vm)
		return EINVAL;

	src = mem_zalloc(sizeof(*src), source_destructor);
	if (!src)
		return ENOMEM;

	n = 1 + list_count(&vm->sourcel);
	update_frame(vm, calc_rows(n), calc_rows(n));

	src->index = find_free_slot(vm);
	src->vm = vm;
	src->alpha = 0.0f;

	list_append(&vm->sourcel, &src->le, src);

	re_printf("vidmix source added: index=%d\n", src->index);

	if (srcp)
		*srcp = src;

	return 0;
}


int vidmix_source_put(struct vidmix_source *src, const struct vidframe *fs)
{
	struct vidmix *vm;
	int xpos, ypos;

	if (!src)
		return EINVAL;

	vm = src->vm;
	xpos = calc_xpos(vm, src->index);
	ypos = calc_ypos(vm, src->index);

	/* Fade-in frame */
	if (!src->terminated && src->alpha < 1.0f) {

		src->alpha += 0.03f;
		vidframe_alphablend_area(fs, 0, 0, fs->size.w, fs->size.h,
					 src->alpha);
	}

	vidframe_copy_offset(vm->frame, fs,
			     xpos * vm->frame->size.w / vm->ncols,
			     ypos * vm->frame->size.h / vm->nrows);

	// refresh image: todo use maxfps
	src->vm->fh(vm->frame, vm->arg);

	return 0;
}
