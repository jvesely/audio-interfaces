#include <pulse/sample.h>
#include <pulse/simple.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>

#include "play.h"


int play(int fd, size_t offset, size_t data_size, unsigned rate,
    unsigned sample_size, unsigned channels, bool sign)
{

	/* Only 8bit samles are supported in unsigned mode */
	if (!sign && sample_size != 8)
		return ENOTSUP;

	pa_sample_spec ss = {
		.format = PA_SAMPLE_U8,
		.channels = channels,
		.rate = rate,
	};
	switch(sample_size)
	{
	case 8:  // 8bit samples must be unsigned
		ss.format = PA_SAMPLE_U8; break;
	case 16: //assume native endian
		ss.format = PA_SAMPLE_S16NE; break;
	case 24:
		ss.format = PA_SAMPLE_S24NE; break;
	case 32: //assume integer
		ss.format = PA_SAMPLE_S32NE; break;
	default:
		return ENOTSUP;
	}

	int error = 0;

	pa_simple *s =
	    pa_simple_new(NULL,               // Use the default server.
			  "TEST APP",           // Our application's name.
			  PA_STREAM_PLAYBACK,
			  NULL,               // Use the default device.
			  "Test",            // Description of our stream.
			  &ss,                // Our sample format.
			  NULL,               // Use default channel map
			  NULL,               // Use default buffering attributes.
			  &error               // E error code.
			  );
	if (!s)
		return error;

	void *data = mmap(NULL, data_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!data)
		return ENOMEM;

	char nice[100];
	printf("Playing: %s.\n", pa_sample_spec_snprint(nice, 100, &ss));
	error = 0;
	if (!pa_simple_write(s, data, data_size, &error))
		pa_simple_drain(s, &error);

	munmap(data, data_size);
	pa_simple_free(s);
	return error;
}
