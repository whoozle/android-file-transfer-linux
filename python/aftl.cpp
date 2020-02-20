#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <mtp/ptp/Device.h>
#include <mtp/ptp/Session.h>
#include <usb/Context.h>
#include <usb/Device.h>
#include <mtp/log.h>

namespace py = pybind11;

using namespace mtp;

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

static void EnableDebug(bool enable) {
	g_debug = enable;
}

PYBIND11_MODULE(aftl, m) {
	m.doc() = "Android File Transfer for Linux python bindings";
	m.def("debug", &EnableDebug, "Enables logs from MTP library");

	py::class_<Session, SessionPtr> session(m, "Session");
	py::class_<ObjectId> objectId(m, "ObjectId");

#define ENUM(TYPE, NAME) .value(#NAME, TYPE :: NAME)

	py::enum_<ObjectFormat>(m, "ObjectFormat", "MTP Object format for querying specific types of media, or Any")
		ENUM(ObjectFormat, Any)
		ENUM(ObjectFormat, Association).
		def("__repr__",
			[](ObjectFormat f) -> std::string { return "ObjectFormat(" + std::to_string(static_cast<int>(f)) + ")"; })
	;
	py::enum_<ObjectProperty>(m, "ObjectProperty", "MTP Object property")
		ENUM(ObjectProperty, StorageId)
		ENUM(ObjectProperty, ObjectFormat)
		ENUM(ObjectProperty, ProtectionStatus)
		ENUM(ObjectProperty, ObjectSize)
		ENUM(ObjectProperty, AssociationType)
		ENUM(ObjectProperty, AssociationDesc)
		ENUM(ObjectProperty, ObjectFilename)
		ENUM(ObjectProperty, DateCreated)
		ENUM(ObjectProperty, DateModified)
		ENUM(ObjectProperty, Keywords)
		ENUM(ObjectProperty, ParentObject)
		ENUM(ObjectProperty, AllowedFolderContents)
		ENUM(ObjectProperty, Hidden)
		ENUM(ObjectProperty, SystemObject)

		ENUM(ObjectProperty, PersistentUniqueObjectId)
		ENUM(ObjectProperty, SyncId)
		ENUM(ObjectProperty, Name)
		ENUM(ObjectProperty, Artist)
		ENUM(ObjectProperty, DateAuthored)
		ENUM(ObjectProperty, DateAdded)

		ENUM(ObjectProperty, RepresentativeSampleFormat)
		ENUM(ObjectProperty, RepresentativeSampleData)

		ENUM(ObjectProperty, DisplayName)
		ENUM(ObjectProperty, BodyText)
		ENUM(ObjectProperty, Subject)
		ENUM(ObjectProperty, Priority)

		ENUM(ObjectProperty, MediaGUID)
		ENUM(ObjectProperty, All)
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

	py::class_<Device, DevicePtr>(m, "Device").
		def_static("find_first", &Device::FindFirst,
			py::arg("claim_interface") = true, py::arg("reset") = false).
		def("open_session", &Device::OpenSession,
			py::arg("session_id") = 1, py::arg("timeout") = static_cast<int>(Session::DefaultTimeout))
	;

	py::class_<StorageId>(m, "StorageId").
		def("__repr__",
			[](const StorageId &id) { return "StorageId(" + std::to_string(id.Id) + ")"; });

	objectId.
		def("__repr__",
			[](const ObjectId &id) { return "ObjectId(" + std::to_string(id.Id) + ")"; });

	py::class_<Session::NewObjectInfo>(m, "NewObjectInfo").
		def_readonly("storage_id", &Session::NewObjectInfo::StorageId).
		def_readonly("parent_object_id", &Session::NewObjectInfo::ParentObjectId).
		def_readonly("object_id", &Session::NewObjectInfo::ObjectId)
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

		def("get_object_handles", [](Session *self, StorageId storageId, ObjectFormat objectFormat, ObjectId parent, int timeout) -> std::vector<ObjectId> {
			std::vector<ObjectId> result;
			auto objects = self->GetObjectHandles(storageId, objectFormat, parent, timeout);
			result.reserve(objects.ObjectHandles.size());
			for(auto & oid: objects.ObjectHandles) {
				result.push_back(oid);
			}
			return result;
		}, py::arg("storageId"), py::arg("objectFormat") = ObjectFormat::Any, py::arg("parent") = Session::Root, py::arg("timeout") = static_cast<int>(Session::LongTimeout)).

		def("get_object_property", &Session::GetObjectProperty).
		def("get_object_string_property", &Session::GetObjectStringProperty).
		def("get_object_integer_property", &Session::GetObjectIntegerProperty).

		def("set_object_property", (void (Session::*)(ObjectId, ObjectProperty, u64)) &Session::SetObjectProperty).
		def("set_object_property", (void (Session::*)(ObjectId, ObjectProperty, const std::string &)) &Session::SetObjectProperty).
		def("set_object_property", (void (Session::*)(ObjectId, ObjectProperty, const ByteArray &)) &Session::SetObjectProperty).

		def("get_object_storage", &Session::GetObjectStorage).
		def("get_object_parent", &Session::GetObjectParent).
		def("abort_current_transaction", &Session::AbortCurrentTransaction, py::arg("timeout") = static_cast<int>(Session::DefaultTimeout)).
		def("delete_object", &Session::DeleteObject).

		def("get_partial_object", &Session::GetPartialObject)
		;
	;

#if 0

		const msg::DeviceInfo & GetDeviceInfo() const
		{ return _deviceInfo; }

		msg::StorageInfo GetStorageInfo(StorageId storageId);

		NewObjectInfo CreateDirectory(const std::string &name, ObjectId parentId, StorageId storageId = AnyStorage, AssociationType type = AssociationType::GenericFolder);
		msg::ObjectInfo GetObjectInfo(ObjectId objectId);
		void GetObject(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		void GetThumb(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		NewObjectInfo SendObjectInfo(const msg::ObjectInfo &objectInfo, StorageId storageId = AnyStorage, ObjectId parentObject = Device);
		void SendObject(const IObjectInputStreamPtr &inputStream, int timeout = LongTimeout);

		bool EditObjectSupported() const
		{ return _editObjectSupported; }
		bool GetObjectPropertyListSupported() const
		{ return _getObjectPropertyListSupported; }

		static ObjectEditSessionPtr EditObject(const SessionPtr &session, ObjectId objectId)
		{ return std::make_shared<ObjectEditSession>(session, objectId); }

		msg::ObjectPropertiesSupported GetObjectPropertiesSupported(ObjectId objectId);

		time_t GetObjectModificationTime(ObjectId id);

		//common properties shortcuts
		ByteArray GetObjectPropertyList(ObjectId objectId, ObjectFormat format, ObjectProperty property, u32 groupCode, u32 depth, int timeout = LongTimeout);

		ByteArray GetDeviceProperty(DeviceProperty property);
		u64 GetDeviceIntegerProperty(DeviceProperty property);
		std::string GetDeviceStringProperty(DeviceProperty property);
		void SetDeviceProperty(DeviceProperty property, const ByteArray & value);
		void SetDeviceProperty(DeviceProperty property, const std::string & value);

#endif


}
