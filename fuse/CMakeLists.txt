add_executable(aft-mtp-mount fuse_ll.cpp)

target_link_libraries(aft-mtp-mount mtp-ng-static ${FUSE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS aft-mtp-mount RUNTIME DESTINATION bin)
