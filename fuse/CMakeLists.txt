add_executable(aft-mtp-mount fuse_ll.cpp)

target_link_libraries(aft-mtp-mount ${MTP_LIBRARIES} ${FUSE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/aft-mtp-mount DESTINATION bin)
