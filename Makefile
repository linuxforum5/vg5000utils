# Simple makefile for utils

CC=gcc
WCC=i686-w64-mingw32-gcc
SRC=src
WBIN=win32
BIN=bin
INSTALL_DIR=~/.local/bin

all: vg5000wav2bin vg5000k72wav

vg5000wav2bin: $(SRC)/vg5000wav2bin.c
	$(CC) -o $(BIN)/vg5000wav2bin $(SRC)/vg5000wav2bin.c $(SRC)/lib/WavReader.c -lm
	$(WCC) -o $(WBIN)/vg5000wav2bin $(SRC)/vg5000wav2bin.c $(SRC)/lib/WavReader.c -lm

vg5000k72wav: $(SRC)/vg5000k72wav.c
	$(CC) -o $(BIN)/vg5000k72wav $(SRC)/vg5000k72wav.c -lm
	$(WCC) -o $(WBIN)/vg5000k72wav $(SRC)/vg5000k72wav.c -lm

clean:
	rm -f *~ $(SRC)/*~ 

install:
	cp $(BIN)/* $(INSTALL_DIR)/
