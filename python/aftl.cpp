#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(aftl, m) {
	m.doc() = "Android File Transfer for Linux python bindings";
	
}
