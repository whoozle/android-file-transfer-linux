project(mtp-qt)
if (USE_QT5)
    find_package(Qt5Widgets)
else()
    find_package(Qt4)
endif()

set(SOURCES main.cpp
    mainwindow.cpp
    fileuploader.cpp
    mtpobjectsmodel.cpp
    progressdialog.cpp
    createdirectorydialog.cpp
    renamedialog.cpp
)

set(HEADERS mainwindow.h
    fileuploader.h
    mtpobjectsmodel.h
    progressdialog.h
    createdirectorydialog.h
    renamedialog.h
)

set(FORMS mainwindow.ui
    progressdialog.ui
    createdirectorydialog.ui
    renamedialog.ui)

if (USE_QT5)
    qt5_wrap_cpp(HEADERS_MOC ${HEADERS})
    qt5_wrap_ui(FORMS_HEADERS ${FORMS})
    include_directories(${Qt5Widgets_INCLUDE_DIRS})
    set(EXTRA_QT_LINK ${Qt5Widgets_LIBRARIES})
else()
    qt4_wrap_cpp(HEADERS_MOC ${HEADERS})
    qt4_wrap_ui(FORMS_HEADERS ${FORMS})
    include(${QT_USE_FILE})
    set(EXTRA_QT_LINK ${QT_LIBRARIES})
endif()
add_definitions(${QT_DEFINITIONS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(android-file-transfer ${SOURCES} ${HEADERS_MOC} ${FORMS_HEADERS})
target_link_libraries(android-file-transfer ${EXTRA_QT_LINK} ${MTP_LIBRARIES})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/android-file-transfer DESTINATION bin)
