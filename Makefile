CC = gcc
FILES = src_c/cdog.c src_c/wdlib.c
OUT_EXE = wdog

default: debug

build: $(FILES)
	$(CC) -o $(OUT_EXE) $(FILES)

debug: $(FILES)
	$(CC) -g -Wall -Werror -o $(OUT_EXE) $(FILES)

clean:
	rm -f *.o core

rebuild: clean build
