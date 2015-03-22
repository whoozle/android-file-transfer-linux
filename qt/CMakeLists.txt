project(mtp-qt)
find_package(Qt4)

set(SOURCES main.cpp
    mainwindow.cpp
    mtpobjectsmodel.cpp
    progressdialog.cpp
    createdirectorydialog.cpp
    renamedialog.cpp
)

set(HEADERS mainwindow.h
    mtpobjectsmodel.h
    progressdialog.h
    createdirectorydialog.h
    renamedialog.h
)

set(FORMS mainwindow.ui
    progressdialog.ui
    createdirectorydialog.ui
    renamedialog.ui)

qt4_wrap_cpp(HEADERS_MOC ${HEADERS})
qt4_wrap_ui(FORMS_HEADERS ${FORMS})

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

add_executable(qt-mtp ${SOURCES} ${HEADERS_MOC} ${FORMS_HEADERS})
target_link_libraries(qt-mtp ${QT_LIBRARIES} ${MTP_LIBRARIES})
