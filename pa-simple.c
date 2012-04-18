#include <pulse/simple.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdio.h>

#include "play.h"


int play(int fd, size_t offset, size_t data_size, unsigned rate,
    unsigned sample_size, unsigned channels, bool sign)
{
	pa_sample_spec ss = {
		.format = PA_SAMPLE_S16NE,
		.channels = channels,
		.rate = rate,
	};


	pa_simple *s =
	    pa_simple_new(NULL,               // Use the default server.
			  "TEST APP",           // Our application's name.
			  PA_STREAM_PLAYBACK,
			  NULL,               // Use the default device.
			  "Test",            // Description of our stream.
			  &ss,                // Our sample format.
			  NULL,               // Use default channel map
			  NULL,               // Use default buffering attributes.
			  NULL               // Ignore error code.
			  );
	assert(s);
	void *audio = mmap(NULL, data_size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert(audio);

	pa_simple_write(s, audio, data_size, NULL);
	pa_simple_drain(s, NULL);

	munmap(audio, data_size);
	pa_simple_free(s);
	return 0;
}
