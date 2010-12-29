struct vidframe;

int  rem_init(void);
void vidconv_init(void);


void vidconv_yuyv_to_yuv420p(struct vidframe *dst,
			     const struct vidframe *src);
void vidconv_yuyv_to_yuv420p_orc(struct vidframe *dst,
				 const struct vidframe *src);
void vidconv_yuyv_to_yuv420p_sws(struct vidframe *dst,
				 const struct vidframe *src);


void vidconv_rgb32_to_yuv420p(struct vidframe *dst, const struct vidframe *s);
