#include <alsa/asoundlib.h>
#include <alloca.h>

#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include "play.h"
int play(int fd, size_t offset, size_t data_size, unsigned rate,
    unsigned sample_size, unsigned channels, bool sign)
{

	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;

	/* Open default ALSA device */
	int ret = snd_pcm_open(&handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0)
		return ret;

	/* Alloc param structure */
	snd_pcm_hw_params_alloca(&params);

	/* load some defaults */
	ret = snd_pcm_hw_params_any(handle, params);
	if (ret < 0) {
		snd_pcm_close(handle);
		return ret;
	}
	snd_pcm_format_t format;
	switch(sample_size)
	{
	case 8:
		format = sign ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_U8; break;
	case 16: //assume native endian
		format = sign ? SND_PCM_FORMAT_S16 : SND_PCM_FORMAT_U16; break;
	case 24:
		format = sign ? SND_PCM_FORMAT_S24 : SND_PCM_FORMAT_U24; break;
	case 32: //assume integer
		format = sign ? SND_PCM_FORMAT_S32 : SND_PCM_FORMAT_U32; break;
	default:
		return ENOTSUP;
	}

	int dir = 0;

	snd_pcm_hw_params_set_access(handle, params,
	    SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_channels(handle, params, channels);
	snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);
	snd_pcm_hw_params_set_format(handle, params, format);

	dir = 0;
	snd_pcm_uframes_t frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	ret = snd_pcm_hw_params(handle, params);
	if (ret < 0) {
		snd_pcm_close(handle);
		return ret;
	}

	printf("handle: %s.\n", snd_pcm_name(handle));

	snd_pcm_hw_params_get_format(params, &format);
	printf("format: %s %s.\n",
	    snd_pcm_format_name(format),
	    snd_pcm_format_description(format));

	snd_pcm_hw_params_get_rate(params, &rate, &dir);
	printf("rate: %u %d.\n", rate, dir);

	const size_t buffer_size = frames * channels * sample_size / 8;

	void *buffer = malloc(buffer_size);
	assert(buffer);
	printf("allocated buffer: %p %zu.\n", buffer, buffer_size);

	while (read(fd, buffer, buffer_size) > 0) {
		while ((ret = snd_pcm_writei(handle, buffer, frames)) == EAGAIN);
		if (ret < 0)
			printf("error writing to buffer: %s.\n",
			    snd_strerror(ret));
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	return ret;
}
