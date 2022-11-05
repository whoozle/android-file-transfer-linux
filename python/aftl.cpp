#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include <mtp/ptp/Device.h>
#include <mtp/ptp/Session.h>
#include <usb/Context.h>
#include <usb/Device.h>
#include <mtp/log.h>

namespace py = pybind11;

using namespace mtp;

using system_clock = std::chrono::system_clock;

namespace pybind11 { namespace detail {
	template <> struct type_caster<ByteArray> {
	public:
		PYBIND11_TYPE_CASTER(ByteArray, _("ByteArray"));

		bool load(handle src, bool) {
			PyObject *source = src.ptr();
			if (!PyByteArray_Check(source))
				return false;

			auto size = PyByteArray_Size(source);
			u8 * ptr = reinterpret_cast<u8 *>(PyByteArray_AsString(source));
			if (ptr && size)
				value.assign(ptr, ptr + size);
			else
				value.clear();

			return !PyErr_Occurred();
		}

		static handle cast(const ByteArray & src, return_value_policy /* policy */, handle /* parent */) {
			return PyByteArray_FromStringAndSize(reinterpret_cast<const char *>(src.data()), src.size());
		}
	};
}} // namespace pybind11::detail

namespace
{
	struct PythonInputStream final : public IObjectInputStream
	{
		py::object 	ReadMethod;
		u64			Size;
		bool 		Cancelled = false;

		PythonInputStream(py::object object, u64 size):
			ReadMethod(object.attr("read")),
			Size(size)
		{ }

   		void Cancel() override
		{ Cancelled = true; }

		u64 GetSize() const override
		{ return Size; }

		size_t Read(u8 *data, size_t maxSize) override
		{
			if (Cancelled)
				return 0;

			py::bytes value = ReadMethod(maxSize);
			char *buffer;
			ssize_t size;
			if (PYBIND11_BYTES_AS_STRING_AND_SIZE(value.ptr(), &buffer, &size) || !buffer || size > maxSize)
				py::pybind11_fail("Unable to extract bytes contents!");

			memcpy(data, buffer, size);
			return PyErr_Occurred()? 0: size;
		}
	};

	struct PythonOutputStream final : public IObjectOutputStream
	{
		py::object 	WriteMethod;
		bool 		Cancelled = false;

		PythonOutputStream(py::object object): WriteMethod(object.attr("write"))
		{ }

		void Cancel() override
		{ Cancelled = true; }
		size_t Write(const u8* data, size_t size) override
		{
			if (Cancelled)
				return 0;
			py::bytes payload(reinterpret_cast<const char *>(data), size);
			WriteMethod(payload).cast<long>();
			return PyErr_Occurred()? 0: size;
		}
	};

}

static void EnableDebug(bool enable) {
	g_debug = enable;
}

