CXX=g++
INCLUDE_DIR=-I. -I./include -I./lib/include -I./src/include
_CFLAGS=${CFLAGS} -O2 -Wall -g -std=c++17
# _CFLAGS=${CFLAGS} -O2 -Wall -g -std=c++17 -march=native -mavx -mavx2 -ftree-vectorize
FILES=lib/utility.cpp lib/vector.cpp lib/physics.cpp lib/universe.cpp lib/MIMOServer.cpp lib/messaging.cpp
EXT_LIBS=
EXT_ST_LIBS=
SHELL=/bin/bash
# Because we're using unannotated lightweight tags to refer to some useful historical commits,
# and those have been merged in with merge commits, not squash-and-merges, we don't want to use
# `--tags` here, or it'll pick those up. Without `--tags`, it only picks up annotated tags.
GIT_VERSION := "\"$(shell git describe --abbrev=8 --dirty --always)\""
GIT_BRANCH := "\"$(shell git rev-parse --abbrev-ref HEAD)\""
GIT_COMMIT_DATE := "\"$(shell TZ=UTC0 git log --date=iso8601-strict-local --pretty=%ad -n1)\""
BUILD_DATE := "\"$(shell date -u +"%FT%T%:z")\""

all:
	make unisim-bin-all
	make tests

clean:
	rm bin/unisim

tests:
	make test-bson
	make test-argparse
	make test-packing

test-bson:
	$(CXX) $(_CFLAGS) $(INCLUDE_DIR) $(EXT_LIBS) $(EXT_ST_LIBS) test/test-bson.cpp -o bin/test-bson

test-argparse:
	$(CXX) $(_CFLAGS) $(INCLUDE_DIR) $(EXT_LIBS) $(EXT_ST_LIBS) test/test-argparse.cpp -o bin/test-argparse

test-packing:
	$(CXX) $(_CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) test/test-packing.cpp -o bin/test-packing

universe-cli-args-header:
	bash build_args.sh

unisim-version-header:
	echo "#ifndef __VERSION_HPP" > src/__version.hpp
	echo "#define __VERSION_HPP" >> src/__version.hpp
	echo "#define GIT_VERSION $(GIT_VERSION)" >> src/__version.hpp
	echo "#define GIT_BRANCH $(GIT_BRANCH)" >> src/__version.hpp
	echo "#define GIT_COMMIT_DATE $(GIT_COMMIT_DATE)" >> src/__version.hpp
	echo "#define BUILD_DATE $(BUILD_DATE)" >> src/__version.hpp
	echo "#endif" >> src/__version.hpp

unisim-headers:
	make unisim-version-header
	make universe-cli-args-header

unisim-bin-all:
	make unisim-headers
	make unisim-bin-static

unisim-bin:
	make unisim-bin-static

unisim-bin-static:
	mkdir -p bin
	$(CXX) $(_CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) \
		src/unisim.cpp -o bin/unisim
