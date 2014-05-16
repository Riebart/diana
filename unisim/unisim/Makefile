CXX=g++
CFLAGS=-std=c++0x -I . -O2 -Wall -g -pthread -fopenmp
FILES=vector.cpp physics.cpp universe.cpp MIMOServer.cpp ../../protocols/universe.pb.cc
EXT_LIBS=google/linux-x64/libprotobuf-lite.a

all: unisim-bin

clean:
	rm unisim libdiana.so *so *o

# %.so: %.cpp $(DEPS)
# 	$(CXX) $(CFLAGS) -shared -fPIC -o $@ $<
	
diana-shared:
	$(CXX) $(CFLAGS) $(FILES) $(EXT_LIBS) -o libdiana.so -fPIC -shared

unisim-bin:
	$(CXX) $(CFLAGS) $(FILES) $(EXT_LIBS) unisim.cpp -o unisim
