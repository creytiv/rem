

void vidconv_packed_to_yuv420p(struct vidframe *dst,
			       const struct vidframe *src, int flags);
void vidconv_yuv420p_to_packed(struct vidframe *dst,
			       const struct vidframe *src, int flags);


void vidconv_packed_to_yuv420p_test(struct vidframe *dst,
				    const struct vidframe *src, int flags);


void vidconv_sws(struct vidconv_ctx *ctx, struct vidframe *dst,
		 const struct vidframe *src);
