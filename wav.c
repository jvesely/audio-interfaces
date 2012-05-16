#include <errno.h>
#include <string.h>


#include "wav.h"

int wav_parse_header(void *file, size_t *data_size,
    unsigned *sampling_rate, unsigned *sample_size, unsigned *channels,
    bool *sign, const char **error)
{
	if (!file) {
		if (error)
			*error = "file not present";
		return EINVAL;
	}

	const wave_header_t *header = file;
	if (strncmp(header->chunk_id, CHUNK_ID, 4) != 0) {
		if (error)
			*error = "invalid chunk id";
		return EINVAL;
	}

	if (strncmp(header->format, FORMAT_STR, 4) != 0) {
		if (error)
			*error = "invalid format string";
		return EINVAL;
	}

	if (strncmp(header->subchunk1_id, SUBCHUNK1_ID, 4) != 0) {
		if (error)
			*error = "invalid subchunk1 id";
		return EINVAL;
	}

	if ((header->subchunk1_size) != PCM_SUBCHUNK1_SIZE) {
		if (error)
			*error = "invalid subchunk1 size";
		return EINVAL;
	}

	if ((header->audio_format) != FORMAT_LINEAR_PCM) {
		if (error)
			*error = "unknown format";
		return ENOTSUP;
	}

	if (strncmp(header->subchunk2_id, SUBCHUNK2_ID, 4) != 0) {
		if (error)
			*error = "invalid subchunk2 id";
		return EINVAL;
	}

	if (data_size)
		*data_size = (header->subchunk2_size);

	if (sampling_rate)
		*sampling_rate = (header->sampling_rate);
	if (sample_size)
		*sample_size = (header->sample_size);
	if (channels)
		*channels = (header->channels);
	if (sign)
		*sign = (header->sample_size) == 8 ? false : true;
	if (error)
		*error = "no error";

	return 0;
}

