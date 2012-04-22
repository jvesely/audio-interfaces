
SHARED_SOURCES=main.c wav.c

all: pa-simple pulse alsa

pa-simple: $(SHARED_SOURCES) pa-simple.c
	$(CC) -I . -std=c99 $^ -lpulse-simple -o $@

pulse: $(SHARED_SOURCES) pulse.c
	$(CC) -I . -std=c99 $^ -lpulse -o $@

alsa: $(SHARED_SOURCES) alsa.c
	$(CC) -I . -std=c99 $^ -lasound -o $@

clean:
	rm -vf *.o pa-simple pulse alsa
