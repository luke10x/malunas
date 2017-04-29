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
	$(CC) $(CFLAGS) -static $^ -o $@

behave: malunas
	behave

.PHONY: clean indent docker-build docker-dist behave hotswap

docker-build:
	@mkdir -p ./build
	docker build -t malunas-build -f Dockerfile.build .
	docker run malunas-build tar -c ./malunas | tar x

docker-dist: docker-build
	docker build -t malunas -f Dockerfile.malunas .
	
clean:
	@rm -f obj/*.o
	@rm -f malunas 
	@rm -f src/*~

indent:
	indent -kr -ts4 -nut -l80 src/*.c
	@rm -f src/*~

hotswap:
	./scripts/hotswap.sh
