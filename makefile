CC = g++

SOURCES = $(wildcard *cpp)
objects = $(SOURCES:.cpp=)
OPTS = -std=c++11 -w

all: $(objects)
	echo $(objects)

$(objects): %:%.cpp
	$(CC)  $^ -o $@ $(OPTS) `pkg-config opencv4 --cflags` `pkg-config opencv4 --libs` 

clean:
	rm -f *.o $(objects)
