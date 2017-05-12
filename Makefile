.DEFAULT_GOAL=malunas
CFLAGS=-std=c99

vpath %.c ./src

src = malunas.c exec.c proxy.c
objects = $(patsubst %.c,obj/%.o,$(src))

$(objects): | obj

obj:
	mkdir -p $@

obj/%.o : %.c
	@echo $<
	$(CC) $(CFLAGS) -c $< -g -o $@

malunas: $(objects)
	echo $< 
	$(CC) $(CFLAGS) $^ -o $@

build/malunas.static: $(objects)
	echo $<
	$(CC) $(CFLAGS) -static $^ -o $@

lib-src:
	mkdir lib-src
	cd lib-src && apt-get source glibc

build/malunas.alpine:
	@mkdir -p ./build
	docker build -t malunas-build -f docker/Dockerfile.alpine.build .
	docker run malunas-build tar -c malunas.alpine | tar -x -C build

behave: malunas docker-web-start
	behave

.PHONY: clean indent docker-alpine-build behave hotswap docker-web-stop

docker-alpine-build: build/malunas.alpine
	docker build -t malunas:alpine -f docker/Dockerfile.alpine .

docker-web-start: docker-web-stop
	docker build -t malunas-webserver docker/httpd
	docker run --name webserver -d -p10080:80 malunas-webserver

docker-web-stop:
	docker stop webserver
	docker rm webserver
	
clean: docker-web-stop
	@rm -rf obj/
	@rm -rf build/
	@rm -f malunas 
	@rm -f malunas.static
	@rm -f src/*~

indent:
	indent -kr -ts4 -nut -l80 src/*.c
	@rm -f src/*~

hotswap:
	./scripts/hotswap.sh
