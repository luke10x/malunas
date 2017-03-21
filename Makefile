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
	$(CC) $^ -o $@

popen: popen.c
	echo $< 
	$(CC) $^ -g -o $@

.PHONY: clean indent test

test: easysrv 
	sh test.sh

clean:
	@rm -f obj/*.o
	@rm -f easysrv 
	@rm -f src/*~

indent:
	indent -kr -ts4 -nut -l80 src/*.c
	@rm -f src/*~
