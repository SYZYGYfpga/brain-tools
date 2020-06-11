
INCLUDEDIR = include

CFLAGS += -Wall

all: smartvio-brain szg_i2cwrite szg_i2cread sequencer-brain


smartvio-brain: src/smartvio-brain.cpp src/syzygy.o
	$(CXX) $(CFLAGS) -std=c++11 -I $(INCLUDEDIR) -o $@ $^


sequencer-brain: src/sequencer-brain.cpp
	$(CXX) $(CFLAGS) -std=c++11 -I $(INCLUDEDIR) -o $@ $^


szg_i2cwrite: src/i2cwrite.c
	$(CC) $(CFLAGS) -I $(INCLUDEDIR) -o $@ $^


szg_i2cread: src/i2cread.c
	$(CC) $(CFLAGS) -I $(INCLUDEDIR) -o $@ $^


src/syzygy.o: src/syzygy.c
	$(CC) $(CFLAGS) -I $(INCLUDEDIR) -o $@ -c $^


.PHONY: clean

clean:
	rm -f smartvio-brain szg_i2cwrite szg_i2cread src/syzygy.o
