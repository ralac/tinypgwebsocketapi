CFLAGS=
CC=gcc
LIBS=-L./ -lpg

all: printenv tpgwsapi

install: printenv tpgwsapi tinypgwebsocketapi.py test.py tail.sh loginform.html traviewer.html
    mkdir -p dist_api
	cp $@ dist_api/
	@echo ""----------------> " $@ is in dist_api.

printenv: printenv.c
	$(CC) $(CFLAGS) -o $@ printenv.c

printenv: printenv.c
	$(CC) $(CFLAGS) -o $@ printenv.c

printenv: printenv.c
	$(CC) $(CFLAGS) -o $@ printenv.c

clean:
	rm -f *.o *.a printenv tpgwsapi tpgwsapi_test

tpgwsapi_test: tpgwsapi_test.c
	$(CC) $(CFLAGS) -o $@ printenv.c
	./tpgwsapi_test
