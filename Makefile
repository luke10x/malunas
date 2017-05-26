.DEFAULT_GOAL=malunas
CFLAGS=-std=c99

vpath %.c ./src

src = malunas.c exec.c proxy.c addrparse.c
tests = addrparse_test.c
objects = $(patsubst %.c,obj/%.o,$(src))
test_objects = $(patsubst %.c,obj/%.o,$(src))

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

addrparse_test: obj/addrparse.o obj/addrparse_test.o
	echo $<
	$(CC) $(CFLAGS) $^ -o $@

lib-src:
	mkdir lib-src
	cd lib-src && apt-get source glibc

.PHONY: clean indent hotswap

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
