project(android-file-transfer)

set (CMAKE_CXX_STANDARD 11)

set(VERSION_MAJOR "3")
set(VERSION_MINOR "9")
set(VERSION_PATCH "0")
set(VERSION_COUNT 3)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

message(STATUS "building for ${CMAKE_SYSTEM_NAME}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(USB_BACKEND_DARWIN TRUE)
endif()

cmake_minimum_required(VERSION 2.8)
set(CMAKE_USE_RELATIVE_PATHS TRUE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(FindPkgConfig)
include(CheckFunctionExists)
include(CheckIncludeFiles)

find_package ( Threads )
find_package (Python COMPONENTS Development QUIET)

option(BUILD_FUSE "Build fuse mount helper" ON)
option(BUILD_MTPZ "Build with MTPZ support" ON)
option(BUILD_PYTHON "Build python module" ON)

if (BUILD_SHARED_LIB)
	set(LIB_NAME mtp-ng)
else()
	set(LIB_NAME mtp-ng-static)
endif()

if (BUILD_FUSE)
	pkg_check_modules ( FUSE fuse )
endif()

if (FUSE_FOUND)
	message(STATUS "fuse found, building mount helper")
	add_definitions(${FUSE_CFLAGS} -DFUSE_USE_VERSION=26)
endif()

if (PYTHON_FOUND)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	set(CMAKE_REQUIRED_INCLUDES ${Python_INCLUDE_DIRS})
	check_include_files("pybind11/pybind11.h" pybind11_FOUND LANGUAGE CXX)
	if (pybind11_FOUND)
		add_subdirectory(python)
	else()
		message(WARNING "pybind11 not found, skipping python bindings")
	endif()
endif()

check_include_files (magic.h HAVE_MAGIC_H)
check_library_exists(magic magic_open "" HAVE_LIBMAGIC)

if (HAVE_MAGIC_H AND HAVE_LIBMAGIC)
	message(STATUS "libmagic found")
	add_definitions(-DHAVE_LIBMAGIC)
	list(APPEND MTP_LIBRARIES magic)
	list(APPEND MTP_SHARED_LIBRARIES magic)
endif()

option(BUILD_QT_UI "Build reference Qt application" ON)
option(BUILD_SHARED_LIB "Build shared library" OFF)

option(USB_BACKEND_LIBUSB "Use libusb-1.0" OFF)
if (USB_BACKEND_LIBUSB)
	message(WARNING "WARNING! You're using libusb, this is known to be broken -- large memory consumption, violating kernel memory limits and bugs. Continue at your own risk")
endif()

add_definitions(-Wall -pthread)
add_definitions(-D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
	add_definitions(-ggdb)
endif()

set(SOURCES
	mtp/log.cpp
	mtp/ByteArray.cpp
	mtp/ptp/Device.cpp
	mtp/ptp/ObjectFormat.cpp
	mtp/ptp/PipePacketer.cpp
	mtp/ptp/Response.cpp
	mtp/ptp/Session.cpp

	mtp/usb/DeviceBusyException.cpp
	mtp/usb/BulkPipe.cpp
	mtp/usb/Request.cpp

	mtp/backend/posix/FileHandler.cpp
	mtp/backend/posix/Exception.cpp

	mtp/mtpz/TrustedApp.cpp
)

exec_program(git ARGS describe --tags OUTPUT_VARIABLE AFT_VERSION RETURN_VALUE GIT_STATUS)
if (GIT_STATUS EQUAL 0)
	string(REGEX MATCH "v([0-9a-fA-F.]+)-" _VERSION ${AFT_VERSION})
	set(AFT_BASE_VERSION ${CMAKE_MATCH_1})
else()
	file(READ ${CMAKE_SOURCE_DIR}/README.md README_MD)
	string(REGEX MATCH "version-([0-9a-fA-F.]+)-" _VERSION ${README_MD})
	set(AFT_BASE_VERSION ${CMAKE_MATCH_1})
	set(AFT_VERSION "v${AFT_BASE_VERSION}-snapshot")
endif()
message(STATUS "version: ${AFT_VERSION}, base version: ${AFT_BASE_VERSION}")

set(VERSION_SRC ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/version.cpp)
configure_file(mtp/version.cpp.in ${VERSION_SRC})

list(APPEND SOURCES ${VERSION_SRC})

if (USB_BACKEND_LIBUSB)
	pkg_check_modules(LIBUSB libusb-1.0 REQUIRED)
	add_definitions(-DUSB_BACKEND_LIBUSB)

	list(APPEND SOURCES
		mtp/backend/libusb/usb/Context.cpp
		mtp/backend/libusb/usb/Device.cpp
		mtp/backend/libusb/usb/DeviceDescriptor.cpp
		mtp/backend/libusb/usb/Exception.cpp
	)
	list(APPEND MTP_LIBRARIES ${LIBUSB_LIBRARIES})
elseif (USB_BACKEND_DARWIN)
	find_library(CORE_LIBRARY CoreFoundation)
	find_library(IOKIT_LIBRARY IOKit)
	list(APPEND MTP_LIBRARIES ${IOKIT_LIBRARY} ${CORE_LIBRARY})
	list(APPEND SOURCES
		mtp/backend/darwin/usb/Context.cpp
		mtp/backend/darwin/usb/Device.cpp
		mtp/backend/darwin/usb/DeviceDescriptor.cpp
		mtp/backend/darwin/usb/Exception.cpp
		mtp/backend/darwin/usb/Interface.cpp
	)
else()
	list(APPEND SOURCES
		mtp/backend/linux/usb/Endpoint.cpp
		mtp/backend/linux/usb/Context.cpp
		mtp/backend/linux/usb/Device.cpp
		mtp/backend/linux/usb/Interface.cpp
		mtp/backend/linux/usb/DeviceDescriptor.cpp
	)
endif()

#Always add TrustedApp to avoid polluting client code.
list(APPEND SOURCES
	mtp/mtpz/TrustedApp.cpp
)

if (BUILD_MTPZ)
	include(FindOpenSSL)
	if (OPENSSL_FOUND AND (OPENSSL_VERSION VERSION_GREATER_EQUAL 1.1))
		set(_BUILD_MTPZ TRUE)
		message(STATUS "building with MTPZ support")
		add_definitions(-DMTPZ_ENABLED)
		list(APPEND MTP_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY})
		list(APPEND MTP_SHARED_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY})
	else()
		message(WARNING "building without MTPZ support, openssl not found")
	endif()
endif()

if (BUILD_SHARED_LIB)
	set(LIB_NAME mtp-ng)
else()
	set(LIB_NAME mtp-ng-static)
endif()

if (BUILD_SHARED_LIB)
	add_library(${LIB_NAME} SHARED ${SOURCES})
	set_target_properties(${LIB_NAME} PROPERTIES SOVERSION ${VERSION})
	target_link_libraries(${LIB_NAME} ${CMAKE_THREAD_LIBS_INIT} ${MTP_SHARED_LIBRARIES})

	if (USB_BACKEND_LIBUSB)
		target_link_libraries(${LIB_NAME} ${LIBUSB_LIBRARIES})
	endif()
else ()
	add_library(${LIB_NAME} STATIC ${SOURCES})
	target_link_libraries(${LIB_NAME} ${MTP_LIBRARIES})
endif ()

list(INSERT MTP_LIBRARIES 0 ${LIB_NAME})
install(TARGETS ${LIB_NAME} LIBRARY DESTINATION "lib${LIB_SUFFIX}" ARCHIVE DESTINATION "lib${LIB_SUFFIX}")
target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/mtp/backend/posix)

if (_BUILD_MTPZ)
	target_include_directories(${LIB_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
endif()

if (USB_BACKEND_LIBUSB)
	target_include_directories(${LIB_NAME} PUBLIC ${LIBUSB_INCLUDE_DIRS})
	target_include_directories(${LIB_NAME} PUBLIC mtp/backend/libusb)
elseif (USB_BACKEND_DARWIN)
	target_include_directories(${LIB_NAME} PUBLIC mtp/backend/darwin)
else()
	target_include_directories(${LIB_NAME} PUBLIC mtp/backend/linux)
endif()

add_subdirectory(cli)

if (FUSE_FOUND)
	add_subdirectory(fuse)
endif()

if (BUILD_QT_UI)
	add_subdirectory(qt)
endif()

add_custom_target(uninstall COMMAND xargs rm < install_manifest.txt)
