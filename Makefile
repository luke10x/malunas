.DEFAULT_GOAL=malunas
CFLAGS=-std=c99

vpath %.c ./src

src = malunas.c exec.c
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

behave: malunas
	behave

.PHONY: clean indent docker-alpine-build behave hotswap

docker-alpine-build: build/malunas.alpine
	docker build -t malunas:alpine -f docker/Dockerfile.alpine .
	
clean:
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
