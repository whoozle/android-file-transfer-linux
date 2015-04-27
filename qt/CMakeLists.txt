project(mtp-qt)

find_package(Qt5Widgets QUIET)
find_package(Qt4 QUIET)

set(SOURCES main.cpp
    mainwindow.cpp
    fileuploader.cpp
    mtpobjectsmodel.cpp
    progressdialog.cpp
    createdirectorydialog.cpp
    renamedialog.cpp
    qtobjectstream.cpp
)

set(HEADERS mainwindow.h
    fileuploader.h
    mtpobjectsmodel.h
    progressdialog.h
    createdirectorydialog.h
    renamedialog.h
    qtobjectstream.h
)

set(FORMS mainwindow.ui
    progressdialog.ui
    createdirectorydialog.ui
    renamedialog.ui)

if (Qt5Widgets_FOUND)
    message(STATUS "Using Qt5")
    qt5_wrap_cpp(HEADERS_MOC ${HEADERS})
    qt5_wrap_ui(FORMS_HEADERS ${FORMS})
    qt5_add_resources(RESOURCES android-file-transfer.qrc)
    include_directories(${Qt5Widgets_INCLUDE_DIRS})
    set(EXTRA_QT_LINK ${Qt5Widgets_LIBRARIES})
elseif(Qt4_FOUND OR QT_FOUND OR QT4_FOUND)
    message(STATUS "Using Qt4")
    qt4_wrap_cpp(HEADERS_MOC ${HEADERS})
    qt4_wrap_ui(FORMS_HEADERS ${FORMS})
    qt4_add_resources(RESOURCES android-file-transfer.qrc)
    include(${QT_USE_FILE})
    set(EXTRA_QT_LINK ${QT_LIBRARIES})
else()
    message(FATAL_ERROR "No Qt found")
endif()

add_definitions(${QT_DEFINITIONS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(android-file-transfer ${SOURCES} ${HEADERS_MOC} ${FORMS_HEADERS} ${RESOURCES})
target_link_libraries(android-file-transfer ${EXTRA_QT_LINK} ${MTP_LIBRARIES})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/android-file-transfer DESTINATION bin)
