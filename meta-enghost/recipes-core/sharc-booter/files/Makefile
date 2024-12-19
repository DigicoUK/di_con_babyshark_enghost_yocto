all: sharc-booter

sharc-booter: sharc-booter.o
	$(CC) $(LDFLAGS) -o $@ $^ -lgpiod

%.o: %.c
	$(CC) -c $< $(CFLAGS)

clean:
	rm -rf sharc-booter.o sharc-booter
