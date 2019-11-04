CXX = g++
CXXFLAGS = -std=c++1y -Wall -Wextra -fpermissive -O3 -Isrc -g
LIBFLAGS = -Llib -lBearLibTerminal.dll -lsqlite3.dll -static-libgcc -static-libstdc++ -static -mwindows -s

SOURCES=$(wildcard src/*.cpp)
SOURCES+=$(wildcard src/pcg/*.cpp)
SOURCES+=$(wildcard src/SQLiteCpp/*.cpp)
SOURCES+=$(wildcard src/jsoncpp/*.cpp)
OBJS=$(SOURCES:.cpp=.o)

duskfall: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o win64/duskfall $(LIBFLAGS) -g

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
