COMPILER := gcc
CFLAGS := -Wall
OPTIMIZATION := -O2
LIBS := -lasound -lsndfile -lpthread

OUTPUT := ./builds/simpleALSA.bin

MAIN:= ./example.c

pc: $(FILES)
	mkdir -p builds
	$(COMPILER) $(MAIN) -o $(OUTPUT) $(CFLAGS) $(LIBS) $(OPTIMIZATION)

run:
	./builds/simpleALSA.bin
