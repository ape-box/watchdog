CC = gcc
FILES = src/cdog.c src/wdlib.c
OUT_EXE = wdog

default: debug

build: $(FILES)
	$(CC) -std=c99 -o $(OUT_EXE) $(FILES)

debug: $(FILES) clean
	$(CC) -std=gnu99 -pedantic-errors -g -Wall -Wextra -Werror -o $(OUT_EXE) $(FILES)

memory-analisys: debug
	valgrind -v --show-reachable=yes --leak-check=full --track-origins=yes --tool=memcheck ./$(OUT_EXE) /home/alessio/

clean:
	rm -f *.o core

rebuild: clean build
