CXX=g++
CFLAGS=-std=c++0x -I . -O2 -Wall -g -pthread -fopenmp

all: unisim-bin

clean:
	rm *.so *.o unisim

# %.so: %.cpp $(DEPS)
# 	$(CXX) $(CFLAGS) -shared -fPIC -o $@ $<
	
diana-shared:
	$(CXX) $(CFLAGS) vector.cpp physics.cpp universe.cpp MIMOServer.cpp unisim.cpp universe.pb.cc google/linux-x64/libprotobuf-lite.a -o libdiana.so -fPIC -shared

unisim-bin:
	$(CXX) $(CFLAGS) vector.cpp physics.cpp universe.cpp MIMOServer.cpp unisim.cpp universe.pb.cc google/linux-x64/libprotobuf-lite.a -o unisim
