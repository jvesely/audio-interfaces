#ifndef PLAY_H
#define PLAY_H

#include <stdbool.h>
#include <stddef.h>

extern int play(int fd, size_t offset, size_t data_size, unsigned rate,
    unsigned sample_size, unsigned channels, bool sign);

#endif
