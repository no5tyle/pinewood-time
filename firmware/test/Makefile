# Unit tests for the timer core, built for this computer: make -C firmware/test
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined
test: test_core
	./test_core
test_core: test_core.c ../src/timer_core.c ../src/timer_core.h
	$(CC) $(CFLAGS) -o $@ test_core.c ../src/timer_core.c
clean:
	rm -f test_core
.PHONY: test clean
