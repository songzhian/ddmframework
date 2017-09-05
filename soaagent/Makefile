#compile under linux

BIN=agent
SCRIPTS=agent.sh
PACKAGE=sproxy.tar.gz
CC=gcc
INCLUDES=-I. -I/usr/include -I/usr/local/include
CFLAGS=-Wall -w -g 
LD=gcc
LIBS=-L/usr/local/lib -lpthread -lzookeeper_mt -lm -lzlog -lprotobuf-c -levent -levent_pthreads

HEADERS=$(wildcard ./*.h)
SOURCES=$(wildcard ./*.c)
OBJS=$(patsubst %.c,%.o,$(SOURCES))

.PHONY: clean $(BIN)

$(BIN):$(OBJS)
	$(LD) $^ -o $@ $(LIBS)

%.o : %.c
	$(CC) -c $< $(CFLAGS) $(INCLUDES)

clean:
	rm -f $(OBJS) $(BIN) $(PACKAGE)

package:
	rm -rf sproxy
	mkdir -m 0775 sproxy
	mkdir -m 0775 -p sproxy/conf
	mkdir -m 0775 -p sproxy/logs
	cp $(BIN) sproxy/
	cp $(SCRIPTS) sproxy/
	cp -R conf/* sproxy/conf/
	tar czvf $(PACKAGE) sproxy/
	rm -rf sproxy/

