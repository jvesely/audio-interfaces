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
	volatile bool running;
} playback_t ;

int data_func(jack_nframes_t nframes, void *arg)
{
	playback_t *pb = arg;
	void * buffer = jack_port_get_buffer(pb->port, nframes);
	const int ret =
	    read(pb->fd, buffer, nframes * sizeof(jack_default_audio_sample_t));
	if (ret <= 0)
		pb->running = false;
	return 0;;
}


int play(int fd, size_t offset, size_t data_size, unsigned rate,
    unsigned sample_size, unsigned channels, bool sign)
{
	if (channels != 1 || sample_size != 32 || !sign)
		return ENOTSUP;

	jack_client_t *client =
	    jack_client_open("simple player", JackNullOption, NULL);
	if (!client)
		return ENOMEM;
	playback_t pb = {.fd = fd, .running = true};
	pb.port = jack_port_register(client, "output", "32 bit float mono audio",
	    JackPortIsOutput, 0);
	if (pb.port) {
		jack_set_process_callback(client, data_func, &pb);
		jack_activate(client);
		const char **ports =
		    jack_get_ports(client, NULL, NULL,
		        JackPortIsPhysical|JackPortIsInput);
		jack_connect(client, jack_port_name(pb.port), ports[0]);
		jack_connect(client, jack_port_name(pb.port), ports[1]);
		while(pb.running)
			sleep(1);
	} else {
		printf("Failed to open port.\n");
	}
	jack_client_close(client);
	return 0;
}
