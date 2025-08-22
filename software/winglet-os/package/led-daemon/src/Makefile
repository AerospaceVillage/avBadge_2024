CFLAGS := -ggdb
LDFLAGS :=
WARN := -Wall
LIBS := -lm

all: ledDaemon_winglet

ledDaemon_winglet: ledDaemon.o rgbled.o battled.o
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

ledDaemon.o: ledDaemon.c ledDaemon.h
	$(CC) $(CFLAGS) $(WARN) -c $< -o $@

rgbled.o: rgbled.c ledDaemon.h
	$(CC) $(CFLAGS) $(WARN) -c $< -o $@

battled.o: battled.c ledDaemon.h
	$(CC) $(CFLAGS) $(WARN) -c $< -o $@

clean:
	rm -f ledDaemon_winglet *.o
