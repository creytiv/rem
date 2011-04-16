/* DSP routines */


static inline uint8_t saturate_u8(int a)
{
	return ((a > 255) ? 255 : ((a < 0) ? 0 : a));
}


static inline int16_t saturate_s16(int32_t a)
{
	if (a > INT16_MAX)
		return INT16_MAX;
	else if (a < INT16_MIN)
		return INT16_MIN;
	else
		return a;
}


#ifdef __arm__
static inline int16_t sadd16(int a, int b)
{
	__asm__ __volatile__ ("add %0, %1, %2"   "\n\t"
			      "ssat %0, #16, %0" "\n\t"
			      :"+r"(a)
			      :"r"(a), "r"(b)
			      );
	return a;
}
#else
static inline int16_t sadd16(int a, int b)
{
	int c = a + b;

	if (c > 32767)
		return 32767;
	else if (c < -32768)
		return -32768;
	else
		return c;
}
#endif
