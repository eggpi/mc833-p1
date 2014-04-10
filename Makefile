CFLAGS = -O0 -g -Wall -pedantic -std=c99 -D_GNU_SOURCE -D_XOPEN_SOURCE=800
JANSSON_LDFLAGS = `pkg-config --libs jansson`
JANSSON_CFLAGS = `pkg-config --cflags jansson`

server: server.o tcp_server.o udp_server.o db.o client.o commands.o main.o
	$(CC) $(JANSSON_LDFLAGS) -lsqlite3 $^ -o $@

commands.o: commands.c
	$(CC) $(CFLAGS) $(JANSSON_CFLAGS) $^ -c -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm *.o server
