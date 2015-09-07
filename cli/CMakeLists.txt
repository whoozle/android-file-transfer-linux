find_package(Readline)

if (READLINE_FOUND)

endif (READLINE_FOUND)

add_executable(aft-mtp-cli cli.cpp)
target_link_libraries(aft-mtp-cli ${MTP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/aft-mtp-cli DESTINATION bin)
