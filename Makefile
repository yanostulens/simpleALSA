COMPILER := gcc
CFLAGS := -Wall
OPTIMIZATION := -O2
DEBUG_FLAG := -g
LIBS := -lasound -lm -lsndfile -lpthread

OUTPUT := ./builds/simpleALSA.bin
OUTPUT_DEBUG := ./builds/simpleALSA_debug.bin

EXAMPLE_MAIN:= ./src/simpleALSA_example_main.c
DEBUG_MAIN := ./src/experimental_mains/simpleALSA_example_main.c
FILES := ./src/ALSAfunctions/ALSAfunctions.c ./src/simpleALSA_API/simpleALSA.c ./src/logger/logger.c

pc: $(FILES)
	mkdir -p builds
	$(COMPILER) $(FILES) $(EXAMPLE_MAIN) -o $(OUTPUT) $(CFLAGS) $(LIBS) $(OPTIMIZATION)

pc_s:
	mkdir -p builds
	$(COMPILER) $(EXAMPLE_MAIN) -o $(OUTPUT) $(CFLAGS) $(LIBS) $(OPTIMIZATION)

example: $(EXAMPLE_MAIN)
	mkdir -p builds
	$(COMPILER) $(FILES) $(EXAMPLE_MAIN) $(OUT_PC) $(CFLAGS) $(LIBS)

debug: $(FILES)
	mkdir -p builds
	$(CC_PC) $(FILES) $(DEBUG_MAIN) $(OUT_PC) $(CFLAGS) $(DEBUG_FLAG) $(LIBS) -o $(OUTPUT_DEBUG) 
	gdb $(OUTPUT_DEBUG)

run:
	./builds/simpleALSA.bin

valgrind:
	valgrind --leak-check=yes --show-leak-kinds=definite,indirect,possible --error-exitcode=1 --suppressions=./valgrind.supp ./builds/alsaPlayer2.bin

header:
	quom ./src/simpleALSA_API/simpleALSA.h ./single_header/simpleALSA.h
