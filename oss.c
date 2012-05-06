#include <sys/soundcard.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "play.h"


int play(int fd, size_t offset, size_t data_size, unsigned rate,
    unsigned sample_size, unsigned channels, bool sign)
{

	/* Only 8bit samles are supported in unsigned mode */
	if (!sign && sample_size != 8)
		return ENOTSUP;

	int dev = open("/dev/dsp", O_WRONLY, 0);
	if (dev == -1) {
		return errno;
	}

	int ioctl_arg = 0;

	switch(sample_size)
	{
	case 8:  // 8bit samples must be unsigned
		ioctl_arg = AFMT_U8; break;
	case 16: //assume native endian
		ioctl_arg = AFMT_S16_NE; break;
// OSS on my machine does not support 24/32 bit samples.
//	case 24:
//		ioctl_arg = AFMT_S24_NE; break;
//	case 32: //assume integer
//		ioctl_arg = AFMT_S32_NE; break;
	default:
		return ENOTSUP;
	}

	ioctl(dev, SNDCTL_DSP_SETFMT, &ioctl_arg);

	ioctl_arg = channels;
	ioctl(dev, SNDCTL_DSP_CHANNELS, &ioctl_arg);

	ioctl_arg = rate;
	ioctl(dev, SNDCTL_DSP_SPEED, &ioctl_arg);


	const size_t buffer_size = 4096;

	void *buffer = malloc(buffer_size);
	assert(buffer);
	printf("allocated buffer: %p %zu.\n", buffer, buffer_size);

	size_t size = 0;
	while ((size = read(fd, buffer, buffer_size)) > 0) {
		write(dev, buffer, size);
	}
	return 0;
}
