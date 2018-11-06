add_executable(aft-mtp-mount fuse_ll.cpp)

target_link_libraries(aft-mtp-mount ${MTP_LIBRARIES} ${FUSE_LDFLAGS} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS aft-mtp-mount RUNTIME DESTINATION bin)
