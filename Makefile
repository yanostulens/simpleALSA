CC_PC := gcc
CFLAGS := -Wall -O2
LIBS := -lasound -lm -lsndfile -lpthread

OUT_PC := -o ./builds/alsaPlayer2.bin

MAIN := ./simpleALSA_main.c
EXAMPLE_MAIN:= ./main.c
FILES := ./ALSAfunctions.c ./simpleALSA.c ./logger.c
LOG_FLAGS := 

pc: $(FILES)
	mkdir -p builds
	mkdir -p builds
	$(CC_PC) $(FILES) $(MAIN) $(OUT_PC) $(CFLAGS) $(LIBS)

example: $(EXAMPLE_MAIN)
	mkdir -p builds
	mkdir -p builds
	$(CC_PC) $(FILES) $(EXAMPLE_MAIN) $(OUT_PC) $(CFLAGS) $(LIBS)

test_poll: ./main_with_polldescr.c
	mkdir -p builds
	mkdir -p builds
	$(CC_PC) ./main_with_polldescr.c $(OUT_PC) $(CFLAGS) $(LIBS)

run:
	./builds/alsaPlayer2.bin

debug: $(FILES)
	mkdir -p builds
	mkdir -p builds
	$(CC_PC) $(FILES) $(MAIN) $(OUT_PC) $(CFLAGS) -g $(LIBS) -o debugging
	gdb debugging
