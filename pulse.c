#include <pulse/context.h>
#include <pulse/thread-mainloop.h>
#include <pulse/stream.h>
#include <pulse/operation.h>

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
	pa_threaded_mainloop *loop;
} buffer_t;

static void get_data(pa_stream *stream, size_t size, void *arg)
{
	printf("Reading data.\n");
	buffer_t *buffer = arg;
	if (buffer->pos + size >= buffer->end)
		size = buffer->end - buffer->pos;
	pa_stream_write(stream, buffer->pos, size, NULL, 0, 0);
	buffer->pos += size;
	if (buffer->pos == buffer->end)
		pa_threaded_mainloop_signal(buffer->loop, 0);
}

static void state_change(pa_context *context, void* arg)
{
	pa_context_state_t state = pa_context_get_state(context);
	printf("Context state(callback): %d.\n", state);
	pa_threaded_mainloop_signal(arg, 0);
}

static void stream_state_change(pa_stream *stream, void* arg)
{
	pa_context_state_t state = pa_stream_get_state(stream);
	printf("Stream state(callback): %d.\n", state);
	pa_threaded_mainloop_signal(arg, 0);
}

static void stream_drain(pa_stream *stream, int s, void* arg)
{
	pa_stream_disconnect(stream);
	printf("Disconnected stream.\n");
	pa_stream_unref(stream);
	pa_threaded_mainloop_signal(arg, 0);
}

static void context_drain(pa_context *context, void* arg)
{
	pa_context_disconnect(context);
	printf("Disconnected context.\n");
	pa_context_unref(context);
	pa_threaded_mainloop_signal(arg, 0);
}

int play(int fd, size_t offset, size_t data_size, unsigned rate,
    unsigned sample_size, unsigned channels, bool sign)
{

	/* Only 8bit samples are supported in unsigned mode */
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

	pa_context_set_state_callback(context, state_change, loop);
	{
		pa_threaded_mainloop_lock(loop);
		pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN | PA_CONTEXT_NOFAIL, NULL);
		pa_context_state_t state = pa_context_get_state(context);
		pa_threaded_mainloop_unlock(loop);
		while (state != PA_CONTEXT_READY && state != PA_CONTEXT_FAILED) {
			pa_threaded_mainloop_lock(loop);
			pa_threaded_mainloop_wait(loop);
			state = pa_context_get_state(context);
			pa_threaded_mainloop_unlock(loop);
			printf("Context state: %d.\n", state);
		}
		printf("Connected context.\n");
	}

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


	void *data = mmap(NULL, data_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!data)
		return ENOMEM;

	printf("Mmapped data.\n");

	buffer_t buffer = {
		.data = data,
		.end = data + data_size,
		.pos = data,
		.loop =  loop,
	};

	{
		pa_threaded_mainloop_lock(loop);
		pa_stream_set_write_callback(stream, get_data, &buffer);
		pa_stream_set_state_callback(stream, stream_state_change, loop);
		printf("Set write and state callbacks.\n");

		pa_stream_connect_playback(stream, NULL, NULL, 0, NULL, NULL);
		pa_stream_state_t state = pa_stream_get_state(stream);
		pa_threaded_mainloop_unlock(loop);
		while (state != PA_STREAM_READY && state != PA_STREAM_FAILED) {
			pa_threaded_mainloop_lock(loop);
			pa_threaded_mainloop_wait(loop);
			state = pa_stream_get_state(stream);
			pa_threaded_mainloop_unlock(loop);
			printf("Stream state: %d.\n", state);
		}
		printf("Connected playback.\n");
	}
	while (buffer.pos < buffer.end) {
		pa_threaded_mainloop_lock(loop);
		pa_threaded_mainloop_wait(loop);
		pa_threaded_mainloop_unlock(loop);
	}

	munmap(data, data_size);

	pa_threaded_mainloop_lock(loop);
	pa_operation *drain_op = pa_stream_drain(stream, stream_drain, loop);
	pa_operation_unref(drain_op);

	drain_op = pa_context_drain(context, context_drain, loop);
	while (pa_operation_get_state(drain_op) == PA_OPERATION_RUNNING)
		pa_threaded_mainloop_wait(loop);
	pa_operation_unref(drain_op);
	pa_threaded_mainloop_unlock(loop);

	pa_threaded_mainloop_stop(loop);
	printf("Stopped main loop.\n");
	pa_threaded_mainloop_free(loop);

	return 0;
}
