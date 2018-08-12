#
# kill $(ps -ef | grep tpgwsapi | awk '{print $2}') && make install
#

CFLAGS=-D_GNU_SOURCE
CC=gcc
LIBS=-L./ -lpq -lfcgi
DEST_DIR=dist_app/api

all: printenv tpgwsapi

install: src/tpgwsapi src/printenv src/tail.sh src/tinypgwebsocketapi.py
	mkdir -p $(DEST_DIR)
	cp src/tpgwsapi src/tpgwsapi-fcgi src/printenv src/tail.sh src/tinypgwebsocketapi.py $(DEST_DIR)/

uninstall:
	rm -rf $(DEST_DIR)

printenv: src/printenv.c
	cd src && $(CC) $(CFLAGS) -o $@ printenv.c

tpgwsapi: src/tpgwsapi.c src/tpgwsapi.h
	cd src && $(CC) $(CFLAGS) -o $@ tpgwsapi.c $(LIBS)
	cd src && $(CC) $(CFLAGS) -o $@-fcgi tpgwsapi.c $(LIBS) -DTPGWSAPI_FCGI

clean:
	rm -f src/printenv src/tpgwsapi src/tpgwsapi-fcgi