PYBIND11_MODULE(aftl, m) {
	m.doc() = "Android File Transfer for Linux python bindings";
	m.def("debug", &EnableDebug, "Enables logs from MTP library");

	py::class_<Session, SessionPtr> session(m, "Session");
	py::class_<ObjectId> objectId(m, "ObjectId");

#define VALUE(TYPE, NAME) .value(#NAME, TYPE :: NAME)
#define DEF_READONLY(TYPE, NAME) .def_readonly(#NAME, & TYPE :: NAME )

	py::enum_<ObjectFormat>(m, "ObjectFormat", "MTP Object format for querying specific types of media, or Any")
#define ENUM_VALUE(NAME, _) VALUE(ObjectFormat, NAME)
#		include <mtp/ptp/ObjectFormat.values.h>
#undef ENUM_VALUE
	;
	py::enum_<ObjectProperty>(m, "ObjectProperty", "MTP object property")
#define ENUM_VALUE(NAME, _) VALUE(ObjectProperty, NAME)
#		include <mtp/ptp/ObjectProperty.values.h>
#undef ENUM_VALUE
	;
	py::enum_<DeviceProperty>(m, "DeviceProperty", "MTP device property")
#define ENUM_VALUE(NAME, _) VALUE(DeviceProperty, NAME)
#		include <mtp/ptp/DeviceProperty.values.h>
#undef ENUM_VALUE
	;

	py::enum_<AssociationType>(m, "AssociationType", "MTP Association Type")
		VALUE(AssociationType, GenericFolder)
		VALUE(AssociationType, Album)
		VALUE(AssociationType, TimeSequence)
		VALUE(AssociationType, HorizontalPanoramic)
		VALUE(AssociationType, VerticalPanoramic)
		VALUE(AssociationType, Panoramic2D)
		VALUE(AssociationType, AncillaryData)
	;

	py::enum_<OperationCode>(m, "OperationCode", "MTP Operation Code")
#define ENUM_VALUE(NAME, _) VALUE(OperationCode, NAME)
#		include <mtp/ptp/OperationCode.values.h>
#undef ENUM_VALUE
	;

	py::class_<usb::DeviceDescriptor, usb::DeviceDescriptorPtr>(m, "DeviceDescriptor").
		def_property_readonly("vendor_id", &usb::DeviceDescriptor::GetVendorId).
		def_property_readonly("product_id", &usb::DeviceDescriptor::GetProductId).
		def("__repr__",
			[](const usb::DeviceDescriptor f) -> std::string {
				return "DeviceDescriptor(" + std::to_string(f.GetVendorId()) + ":" + std::to_string(f.GetProductId()) + ")";
			})
		;
	;

	py::class_<usb::Context, usb::ContextPtr>(m, "UsbContext").
		def(py::init<>()).
		def("get_device_descriptors", &usb::Context::GetDevices)
	;

	py::class_<Device, DevicePtr>(m, "Device").
		def_static("find_first", static_cast<DevicePtr (*)(const std::string &, bool claimInterface, bool)>(&Device::FindFirst),
			py::arg("filter_device") = std::string(), py::arg("claim_interface") = true, py::arg("reset_device") = false).
		def_static("open", &Device::Open, py::arg("context"), py::arg("device_descriptor"), py::arg("claim_interface") = true, py::arg("reset_device") = false).
		def("open_session", &Device::OpenSession,
			py::arg("session_id") = 1, py::arg("timeout") = static_cast<int>(Session::DefaultTimeout))
	;

	py::class_<StorageId>(m, "StorageId").
		def("__repr__",
			[](const StorageId &id) { return "StorageId(" + std::to_string(id.Id) + ")"; });

	objectId.
		def("__repr__",
			[](const ObjectId &id) { return "ObjectId(" + std::to_string(id.Id) + ")"; });

	py::class_<msg::NewObjectInfo>(m, "NewObjectInfo", "information about newly created object").
		def_readonly("storage_id", &msg::NewObjectInfo::StorageId).
		def_readonly("parent_object_id", &msg::NewObjectInfo::ParentObjectId).
		def_readonly("object_id", &msg::NewObjectInfo::ObjectId)
	;

	py::class_<msg::ObjectInfo>(m, "ObjectInfo").
		def(py::init<>())
		DEF_READONLY(msg::ObjectInfo, ObjectFormat)
		DEF_READONLY(msg::ObjectInfo, ProtectionStatus)
		DEF_READONLY(msg::ObjectInfo, ObjectCompressedSize)
		DEF_READONLY(msg::ObjectInfo, ThumbFormat)
		DEF_READONLY(msg::ObjectInfo, ThumbCompressedSize)
		DEF_READONLY(msg::ObjectInfo, ThumbPixWidth)
		DEF_READONLY(msg::ObjectInfo, ThumbPixHeight)
		DEF_READONLY(msg::ObjectInfo, ImagePixWidth)
		DEF_READONLY(msg::ObjectInfo, ImagePixHeight)
		DEF_READONLY(msg::ObjectInfo, ImageBitDepth)
		DEF_READONLY(msg::ObjectInfo, ParentObject)
		DEF_READONLY(msg::ObjectInfo, AssociationType)
		DEF_READONLY(msg::ObjectInfo, AssociationDesc)
		DEF_READONLY(msg::ObjectInfo, SequenceNumber)
		DEF_READONLY(msg::ObjectInfo, Filename)
		DEF_READONLY(msg::ObjectInfo, CaptureDate)
		DEF_READONLY(msg::ObjectInfo, ModificationDate)
		DEF_READONLY(msg::ObjectInfo, Keywords)
	;

	py::class_<msg::StorageInfo>(m, "StorageInfo")
		DEF_READONLY(msg::StorageInfo, StorageType)
		DEF_READONLY(msg::StorageInfo, FilesystemType)
		DEF_READONLY(msg::StorageInfo, AccessCapability)
		DEF_READONLY(msg::StorageInfo, MaxCapacity)
		DEF_READONLY(msg::StorageInfo, FreeSpaceInBytes)
		DEF_READONLY(msg::StorageInfo, FreeSpaceInImages)
		DEF_READONLY(msg::StorageInfo, StorageDescription)
		DEF_READONLY(msg::StorageInfo, VolumeLabel)
	;
	py::class_<msg::DeviceInfo>(m, "DeviceInfo")
		DEF_READONLY(msg::DeviceInfo, StandardVersion)
		DEF_READONLY(msg::DeviceInfo, VendorExtensionId)
		DEF_READONLY(msg::DeviceInfo, VendorExtensionVersion)
		DEF_READONLY(msg::DeviceInfo, VendorExtensionDesc)
		DEF_READONLY(msg::DeviceInfo, FunctionalMode)
		DEF_READONLY(msg::DeviceInfo, OperationsSupported)
		DEF_READONLY(msg::DeviceInfo, EventsSupported)
		DEF_READONLY(msg::DeviceInfo, DevicePropertiesSupported)
		DEF_READONLY(msg::DeviceInfo, CaptureFormats)
		DEF_READONLY(msg::DeviceInfo, ImageFormats)
		DEF_READONLY(msg::DeviceInfo, Manufacturer)
		DEF_READONLY(msg::DeviceInfo, Model)
		DEF_READONLY(msg::DeviceInfo, DeviceVersion)
		DEF_READONLY(msg::DeviceInfo, SerialNumber)
	;

	py::class_<Session::ObjectEditSession, Session::ObjectEditSessionPtr>(m, "ObjectEditSession").
		def("truncate", &Session::ObjectEditSession::Truncate).
		def("send", &Session::ObjectEditSession::Send)
	;


	session.
		// def_readonly_static("DefaultTimeout", &Session::DefaultTimeout).
		// def_readonly_static("LongTimeout", &Session::LongTimeout).

		def_readonly_static("AllStorages", &Session::AllStorages).
		def_readonly_static("AnyStorage", &Session::AnyStorage).
		def_readonly_static("Device", &Session::Device).
		def_readonly_static("Root", &Session::Root).

		def("get_storage_ids", [](Session * self) -> std::vector<StorageId> {
			std::vector<StorageId> result;
			auto sids = self->GetStorageIDs();
			result.reserve(sids.StorageIDs.size());
			for(auto & sid: sids.StorageIDs) {
				result.push_back(sid);
			}
			return result;
		}).

		def("get_storage_info", &Session::GetStorageInfo).

		def("get_object_handles", [](Session *self, StorageId storageId, ObjectFormat objectFormat, ObjectId parent, int timeout) -> std::vector<ObjectId> {
			std::vector<ObjectId> result;
			auto objects = self->GetObjectHandles(storageId, objectFormat, parent, timeout);
			result.reserve(objects.ObjectHandles.size());
			for(auto & oid: objects.ObjectHandles) {
				result.push_back(oid);
			}
			return result;
		}, py::arg("storageId"), py::arg("objectFormat") = ObjectFormat::Any, py::arg("parent") = Session::Root, py::arg("timeout") = static_cast<int>(Session::LongTimeout)).

		def("get_object_info", &Session::GetObjectInfo).
		def("get_object_properties_supported", [](Session *self, ObjectFormat format) -> std::vector<ObjectProperty> {
			std::vector<ObjectProperty> result;
			auto props = self->GetObjectPropertiesSupported(format);
			result.reserve(props.ObjectPropertyCodes.size());
			for(auto prop: props.ObjectPropertyCodes) {
				result.push_back(prop);
			}
			return result;
		}).

		def("get_object", [](Session * self, ObjectId object, py::object stream) {
			self->GetObject(object, std::make_shared<PythonOutputStream>(stream));
		}).
		def("get_thumb", [](Session * self, ObjectId object, py::object stream) {
			self->GetThumb(object, std::make_shared<PythonOutputStream>(stream));
		}).

		def("send_object", [](Session * self, py::object stream, long size, int timeout) {
			self->SendObject(std::make_shared<PythonInputStream>(stream, size), timeout);
		}, py::arg("stream"), py::arg("size"), py::arg("timeout") = static_cast<int>(Session::LongTimeout)).

		def("get_object_property", &Session::GetObjectProperty).
		def("get_object_string_property", &Session::GetObjectStringProperty).
		def("get_object_integer_property", &Session::GetObjectIntegerProperty).

		def("set_object_property", (void (Session::*)(ObjectId, ObjectProperty, u64)) &Session::SetObjectProperty).
		def("set_object_property", (void (Session::*)(ObjectId, ObjectProperty, const std::string &)) &Session::SetObjectProperty).
		def("set_object_property", (void (Session::*)(ObjectId, ObjectProperty, const ByteArray &)) &Session::SetObjectProperty).

		def("get_object_storage", &Session::GetObjectStorage).
		def("get_object_parent", &Session::GetObjectParent).
		def("delete_object", &Session::DeleteObject).

		def("edit_object_supported", &Session::EditObjectSupported).
		def("get_object_property_list_supported", &Session::GetObjectPropertyListSupported).
		def("get_object_property_list", &Session::GetObjectPropertyList).
		def("get_object_modification_time", [](Session * self, ObjectId objectId) -> std::chrono::system_clock::time_point {
			time_t t = self->GetObjectModificationTime(objectId);
			return system_clock::from_time_t(t);
		}).

		def("get_partial_object", &Session::GetPartialObject).
		def("edit_object", &Session::EditObject).

		def("create_directory", &Session::CreateDirectory, py::arg("name"), py::arg("parent"), py::arg("storage") = Session::AnyStorage, py::arg("association_type") = AssociationType::GenericFolder).
		def("send_object_info", &Session::SendObjectInfo, py::arg("object_info"), py::arg("storage"), py::arg("parent")).

		def("get_device_info", static_cast<const msg::DeviceInfo & (Session::*)() const>(&Session::GetDeviceInfo)).
		def("get_device_property", &Session::GetDeviceProperty).
		def("get_device_integer_property", &Session::GetDeviceIntegerProperty).
		def("get_device_string_property", &Session::GetDeviceStringProperty).
		def("set_device_property", (void (Session::*)(DeviceProperty, const std::string &)) &Session::SetDeviceProperty).
		def("set_device_property", (void (Session::*)(DeviceProperty, const ByteArray &)) &Session::SetDeviceProperty).

		def("abort_current_transaction", &Session::AbortCurrentTransaction, py::arg("timeout") = static_cast<int>(Session::DefaultTimeout))
		;
	;
}
