CXX = g++
CXXFLAGS = -std=c++1y -Wall -Wextra -fpermissive -O3 -Isrc -g
LIBFLAGS = -Llib -lmingw32 -lSDL2main -lSDL2.dll -lSDL2_image.dll -ljpeg -lsqlite3.dll -mwindows -s

SOURCES=$(wildcard src/*.cpp)
SOURCES+=$(wildcard src/lodepng/*.cpp)
SOURCES+=$(wildcard src/pcg/*.c)
SOURCES+=$(wildcard src/sdl_savejpeg/*.c)
SOURCES+=$(wildcard src/snes_ntsc/*.c)
SOURCES+=$(wildcard src/SQLiteCpp/*.cpp)
OBJS=$(SOURCES:.cpp=.o)

duskfall: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o win64/duskfall $(LIBFLAGS) -g

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
