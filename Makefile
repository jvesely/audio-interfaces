
SHARED_SOURCES=main.c wav.c

all: pa-simple pulse

pa-simple: $(SHARED_SOURCES) pa-simple.c
	$(CC) -I . -std=c99 $^ -lpulse-simple -o $@

pulse: $(SHARED_SOURCES) pulse.c
	$(CC) -I . -std=c99 $^ -lpulse -o $@

clean:
	rm -vf *.o pa-simple pulse
