CC_PC := gcc
CFLAGS := -Wall
LIBS := -lasound -lm -lsndfile

OUT_PC := -o ./builds/alsaPlayer2.bin

MAIN := ./sipleALSA_main.c
EXAMPLE_MAIN:= ./main.c
FILES := ./ALSAfunctions.c ./simpleALSA.c

pc: $(FILES)
	mkdir -p builds
	mkdir -p builds
	$(CC_PC) $(FILES) $(MAIN) $(OUT_PC) $(CFLAGS) $(LIBS)

test_poll: ./main_with_polldescr.c
	mkdir -p builds
	mkdir -p builds
	$(CC_PC) ./main_with_polldescr.c $(OUT_PC) $(CFLAGS) $(LIBS)

run:
	./builds/alsaPlayer2.bin
