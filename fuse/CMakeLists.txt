set(LL_FUSE_CODE TRUE)
if (LL_FUSE_CODE)
	add_executable(aft-mtp-mount fuse_ll.cpp)
else()
	add_executable(aft-mtp-mount fuse.cpp)
endif()

target_link_libraries(aft-mtp-mount ${MTP_LIBRARIES} ${FUSE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/aft-mtp-mount DESTINATION bin)
