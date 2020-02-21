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

static void EnableDebug(bool enable) {
	g_debug = enable;
}

PYBIND11_MODULE(aftl, m) {
	m.doc() = "Android File Transfer for Linux python bindings";
	m.def("debug", &EnableDebug, "Enables logs from MTP library");

	py::class_<Session, SessionPtr> session(m, "Session");
	py::class_<ObjectId> objectId(m, "ObjectId");

#define ENUM(TYPE, NAME) .value(#NAME, TYPE :: NAME)
#define DICT(NAME) d[#NAME] = info . NAME

	py::enum_<ObjectFormat>(m, "ObjectFormat", "MTP Object format for querying specific types of media, or Any")
		ENUM(ObjectFormat, Any)
		ENUM(ObjectFormat, Association).
		def("__repr__",
			[](ObjectFormat f) -> std::string { return "ObjectFormat(" + std::to_string(static_cast<int>(f)) + ")"; })
	;
	py::enum_<ObjectProperty>(m, "ObjectProperty", "MTP object property")
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
	py::enum_<DeviceProperty>(m, "DeviceProperty", "MTP device property")
		ENUM(DeviceProperty, Undefined)
		ENUM(DeviceProperty, BatteryLevel)
		ENUM(DeviceProperty, FunctionalMode)
		ENUM(DeviceProperty, ImageSize)
		ENUM(DeviceProperty, CompressionSetting)
		ENUM(DeviceProperty, WhiteBalance)
		ENUM(DeviceProperty, RgbGain)
		ENUM(DeviceProperty, FNumber)
		ENUM(DeviceProperty, FocalLength)
		ENUM(DeviceProperty, FocusDistance)
		ENUM(DeviceProperty, FocusMode)
		ENUM(DeviceProperty, ExposureMeteringMode)
		ENUM(DeviceProperty, FlashMode)
		ENUM(DeviceProperty, ExposureTime)
		ENUM(DeviceProperty, ExposureProgramMode)
		ENUM(DeviceProperty, ExposureIndex)
		ENUM(DeviceProperty, ExposureBiasCompensation)
		ENUM(DeviceProperty, Datetime)
		ENUM(DeviceProperty, CaptureDelay)
		ENUM(DeviceProperty, StillCaptureMode)
		ENUM(DeviceProperty, Contrast)
		ENUM(DeviceProperty, Sharpness)
		ENUM(DeviceProperty, DigitalZoom)
		ENUM(DeviceProperty, EffectMode)
		ENUM(DeviceProperty, BurstNumber)
		ENUM(DeviceProperty, BurstInterval)
		ENUM(DeviceProperty, TimelapseNumber)
		ENUM(DeviceProperty, TimelapseInterval)
		ENUM(DeviceProperty, FocusMeteringMode)
		ENUM(DeviceProperty, UploadUrl)
		ENUM(DeviceProperty, Artist)
		ENUM(DeviceProperty, CopyrightInfo)
		ENUM(DeviceProperty, SynchronizationPartner)
		ENUM(DeviceProperty, DeviceFriendlyName)
		ENUM(DeviceProperty, Volume)
		ENUM(DeviceProperty, SupportedFormatsOrdered)
		ENUM(DeviceProperty, DeviceIcon)
		ENUM(DeviceProperty, PlaybackRate)
		ENUM(DeviceProperty, PlaybackObject)
		ENUM(DeviceProperty, PlaybackContainerIndex)
		ENUM(DeviceProperty, SessionInitiatorVersionInfo)
		ENUM(DeviceProperty, PerceivedDeviceType)
	;

	py::enum_<AssociationType>(m, "AssociationType", "MTP Association Type")
		ENUM(AssociationType, GenericFolder)
		ENUM(AssociationType, Album)
		ENUM(AssociationType, TimeSequence)
		ENUM(AssociationType, HorizontalPanoramic)
		ENUM(AssociationType, VerticalPanoramic)
		ENUM(AssociationType, Panoramic2D)
		ENUM(AssociationType, AncillaryData)
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

		def("get_storage_info", [](Session *self, StorageId storage) -> py::dict {
			auto info = self->GetStorageInfo(storage);

			py::dict d;
			DICT(StorageType);
			DICT(FilesystemType);
			DICT(AccessCapability);
			DICT(MaxCapacity);
			DICT(FreeSpaceInBytes);
			DICT(FreeSpaceInImages);
			DICT(StorageDescription);
			DICT(VolumeLabel);
			return d;
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

		def("get_object_info", [](Session * self, ObjectId object) {
			auto info = self->GetObjectInfo(object);

			py::dict d;
			DICT(ObjectFormat);
			DICT(ProtectionStatus);
			DICT(ObjectCompressedSize);
			DICT(ThumbFormat);
			DICT(ThumbCompressedSize);
			DICT(ThumbPixWidth);
			DICT(ThumbPixHeight);
			DICT(ImagePixWidth);
			DICT(ImagePixHeight);
			DICT(ImageBitDepth);
			DICT(ParentObject);
			DICT(AssociationType);
			DICT(AssociationDesc);
			DICT(SequenceNumber);
			DICT(Filename);
			DICT(CaptureDate);
			DICT(ModificationDate);
			DICT(Keywords);
			return d;
		}).

		def("get_object_properties_supported", [](Session *self, ObjectId objectId) -> std::vector<ObjectProperty> {
			std::vector<ObjectProperty> result;
			auto props = self->GetObjectPropertiesSupported(objectId);
			result.reserve(props.ObjectPropertyCodes.size());
			for(auto prop: props.ObjectPropertyCodes) {
				result.push_back(prop);
			}
			return result;
		}).

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
		def("get_object_modification_time", [](Session * self, ObjectId objectId) -> std::chrono::system_clock::time_point {
			time_t t = self->GetObjectModificationTime(objectId);
			return system_clock::from_time_t(t);
		}).

		def("get_partial_object", &Session::GetPartialObject).
		def("edit_object", &Session::EditObject).

		def("get_device_property", &Session::GetDeviceProperty).
		def("get_device_integer_property", &Session::GetDeviceIntegerProperty).
		def("get_device_string_property", &Session::GetDeviceStringProperty).
		def("set_device_property", (void (Session::*)(DeviceProperty, const std::string &)) &Session::SetDeviceProperty).
		def("set_device_property", (void (Session::*)(DeviceProperty, const ByteArray &)) &Session::SetDeviceProperty).
		def("abort_current_transaction", &Session::AbortCurrentTransaction, py::arg("timeout") = static_cast<int>(Session::DefaultTimeout))
		;
	;

#if 0

		const msg::DeviceInfo & GetDeviceInfo() const
		{ return _deviceInfo; }

		NewObjectInfo CreateDirectory(const std::string &name, ObjectId parentId, StorageId storageId = AnyStorage, AssociationType type = AssociationType::GenericFolder);
		void GetObject(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		void GetThumb(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		NewObjectInfo SendObjectInfo(const msg::ObjectInfo &objectInfo, StorageId storageId = AnyStorage, ObjectId parentObject = Device);
		void SendObject(const IObjectInputStreamPtr &inputStream, int timeout = LongTimeout);

		//common properties shortcuts
		ByteArray GetObjectPropertyList(ObjectId objectId, ObjectFormat format, ObjectProperty property, u32 groupCode, u32 depth, int timeout = LongTimeout);

#endif


}
