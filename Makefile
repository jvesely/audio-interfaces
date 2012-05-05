
SHARED_SOURCES=main.c wav.c
PROJECTS=pa-simple pulse alsa alsa-map alsa-map2

all: $(PROJECTS)

pa-simple: $(SHARED_SOURCES) pa-simple.c
	$(CC) -I . -std=c99 $^ -lpulse-simple -o $@

pulse: $(SHARED_SOURCES) pulse.c
	$(CC) -I . -std=c99 $^ -lpulse -o $@

alsa: $(SHARED_SOURCES) alsa.c
	$(CC) -I . -std=c99 $^ -lasound -o $@

alsa-map: $(SHARED_SOURCES) alsa-map.c
	$(CC) -I . -std=c99 $^ -lasound -o $@

alsa-map2: $(SHARED_SOURCES) alsa-map2.c
	$(CC) -I . -std=c99 $^ -lasound -o $@

clean:
	rm -vf *.o $(PROJECTS)
