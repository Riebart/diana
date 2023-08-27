CXX=g++
INCLUDE_DIR=-I. -I./include -I./lib/include
CFLAGS=-O2 -Wall -g -std=c++17
FILES=lib/utility.cpp lib/vector.cpp lib/physics.cpp lib/universe.cpp lib/MIMOServer.cpp lib/messaging.cpp
EXT_LIBS=
EXT_ST_LIBS=
SHELL=/bin/bash

all: unisim-bin

clean:
	rm bin/unisim

unisim-bin:
	make unisim-bin-static

unisim-bin-static:
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) src/unisim.cpp -o bin/unisim
