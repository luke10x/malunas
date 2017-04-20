.DEFAULT_GOAL=tcpexecd
CFLAGS=-std=c99

vpath %.c ./src

src = tcpexecd.c 
objects = $(patsubst %.c,obj/%.o,$(src))

$(objects): | obj

obj:
	mkdir -p $@

obj/%.o : %.c
	@echo $<
	$(CC) $(CFLAGS) -c $< -g -o $@

tcpexecd: $(objects)
	echo $< 
	$(CC) $(CFLAGS) -static $^ -o $@

behave: tcpexecd
	behave

.PHONY: clean indent docker-build docker-dist behave

docker-build:
	@mkdir -p ./build
	docker build -t tcpexecd-build -f Dockerfile.build .
	docker run tcpexecd-build tar -c ./tcpexecd | tar x

docker-dist: docker-build
	docker build -t tcpexecd -f Dockerfile.tcpexecd .
	
clean:
	@rm -f obj/*.o
	@rm -f tcpexecd 
	@rm -f src/*~

indent:
	indent -kr -ts4 -nut -l80 src/*.c
	@rm -f src/*~
