#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

typedef struct {
	uint32_t numBytes;
	uint32_t sampleRate;
	uint16_t numChannels;
	uint16_t bitsPerSample;
} pcm_info_st;

static inline int pcm_get_sample_num(pcm_info_st *pcm_info) {
	return pcm_info->numBytes / ((pcm_info->numChannels * pcm_info->bitsPerSample) >> 3);
}

void pcm_write_wav_file(uint8_t *pcm_buf, pcm_info_st *pcm_info, const char *wavFilename);

//返回pcm_buf需要free
uint8_t * pcm_read_wav_file(pcm_info_st *pcm_info, const char *wavFilename);

//返回pcm_buf需要free
uint8_t * pcm_s16_mono_resample(uint8_t *src_buf, pcm_info_st *src_info, int dst_sample_rate, pcm_info_st *pcm_info);

//返回pcm_buf需要free
uint8_t * pdm_to_pcm_s16_mono(uint8_t *pdm_buf, int pdm_bype_len, int pdm_sample_rate, int decimation, pcm_info_st *pcm_info);

#define pcm_free_buf(buf) do {if(buf) free(buf);} while(0)
