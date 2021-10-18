CC      = g++
CPPFLAGS= -O3 -c -std=c++11
LDFLAGS = -static -O3 -m64
SOURCES = main.cpp graph.cpp sort.cpp cachefunc.cpp 
OBJECTS = $(SOURCES:.cpp=.o)

all: $(SOURCES) br

br : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o : 
	$(CC) $(CPPFLAGS) $< -o $@

clean:
	rm -f *.o br

