all: clean build

clean:
	@rm -f childSem parentSem

build:
	gcc childSem.c -o childSem -lpthread
	gcc parentSem.c -o parentSem -lpthread