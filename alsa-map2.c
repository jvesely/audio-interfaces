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
	    SND_PCM_ACCESS_MMAP_INTERLEAVED);
	snd_pcm_hw_params_set_channels(handle, params, channels);
	snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);
	snd_pcm_hw_params_set_format(handle, params, format);

	dir = 0;
	snd_pcm_uframes_t frames = 4096;
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

	bool started = false;

	while (read(fd, buffer, buffer_size) > 0) {
		/* Get available buffer size */
		snd_pcm_sframes_t avail_frames = snd_pcm_avail_update(handle);
		while (avail_frames < frames) {
			if (avail_frames < 0) {
				printf("Avail frames fail: %s.\n",
				    snd_strerror(avail_frames));
				return 1;
			}
			if(!started) {
				printf("Starting playback.\n");
				const int ret = snd_pcm_start(handle);
				if (ret < 0) {
					printf("snd_pcm_start failed.\n");
					return 1;
				}
				started = true;
			} else {
				printf("waiting.\n");
				const int ret = snd_pcm_wait(handle, -1);
				if (ret < 0) {
					printf("snd_pcm_wait failed.\n");
					return 1;
				}
			}
			avail_frames = snd_pcm_avail_update(handle);
		}
		printf("Available frames: %ld.\n", avail_frames);
		snd_pcm_uframes_t off = 0;
		snd_pcm_uframes_t req_frames = frames;
		/* Request access to buffer */
		const snd_pcm_channel_area_t *areas = NULL;
		int ret = snd_pcm_mmap_begin(handle, &areas, &off, &req_frames);
		if (ret < 0) {
			printf("snd_pcm_mmap_begin: %s.\n", snd_strerror(ret));
			return 1;
		}
		/* Manipulate buffer */
		assert(req_frames == frames);
		printf("Got access to buffer: %lu(%lu).\n", off, req_frames);

		for (unsigned i = 0; i < channels; ++i) {
			printf("Channel %u area: ptr: %p, first %u, step %u.\n",
			    i, areas[i].addr, areas[i].first / 8,
			    areas[i].step / 8);
		}
		printf("Copying data: %p(%zu) => %p(%zu).\n",
		    buffer, buffer_size, areas[0].addr,
		    frames * channels * sample_size /8);

		memcpy(areas[0].addr + off * areas[0].step / 8, buffer, buffer_size);

		/* Send new data to device */
		ret = snd_pcm_mmap_commit(handle, off, req_frames);
		if (ret != req_frames) {
			printf("snd_pcm_mmap_commit: %s.\n", snd_strerror(ret));
			return 1;
		}
		memset(buffer, 0, buffer_size);
	}
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	return ret;
}
