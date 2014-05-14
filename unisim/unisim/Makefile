CXX=g++
CFLAGS=-std=c++0x -I . -O0 -Wall -g

all: unisim-bin

clean:
	rm *.so *.o unisim

# %.so: %.cpp $(DEPS)
# 	$(CXX) $(CFLAGS) -shared -fPIC -o $@ $<
	
diana-shared:
	$(CXX) $(CFLAGS) *.cpp -o libdiana.so -fPIC -shared

unisim-bin:
	$(CXX) $(CFLAGS) *.cpp -o unisim