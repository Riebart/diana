#include <chrono>

#include "reflection.hpp"
#include "messaging.hpp"

REFLECTION_STRUCT(PhysicalPropertiesMsg,
    (std::int64_t) server_id,
    (std::int64_t) client_id,
    (Optional<const char*>) object_type,
    (Optional<double>) mass,
    (Optional<double>) radius,
    (Optional<Vector3<double>>) position,
    (Optional<Vector3<double>>) velocity,
    (Optional<Vector3<double>>) thrust,
    (Optional<Vector4<double>>) orientation
)

REFLECTION_STRUCT(BenchmarkResults,
    (int) loop_count,
    (int) bytes_json, (int) ms_json, (float) rate_json,
    (int) bytes_json_n, (int) ms_json_n, (float) rate_json_n,
    (int) bytes_binary_write, (int) ms_binary_write, (float) rate_binary_write,
    (int) bytes_binary_read, (int) ms_binary_read, (float) rate_binary_read
)

std::uint8_t buf[4096];

int main(int argc, char** argv)
{
    PhysicalPropertiesMsg msg{}, msg2{};
    
    std::cout << sizeof(msg) << std::endl;
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    msg.radius = 102020.1;
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    msg.mass = 0.1;
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    msg.object_type = "This is some stuff!"; // strnlen() = 19 + null
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    msg.position = Vector3<double>(1.1, 2.2, 3.3);
    msg.orientation = Vector4<double>(-100.1,-200.2,-300.3,-400.4);
    std::cout << msg.binary_size() << " " << msg.json() << std::endl;
    std::cout << msg.json() << std::endl;

    std::size_t binary_write_count = msg.binary_write(buf);
    std::cout << msg.binary_size() << " " << binary_write_count << std::endl;

    std::size_t binary_read_count = msg2.binary_read(buf);
    std::cout << binary_read_count << std::endl;
    std::cout << msg2.json() << std::endl;
    std::cout << msg2.json_n() << std::endl;
    msg2.velocity = Vector3<double>(18.1,18.9,1963.02);
    msg2.thrust = Vector3<double>(144.1,1333.9,1653.008);

    std::chrono::time_point<std::chrono::high_resolution_clock> t0, t1;
    std::chrono::milliseconds dt, bare_loop_dt;

    BenchmarkResults results{};
    results.loop_count = 500000;

    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0 ; i < results.loop_count ; i++)
    {
        msg2.mass.present = i & 1;
        msg2.radius.present = i & 2;
        msg2.position.present = i & 4;
        msg2.velocity.present = i & 8;
        msg2.thrust.present = i & 16;
        msg2.object_type.present = i & 32;
        msg2.orientation.present = i & 64;
    }
    t1 = std::chrono::high_resolution_clock::now();
    bare_loop_dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);

    results.bytes_json = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0 ; i < results.loop_count ; i++)
    {
        msg2.mass.present = i & 1;
        msg2.radius.present = i & 2;
        msg2.position.present = i & 4;
        msg2.velocity.present = i & 8;
        msg2.thrust.present = i & 16;
        msg2.object_type.present = i & 32;
        msg2.orientation.present = i & 64;
        results.bytes_json += msg2.json().length();
    }
    t1 = std::chrono::high_resolution_clock::now();
    dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
    results.ms_json = dt.count() - bare_loop_dt.count();

    results.bytes_json_n = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0 ; i < results.loop_count ; i++)
    {
        msg2.mass.present = i & 1;
        msg2.radius.present = i & 2;
        msg2.position.present = i & 4;
        msg2.velocity.present = i & 8;
        msg2.thrust.present = i & 16;
        msg2.object_type.present = i & 32;
        msg2.orientation.present = i & 64;
        results.bytes_json_n += msg2.json_n().length();
    }
    t1 = std::chrono::high_resolution_clock::now();
    dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
    results.ms_json_n = dt.count() - bare_loop_dt.count();
    
    results.bytes_binary_write = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0 ; i < results.loop_count ; i++)
    {
        msg2.mass.present = i & 1;
        msg2.radius.present = i & 2;
        msg2.position.present = i & 4;
        msg2.velocity.present = i & 8;
        msg2.thrust.present = i & 16;
        msg2.object_type.present = i & 32;
        msg2.orientation.present = i & 64;
        results.bytes_binary_write += msg2.binary_write(buf);
    }
    t1 = std::chrono::high_resolution_clock::now();
    dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
    results.ms_binary_write = dt.count() - bare_loop_dt.count();

    results.bytes_binary_read = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0 ; i < results.loop_count ; i++)
    {
        msg2.mass.present = i & 1;
        msg2.radius.present = i & 2;
        msg2.position.present = i & 4;
        msg2.velocity.present = i & 8;
        msg2.thrust.present = i & 16;
        msg2.object_type.present = i & 32;
        msg2.orientation.present = i & 64;
        msg2.binary_write(buf);
        results.bytes_binary_read += msg.binary_read(buf);
    }
    t1 = std::chrono::high_resolution_clock::now();
    dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
    results.ms_binary_read = dt.count() - results.ms_binary_write - bare_loop_dt.count();

    results.rate_json = results.loop_count / (0.001 * results.ms_json);
    results.rate_json_n = results.loop_count / (0.001 * results.ms_json_n);
    results.rate_binary_write = results.loop_count / (0.001 * results.ms_binary_write);
    results.rate_binary_read = results.loop_count / (0.001 * results.ms_binary_read);
    
    std::cout << results.json() << std::endl;

    return 0;
}
