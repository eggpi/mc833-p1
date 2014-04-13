CFLAGS = -O0 -g -Wall -Werror -pedantic -std=c99 -D_GNU_SOURCE -D_XOPEN_SOURCE=800 -D_DARWIN_C_SOURCE

export PKG_CONFIG_PATH=$(PWD)/lib/jansson-2.6/build/lib/pkgconfig
export JANSSON_CFLAGS=`PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags jansson`
export JANSSON_LDFLAGS=`PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs jansson` -Wl,-rpath $(PWD)/lib/jansson-2.6/build/lib/

server: server.o tcp_server.o udp_server.o db.o client.o commands.o main.o libjansson
	$(CC) $(CFLAGS) $(JANSSON_LDFLAGS) -lsqlite3 $(filter-out libjansson, $^) -o $@

commands.o: commands.c libjansson
	$(CC) $(CFLAGS) $(JANSSON_CFLAGS) $(filter-out libjansson, $^) -c -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

libjansson: lib/jansson-2.6/.built

lib/jansson-2.6/.built:
	cd lib/jansson-2.6/; \
	./configure --prefix=$(PWD)/lib/jansson-2.6/build/; \
	$(MAKE) install;
	touch $@

clean:
	rm *.o server
