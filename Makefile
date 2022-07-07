CC_PC := gcc
CFLAGS := -Wall
LIBS := -lasound -lm -lsndfile

OUT_PC := -o ./builds/alsaPlayer2.bin

FILES := ./main.c
MAIN := 

pc: $(FILES)
	mkdir -p builds
	mkdir -p builds
	$(CC_PC) $(FILES) $(MAIN) $(OUT_PC) $(CFLAGS) $(LIBS)

run:
	./builds/alsaPlayer2.bin