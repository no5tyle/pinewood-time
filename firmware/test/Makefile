# Unit tests for the timer core and the gate links, built for this computer: make -C firmware/test
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined
test: test_core test_link
	./test_core
	./test_link
test_core: test_core.c ../src/timer_core.c ../src/timer_core.h
	$(CC) $(CFLAGS) -o $@ test_core.c ../src/timer_core.c
test_link: test_link.c ../src/link.c ../src/link.h ../src/timer_core.c ../src/timer_core.h
	$(CC) $(CFLAGS) -o $@ test_link.c ../src/link.c ../src/timer_core.c
clean:
	rm -f test_core test_link
.PHONY: test clean
