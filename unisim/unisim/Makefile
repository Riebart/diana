CXX=g++
INCLUDE_DIR=-I. -I./include -I./lib/include
CFLAGS=-O2 -Wall -g -std=c++17
FILES=lib/utility.cpp lib/vector.cpp lib/physics.cpp lib/universe.cpp lib/MIMOServer.cpp lib/messaging.cpp
EXT_LIBS=
EXT_ST_LIBS=
SHELL=/bin/bash
GIT_VERSION := "\"$(shell git describe --abbrev=8 --dirty --always --tags)\""
GIT_COMMIT_DATE := "\"$(shell TZ=UTC0 git log --date=iso8601-strict-local --pretty=%ad -n1)\""
BUILD_DATE := "\"$(shell date -u +"%FT%T%:z")\""

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

universe-cli-args-header:
	bash build_args.sh

unisim-version-header:
	echo "#ifndef __VERSION_HPP" > src/__version.hpp
	echo "#define __VERSION_HPP" >> src/__version.hpp
	echo "#define GIT_VERSION $(GIT_VERSION)" >> src/__version.hpp
	echo "#define GIT_COMMIT_DATE $(GIT_COMMIT_DATE)" >> src/__version.hpp
	echo "#define BUILD_DATE $(BUILD_DATE)" >> src/__version.hpp
	echo "#endif" >> src/__version.hpp

unisim-bin-all:
	make universe-cli-args-header
	make unisim-version-header
	make unisim-bin-static

unisim-bin:
	make unisim-bin-static

unisim-bin-static:
	mkdir -p bin
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) \
		src/unisim.cpp -o bin/unisim
