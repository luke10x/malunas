.DEFAULT_GOAL=easysrv
vpath %.c ./src

src = easysrv.c 
objects = $(patsubst %.c,obj/%.o,$(src))

$(objects): | obj

obj:
	mkdir -p $@

obj/%.o : %.c
	@echo $<
	$(CC) -c $< -g -o $@

easysrv: $(objects)
	echo $< 
	$(CC) -static $^ -o $@

.PHONY: clean indent docker-build docker-dist

docker-build:
	@mkdir -p ./build
	docker build -t easysrv-build -f Dockerfile.build .
	docker run easysrv-build tar -c ./easysrv | tar x

docker-dist: docker-build
	docker build -t easysrv -f Dockerfile.easysrv .
	
clean:
	@rm -f obj/*.o
	@rm -f easysrv 
	@rm -f src/*~

indent:
	indent -kr -ts4 -nut -l80 src/*.c
	@rm -f src/*~
