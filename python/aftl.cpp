#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <mtp/ptp/Device.h>
#include <mtp/ptp/Session.h>
#include <usb/Context.h>
#include <usb/Device.h>

namespace py = pybind11;

using namespace mtp;

PYBIND11_MODULE(aftl, m) {
	m.doc() = "Android File Transfer for Linux python bindings";

	py::class_<usb::DeviceDescriptor, usb::DeviceDescriptorPtr>(m, "DeviceDescriptor").
		def_property_readonly("vendor_id", &usb::DeviceDescriptor::GetVendorId).
		def_property_readonly("product_id", &usb::DeviceDescriptor::GetProductId);
	;

	py::class_<Device, DevicePtr>(m, "Device").
		def_static("find_first", &Device::FindFirst,
			py::arg("claim_interface") = true, py::arg("reset") = false).
		def("open_session", &Device::OpenSession,
			py::arg("session_id") = 1, py::arg("timeout") = static_cast<int>(Session::DefaultTimeout))
	;

	py::class_<Session, SessionPtr>(m, "Session");
}
