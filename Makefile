
INCLUDEDIR = include


all: smartvio-brain smartvio-test szg_i2cwrite szg_i2cread dna-writer


smartvio-brain: src/smartvio-brain.c src/syzygy.o
	$(CC) -I $(INCLUDEDIR) -o $@ $^


smartvio-test: src/smartvio-test.cpp src/syzygy.o
	$(CXX) -std=c++11 -I $(INCLUDEDIR) -o $@ $^


szg_i2cwrite: src/i2cwrite.c
	$(CC) -I $(INCLUDEDIR) -o $@ $^


szg_i2cread: src/i2cread.c
	$(CC) -I $(INCLUDEDIR) -o $@ $^


dna-writer: src/dna-writer.cpp src/syzygy.o
	$(CXX) -std=c++11 -I $(INCLUDEDIR) -o $@ $^


src/syzygy.o: src/syzygy.c
	$(CC) -I $(INCLUDEDIR) -o $@ -c $^


