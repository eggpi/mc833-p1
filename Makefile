CFLAGS = -O0 -g -Wall -Werror -pedantic -std=c99 -D_GNU_SOURCE -D_XOPEN_SOURCE=800 -D_DARWIN_C_SOURCE
IP = localhost
PORT = 8989
NOW := $(shell date +"%s")

export PKG_CONFIG_PATH=$(PWD)/lib/jansson-2.6/build/lib/pkgconfig
export JANSSON_CFLAGS=`PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags jansson`
export JANSSON_LDFLAGS=`PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs jansson` -Wl,-rpath $(PWD)/lib/jansson-2.6/build/lib/

all: server client places.sqlite3

server: server.o tcp_server.o udp_server.o db.o client_class.o commands.o main.o libjansson
	$(CC) $(filter-out libjansson, $^) $(CFLAGS) $(JANSSON_LDFLAGS) -lsqlite3 -o $@

places.sqlite3: places.sql
	sqlite3 $@ < $^

client: client.o
	$(CC) $^ $(CFLAGS) $(JANSSON_LDFLAGS) -o $@

client.o: client.c
		$(CC) $(CFLAGS) $(JANSSON_CFLAGS) $^ -c -o $@

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
	rm -f *.o server client places.sqlite3

.PHONY: test_tcp
test_tcp: client speed_test.txt
	./client $(IP) $(PORT) < speed_test.txt | grep 'elapsed' > $@_$(NOW).txt ;\
	awk '{ sum += $$6 } END { print "Total: ", sum }' $@_$(NOW).txt >> $@_$(NOW).txt

.PHONY: test_udp
test_udp: client speed_test.txt
	./client $(IP) $(PORT) u < speed_test.txt | grep 'elapsed' > $@_$(NOW).txt ;\
	awk '{ sum += $$6 } END { print "Total: ", sum }' $@_$(NOW).txt >> $@_$(NOW).txt
