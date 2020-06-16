all: clean build

clean:
	@rm -f consumer producer finalizer initializer

CFLAGS = -Wall -lpthread
cc = cc

build:
	$(cc) consumer.c -o consumer $(CFLAGS)
	$(cc) producer.c -o producer $(CFLAGS)
	$(cc) initializer.c -o initializer $(CFLAGS)
	$(cc) finalizer.c -o finalizer $(CFLAGS)