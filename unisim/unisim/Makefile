CXX=g++
# Note that there aren't 32-bit protobuf libraries included, so building on 32-bit platforms won't work.
INCLUDE_DIR=-I . -I libodb/src/include -I google/protobuf
CFLAGS=-O3 -Wall -g -pthread -fopenmp
FILES=vector.cpp physics.cpp universe.cpp MIMOServer.cpp ../../protocols/universe.pb.cc
EXT_LIBS=-lodb -Llibodb/linux-x64
EXT_ST_LIBS=google/linux-x64/libprotobuf-lite.a

all: unisim-bin

clean:
	rm unisim libdiana.so

diana-shared:
	$(CXX) $(CFLAGS) $(INDLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) -o libdiana.so -fPIC -shared
	cp libodb/linux-x64/libodb.so ./

unisim-bin:
	$(CXX) $(CFLAGS) $(INCLUDE_DIR) $(FILES) $(EXT_LIBS) $(EXT_ST_LIBS) unisim.cpp -o unisim
	cp libodb/linux-x64/libodb.so ./
