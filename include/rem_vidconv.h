int  rem_init(void);
void vidconv_init(void);


void vidconv_yuyv_to_yuv420p(struct vidframe *dst, const struct vidframe *src);

void vidconv_yuyv_to_yuv420p_b(uint16_t *y1, int y1_linesize,
			       uint16_t *y2, int y2_linesize,
			       uint8_t *u, int u_linesize,
			       uint8_t *v, int v_linesize,
			       const uint32_t *p1, int p1_linesize,
			       const uint32_t *p2, int p2_linesize,
			       int width, int height);

void vidconv_yuyv_to_yuv420p_orc(struct vidframe *dst,
				 const struct vidframe *src);
void vidconv_yuyv_to_yuv420p_sws(struct vidframe *dst,
				 const struct vidframe *src);
