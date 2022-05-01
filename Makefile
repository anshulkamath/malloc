FLAGS = -g -Wall -Wextra -Wno-deprecated-declarations -D DEBUG=$(DBG)
DBG=0

all: malloc

test: DBG=1
test: test-malloc.c malloc.o
	gcc $(FLAGS) -c $< -o test-malloc.o
	gcc -o test-malloc test-malloc.o malloc.o

malloc: malloc.o
	gcc -o $@ $^

malloc.o: malloc.c
	gcc $(FLAGS) -c $< -o $@

clean:
	rm -f malloc malloc.o test-malloc test-malloc.o
