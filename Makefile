COMPILER := gcc
CFLAGS := -Wall
OPTIMIZATION := -O2
LIBS := -lasound -lm -lsndfile -lpthread

OUTPUT := ./builds/simpleALSA.bin

MAIN:= ./example.c

pc: $(FILES)
	mkdir -p builds
	$(COMPILER) $(MAIN) -o $(OUTPUT) $(CFLAGS) $(LIBS) $(OPTIMIZATION) -DSA_DEBUG

run:
	./builds/simpleALSA.bin
