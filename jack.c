#include <jack/jack.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "play.h"

typedef struct playback {
	int fd;
	jack_port_t *port;
} playback_t ;

int data_func(jack_nframes_t nframes, void *arg)
{
	playback_t *pb = arg;
	void * buffer = jack_port_get_buffer(pb->port, nframes);
	int ret = read(pb->fd, buffer, nframes * sizeof(jack_default_audio_sample_t));
	return (ret > 0) ? 0 : ret;
}


int play(int fd, size_t offset, size_t data_size, unsigned rate,
    unsigned sample_size, unsigned channels, bool sign)
{
	jack_status_t status;
	jack_client_t *client =
	    jack_client_open("simple player", JackNullOption, &status);
	if (!client)
		return ENOMEM;
	printf("Opened jack client with status: %x.\n", status);
	playback_t pb = {.fd = fd};
	pb.port = jack_port_register(client, "output", "32 bit float mono audio",
	    JackPortIsOutput, 0);
	if (pb.port) {
		jack_set_process_callback(client, data_func, &pb);
		jack_activate(client);
		// wait here
		sleep(10);
	} else {
		printf("Failed to open port.\n");
	}
	jack_client_close(client);
	return 0;
}
