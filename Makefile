CFLAGS = -O2 -Wall -pedantic -std=c99

server: libserver.a client.o commands.o main.o
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

libserver.a: server.o tcp_server.o udp_server.o
	$(AR) -rcs $@ $^

clean:
	rm *.o *.a server
