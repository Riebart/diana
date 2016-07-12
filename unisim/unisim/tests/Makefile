CXX=g++
INCLUDE_DIR=-I . -I ..
LIB_DIR=-L..
CFLAGS=-O0 -Wall -g -fopenmp -std=c++11
USIM_FILES=../vector.cpp ../physics.cpp ../universe.cpp ../MIMOServer.cpp ../messaging.cpp
EXT_LIBS=-lUnitTest++
EXT_ST_LIBS=

all: vectortests physicstests

clean:
	rm vectortests physicstests

vectortests: vectortests.cpp ../vector.hpp ../vector.cpp
	$(CXX) $(CFLAGS) vectortests.cpp $(INCLUDE_DIR) $(LIB_DIR) $(EXT_LIBS) $(EXT_ST_LIBS) -o vectortests
	
physicstests: physicstests.cpp ../physics.hpp ../physics.cpp ../libdiana.so
#	$(CXX) $(CFLAGS) $(USIM_FILES) physicstests.cpp  $(INCLUDE_DIR) $(LIB_DIR) $(EXT_LIBS) $(EXT_ST_LIBS) -o physicstests
	$(CXX) $(CFLAGS) physicstests.cpp  $(INCLUDE_DIR) $(LIB_DIR) -ldiana  $(EXT_LIBS) $(EXT_ST_LIBS) -o physicstests
	
runtests:all
	-./vectortests
	-LD_LIBRARY_PATH=.. ./physicstests