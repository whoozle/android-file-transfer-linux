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

	py::class_<StorageId>(m, "StorageId");
	py::class_<ObjectId>(m, "ObjectId");

	py::class_<Session, SessionPtr>(m, "Session").
		// def_readonly_static("DefaultTimeout", &Session::DefaultTimeout).
		// def_readonly_static("LongTimeout", &Session::LongTimeout).

		def_readonly_static("AllStorages", &Session::AllStorages).
		def_readonly_static("AnyStorage", &Session::AnyStorage).
		def_readonly_static("Device", &Session::Device).
		def_readonly_static("Root", &Session::Root).

		def("get_storage_ids", [](Session * self) -> std::vector<StorageId> {
			std::vector<StorageId> result;
			auto sids = self->GetStorageIDs();
			for(auto & sid: sids.StorageIDs) {
				result.push_back(sid);
			}
			return result;
		});
	;

	py::class_<Session::NewObjectInfo>(m, "NewObjectInfo").
		def_readonly("storage_id", &Session::NewObjectInfo::StorageId).
		def_readonly("parent_object_id", &Session::NewObjectInfo::ParentObjectId).
		def_readonly("object_id", &Session::NewObjectInfo::ObjectId)
	;

	py::class_<Session::ObjectEditSession, Session::ObjectEditSessionPtr>(m, "ObjectEditSession").
		def("truncate", &Session::ObjectEditSession::Truncate).
		def("send", &Session::ObjectEditSession::Send)
	;

#if 0

		const msg::DeviceInfo & GetDeviceInfo() const
		{ return _deviceInfo; }

		msg::ObjectHandles GetObjectHandles(StorageId storageId = AllStorages, ObjectFormat objectFormat = ObjectFormat::Any, ObjectId parent = Device, int timeout = LongTimeout);
		msg::StorageInfo GetStorageInfo(StorageId storageId);

		NewObjectInfo CreateDirectory(const std::string &name, ObjectId parentId, StorageId storageId = AnyStorage, AssociationType type = AssociationType::GenericFolder);
		msg::ObjectInfo GetObjectInfo(ObjectId objectId);
		void GetObject(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		void GetThumb(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		ByteArray GetPartialObject(ObjectId objectId, u64 offset, u32 size);
		NewObjectInfo SendObjectInfo(const msg::ObjectInfo &objectInfo, StorageId storageId = AnyStorage, ObjectId parentObject = Device);
		void SendObject(const IObjectInputStreamPtr &inputStream, int timeout = LongTimeout);
		void DeleteObject(ObjectId objectId, int timeout = LongTimeout);

		bool EditObjectSupported() const
		{ return _editObjectSupported; }
		bool GetObjectPropertyListSupported() const
		{ return _getObjectPropertyListSupported; }

		static ObjectEditSessionPtr EditObject(const SessionPtr &session, ObjectId objectId)
		{ return std::make_shared<ObjectEditSession>(session, objectId); }

		msg::ObjectPropertiesSupported GetObjectPropertiesSupported(ObjectId objectId);

		void SetObjectProperty(ObjectId objectId, ObjectProperty property, const ByteArray &value);
		void SetObjectProperty(ObjectId objectId, ObjectProperty property, u64 value);
		void SetObjectProperty(ObjectId objectId, ObjectProperty property, const std::string &value);
		time_t GetObjectModificationTime(ObjectId id);

		//common properties shortcuts
		StorageId GetObjectStorage(ObjectId id);
		ObjectId GetObjectParent(ObjectId id);

		ByteArray GetObjectProperty(ObjectId objectId, ObjectProperty property);
		u64 GetObjectIntegerProperty(ObjectId objectId, ObjectProperty property);
		std::string GetObjectStringProperty(ObjectId objectId, ObjectProperty property);

		ByteArray GetObjectPropertyList(ObjectId objectId, ObjectFormat format, ObjectProperty property, u32 groupCode, u32 depth, int timeout = LongTimeout);

		ByteArray GetDeviceProperty(DeviceProperty property);
		u64 GetDeviceIntegerProperty(DeviceProperty property);
		std::string GetDeviceStringProperty(DeviceProperty property);
		void SetDeviceProperty(DeviceProperty property, const ByteArray & value);
		void SetDeviceProperty(DeviceProperty property, const std::string & value);

		void AbortCurrentTransaction(int timeout = DefaultTimeout);

#endif


}
