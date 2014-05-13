CXX=g++
CFLAGS=-std=c++0x -I .

clean:
	rm *.so *.o unisim

%.so: %.cpp $(DEPS)
	$(CXX) $(CFLAGS) -shared -fPIC -o $@ $<

all:
	$(CXX) $(CFLAGS) -shared -fPIC vector.cpp -o vector.so
	$(CXX) $(CFLAGS) -shared -fPIC physics.cpp -o physics.so
	$(CXX) $(CFLAGS) -shared -fPIC MIMOServer.cpp -o MIMOServer.so
