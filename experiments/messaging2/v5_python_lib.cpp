#include "v5_messages.hpp"
#include "extern/pybind11/include/pybind11/pybind11.h"

namespace py = pybind11;

PYBIND11_MODULE(diana_messaging, m)
{
    m.doc() = "Diana experimental messaging library";

    py::class_<Element<std::int64_t>>(m, "ElementInt64");
    py::class_<Element<std::double>>(m, "ElementDouble");
    py::class_<OptionalElement<std::int64_t>>(m, "OptionalElementInt64");
    py::class_<OptionalElement<std::int64_t>>(m, "OptionalElementDouble");

    py::class_<Message>(m, "DianaMessage")
    .def(py::init<>());

    // m.def("add", &add, "A function that adds two numbers");
    py::class_<PhysicalPropertiesMsg, Message>(m, "PhysicalPropertiesMsg")
    .def(py::init<>())
    .def("dump_size", &PhysicalPropertiesMsg::dump_size);
}
