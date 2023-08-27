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

tests:
	make test-bson
	make test-argparse
	make test-packing

test-bson:
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(EXT_LIBS) $(EXT_ST_LIBS) test/test-bson.cpp -o bin/test-bson

test-argparse:
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(EXT_LIBS) $(EXT_ST_LIBS) test/test-argparse.cpp -o bin/test-argparse

test-packing:
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) test/test-oacking.cpp -o bin/test-packing

universe-cli-args:
	bash build_args.sh

unisim-bin:
	make universe-cli-args
	make unisim-bin-static

unisim-bin-static:
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) src/unisim.cpp -o bin/unisim
