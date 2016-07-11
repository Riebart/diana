CXX=g++
INCLUDE_DIR=-I . -I ..
LIB_DIR=
CFLAGS=-O2 -Wall -g -fopenmp -std=c++11
FILES=
EXT_LIBS=-lUnitTest++
EXT_ST_LIBS=

all: vectortests

clean:
	rm vectortests

vectortests:
	$(CXX) $(CFLAGS)  vectortests.cpp $(INCLUDE_DIR) $(LIB_DIR) $(EXT_LIBS) $(EXT_ST_LIBS) -o vectortests
	
	
