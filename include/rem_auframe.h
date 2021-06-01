/*
 * Audio frame
 */

/**
 * Defines a frame of audio samples
 */
struct auframe {
	enum aufmt fmt;      /**< Sample format (enum aufmt)        */
	void *sampv;         /**< Audio samples (must be mem_ref'd) */
	size_t sampc;        /**< Total number of audio samples     */
	uint64_t timestamp;  /**< Timestamp in AUDIO_TIMEBASE units */
	uint32_t srate;      /**< Samplerate                        */
	uint8_t ch;          /**< Channels                          */
};

void auframe_init(struct auframe *af, enum aufmt fmt, void *sampv,
		  size_t sampc, uint32_t srate, uint8_t ch);
size_t auframe_size(const struct auframe *af);
void   auframe_mute(struct auframe *af);
