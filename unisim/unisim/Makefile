CXX=g++
INCLUDE_DIR=-I .
CFLAGS=-O3 -Wall -g -pthread -fopenmp -std=c++11
FILES=vector.cpp physics.cpp universe.cpp MIMOServer.cpp messaging.cpp
EXT_LIBS=
EXT_ST_LIBS=

all: unisim-bin

clean:
	rm unisim libdiana.so

diana-shared:
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) -o libdiana.so -fPIC -shared

unisim-bin:
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) unisim.cpp -o unisim
