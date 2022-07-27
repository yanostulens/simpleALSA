C_COMPILER := gcc
CPP_COMPILER := g++
CFLAGS := -Wall
OPTIMIZATION := -O2
DEBUG_FLAGS := -g -DSA_DEBUG
LIBS := -lasound -lm -lsndfile -lpthread

OUTPUT := ./builds/simpleALSA.bin

EXAMPLE_MAIN:= ./example.c
TEST_MAIN := ./tests/test_main.c
TEST_AUDIO_FILE := ./audioFiles/afraid.wav

pc: $(FILES)
	mkdir -p builds
	$(C_COMPILER) $(TEST_MAIN) -o $(OUTPUT) $(CFLAGS) $(LIBS) $(OPTIMIZATION) $(DEBUG)

pc_cpp: $(FILES)
	mkdir -p builds
	$(CPP_COMPILER) $(TEST_MAIN) -o $(OUTPUT) $(CFLAGS) $(LIBS) $(OPTIMIZATION) $(DEBUG)

example: $(FILES)
	mkdir -p builds
	$(C_COMPILER) $(EXAMPLE_MAIN) -o $(OUTPUT) $(CFLAGS) $(LIBS) $(OPTIMIZATION) $(DEBUG)

debug:
	gdb $(OUTPUT_DEBUG) $(TEST_AUDIO_FILE)

run:
	$(OUTPUT) $(TEST_AUDIO_FILE)

valgrind:
	valgrind --leak-check=yes --show-leak-kinds=definite,indirect,possible --error-exitcode=1 --suppressions=./valgrind.supp ./builds/simpleALSA.bin $(TEST_AUDIO_FILE)

