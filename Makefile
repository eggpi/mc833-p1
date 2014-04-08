CFLAGS = -O0 -g -Wall -pedantic -std=c99
JANSSON_CFLAGS = `pkg-config --cflags --libs jansson`

server: libserver.a db.o client.o commands.o main.o
	$(CC) $(JANSSON_CFLAGS) -lsqlite3 $^ -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

libserver.a: server.o tcp_server.o udp_server.o
	$(AR) -rcs $@ $^

clean:
	rm *.o *.a server
