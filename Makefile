CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -O2
OBJS = main.o detector.o ring_buffer.o low_pass.o

fallguard: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) fallguard
