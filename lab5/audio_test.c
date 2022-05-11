#include <stdio.h>

#include "audio_util.h"

#define PDM_SAMPLE_RATE 3072000 /* 3.072MHz */
#define PCM_SAMPLE_RATE 16000 /* 16KHz */

int main(int argc, char *argv[])
{
	pcm_info_st pcm_info;
	uint8_t * pdm_buf = pcm_read_wav_file(&pcm_info, "test.pdm");

	int pdm_byte_len = pcm_info.numBytes;
	uint8_t * pcm_buf = pdm_to_pcm_s16_mono(pdm_buf, pdm_byte_len, PDM_SAMPLE_RATE, 64, &pcm_info);
	pcm_write_wav_file(pcm_buf, &pcm_info, "test.wav");

	pcm_info_st pcm_info2;
	uint8_t * pcm_buf2 = pcm_s16_mono_resample(pcm_buf, &pcm_info, PCM_SAMPLE_RATE, &pcm_info2);
	pcm_write_wav_file(pcm_buf2, &pcm_info2, "test2.wav");
	return 0;
}

