#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "wav.h"
#include "play.h"

static void print_usage(const char *name)
{
	printf("Usage %s filename\n\t filename is a path to wav file.\n", name);
}


int main(int argc, char **argv)
{
	if (argc != 2) {
		print_usage(argv[0]);
		return 1;
	}

	const int fd = open(argv[1], O_RDONLY);
	wave_header_t header = {};
	ssize_t bytes = read(fd, &header, sizeof(header));

	if (bytes != sizeof(header)) {
		printf("Invalid header.\n");
		return 1;
	}

	size_t data_size = 0;
	unsigned sampling_rate = 0;
	unsigned sample_size = 0;
	unsigned channels = 0;
	bool sign = false;
	const char *error = NULL;
	int ret = wav_parse_header(&header, &data_size, &sampling_rate,
	    &sample_size, &channels, &sign, &error);
	if (ret) {
		printf("Failed to parse wav header: %s.\n", error);
		return 1;
	}

	play(fd, sizeof(header), data_size, sampling_rate, sample_size,
	    channels, sign);
}
