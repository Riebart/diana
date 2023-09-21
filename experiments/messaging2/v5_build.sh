g++ --std=c++17 -fPIC -shared -I/usr/include/python3.10 -o diana_messaging.so v5_python_lib.cpp && \
python3 -c 'import diana_messaging; print(dir(diana_messaging.PhysicalPropertiesMsg)); print(diana_messaging.PhysicalPropertiesMsg().dump_size())'
