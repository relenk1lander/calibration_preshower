CC = g++
CFLAGS = -Wall -Os -pedantic-errors `root-config --cflags`
LDFLAGS = `root-config --glibs` -lSpectrum
INCLUDE = -I ./src/
PROFILE = #-pg --coverage

TARGET = main

OBJECTS = $(patsubst %.cpp, %.o, $(shell find ./src/ -type f -name '*.cpp'))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(PROFILE) $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(INCLUDE) -c $(PROFILE) $(CFLAGS) $^ -o $@

clean:
	rm $(TARGET) $(OBJECTS)
#	rm -r ./Data
#	rm massif.out.* gmon.out

run:
	LD_LIBRARY_PATH=/usr/local/Cellar/root/6.22.08_2/lib/root ./main

