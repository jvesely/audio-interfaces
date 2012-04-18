
SHARED_SOURCES=main.c wav.c

all: pa-simple

pa-simple: $(SHARED_SOURCES) pa-simple.c
	$(CC) -I . -std=c99 $^ -lpulse-simple -o $@

clean:
	rm -vf *.o pa-simple
