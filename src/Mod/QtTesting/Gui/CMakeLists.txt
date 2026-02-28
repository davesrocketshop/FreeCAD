# SPDX-License-Identifier: LGPL-2.1-or-later

set(QtTestingGui_LIBS
    qttesting
    FreeCADGui
)

generate_from_py(QtTestUtility)

SET(Python_SRCS
    QtTestUtility.pyi
    QtTestUtilityPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(QtTestingGui_SRCS
    ${Python_SRCS}
    AppQtTesting.cpp
    Command.cpp
    QtTestUtility.cpp
    QtTestUtility.h
    XMLEventObserver.cpp
    XMLEventObserver.h
    XMLEventSource.cpp
    XMLEventSource.h
)

add_library(QtTestingGui SHARED
    ${QtTestingGui_SRCS}
)

target_include_directories(
    QtTestingGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(QtTestingGui ${QtTestingGui_LIBS})

if (MSVC)
    target_compile_options(QtTestingGui PRIVATE /wd4251 /wd4273)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(QtTestingGui)
endif()

SET_BIN_DIR(QtTestingGui QtTestingGui /Mod/QtTesting)
SET_PYTHON_PREFIX_SUFFIX(QtTestingGui)

INSTALL(TARGETS QtTestingGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
