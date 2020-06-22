all: clean createBin build
.PHONY: clean build producer

CFLAGS = -Wall
CC := gcc
LDLIBS = -lrt -lpthread 

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
IDIR := include

MFLAG = -lm

clean:	
	@rm -rf $(BIN_DIR)

createBin:
	@mkdir $(BIN_DIR)

P_SRC = $(SRC_DIR)/producer.c $(SRC_DIR)/randomGenerators.c
producer: 
	$(CC) $(P_SRC) -o $(BIN_DIR)/$@ $(CFLAGS) $(LDLIBS) $(MFLAG)

C_SRC = $(SRC_DIR)/consumer.c $(SRC_DIR)/randomGenerators.c
consumer: 
	$(CC) $(C_SRC) -o $(BIN_DIR)/$@ $(CFLAGS) $(LDLIBS) $(MFLAG)

I_SRC = $(SRC_DIR)/initializer.c
initializer: 
	$(CC) $(I_SRC) -o $(BIN_DIR)/$@ $(CFLAGS) $(LDLIBS)

F_SRC = $(SRC_DIR)/finalizer.c 
finalizer: 
	$(CC) $(F_SRC) -o $(BIN_DIR)/$@ $(CFLAGS) $(LDLIBS)

build: producer consumer finalizer initializer
