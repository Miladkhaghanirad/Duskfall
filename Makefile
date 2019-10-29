CXX = g++
CXXFLAGS = -std=c++1y -Wall -Wextra -pedantic -O3 -I../src -g
LIBFLAGS = -Llib -lmingw32 -lSDL2main -lSDL2.dll -lSDL2_image.dll -ljpeg -mwindows -s

SOURCES=$(wildcard src/*.cpp)
OBJS=$(SOURCES:.cpp=.o)

duskfall: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o win64/duskfall $(LIBFLAGS) -g

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
