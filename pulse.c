
#ifndef SIMPLE
#include <pulse/context.h>
#include <pulse/thread-mainloop.h>
#include <pulse/stream.h>
#include <pulse/operation.h>
#else
#include <pulse/simple.h>
#endif

#include <pulse/sample.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>

#include "play.h"

typedef struct buffer {
	void *data;
	void *end;
	void *pos;
} buffer_t;

void get_data(pa_stream *stream, size_t size, void *arg)
{
	printf("Reading data.\n");
	buffer_t *buffer = arg;
	if (buffer->pos + size >= buffer->end)
		size = buffer->end - buffer->pos;
	pa_stream_write(stream, buffer->pos, size, NULL, 0, 0);
	buffer->pos += size;
}

void state_change(pa_context *context, void* arg)
{
	pa_context_state_t state = pa_context_get_state(context);
	printf("Context state: %d.\n", state);
	if (state == PA_CONTEXT_READY || state == PA_CONTEXT_FAILED)
		*(volatile bool*)arg = true;
}

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

#ifndef SIMPLE

	/* This will do the work */
	pa_threaded_mainloop *loop = pa_threaded_mainloop_new();
	if (!loop)
		return ENOMEM;

	printf("Starting mainloop.\n");
	pa_threaded_mainloop_start(loop);

	/* One per application should be enough */
	pa_context *context =
	   pa_context_new(pa_threaded_mainloop_get_api(loop), "CONTEXT");

	if (!context) {
		pa_threaded_mainloop_stop(loop);
		pa_threaded_mainloop_free(loop);
		return ENOMEM;
	}
	printf("Created context.\n");

	volatile bool context_connected = false;

	pa_context_set_state_callback(context, state_change, (void*)&context_connected);

	pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
	while (!context_connected) usleep(100);
	printf("Connected context.\n");


	/* Create stream */
	pa_stream *stream = pa_stream_new(context, "STREAM", &ss, NULL);
	if (!stream) {
		pa_context_disconnect(context);
		pa_context_unref(context);
		pa_threaded_mainloop_stop(loop);
		pa_threaded_mainloop_free(loop);
		return ENOMEM;
	}
	printf("Created stream.\n");


	int error = 0;
	void *data = mmap(NULL, data_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!data)
		return ENOMEM;

	printf("Mmapped data.\n");

	buffer_t buffer = {
		.data = data,
		.end = data + data_size,
		.pos = data,
	};

	pa_stream_set_write_callback(stream, get_data, &buffer);
	printf("Set write callback.\n");

	pa_stream_connect_playback(stream, NULL, NULL, 0, NULL, NULL);
	// TODO wait for connection active status
	printf("Connected playback.\n");
	while (buffer.pos < buffer.end)
		usleep(10);

	munmap(data, data_size);

	{
		pa_operation *drain_op = pa_stream_drain(stream, NULL, NULL);
		if (drain_op) {
			printf("Waiting for stream drain.\n");
			while (pa_operation_get_state(drain_op)
			    == PA_OPERATION_RUNNING)
				usleep(100);
			pa_operation_unref(drain_op);
		}
		pa_stream_disconnect(stream);
		printf("Disconnected stream.\n");
		pa_stream_unref(stream);
	}

	{
		pa_operation *drain_op = pa_context_drain(context, NULL, NULL);
		if (drain_op) {
			printf("Waiting for context drain.\n");
			while (pa_operation_get_state(drain_op)
			    == PA_OPERATION_RUNNING)
				usleep(100);
			pa_operation_unref(drain_op);
		}
		pa_context_disconnect(context);
		printf("Disconnected context.\n");
		pa_context_unref(context);
	}
	pa_threaded_mainloop_stop(loop);
	printf("Stopped main loop.\n");
	pa_threaded_mainloop_free(loop);
#else

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
#endif
	return error;
}
