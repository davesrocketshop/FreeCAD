add_subdirectory(FastSignals)

# Build SalomeMesh for all Platforms since heavily patched
if (BUILD_SMESH)
    add_subdirectory(salomesmesh)
endif()

add_subdirectory(lazy_loader)

if(NOT FREECAD_USE_EXTERNAL_E57FORMAT)
    add_subdirectory(libE57Format)
endif()

if(BUILD_MATERIAL_EXTERNAL)
    add_subdirectory(lru-cache)
endif()

if (BUILD_ASSEMBLY AND NOT FREECAD_USE_EXTERNAL_ONDSELSOLVER)
    if( NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/OndselSolver/CMakeLists.txt" )
        message(FATAL_ERROR "The OndselSolver git submodule is not available. Please run
        git submodule update --init" )
    endif()
    add_subdirectory(OndselSolver)
endif()

if (BUILD_TRACY_FRAME_PROFILER)
    if( NOT EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/tracy/CMakeLists.txt" )
        message(FATAL_ERROR "The Tracy git directory is not available. Please clone it manually." )
    endif()

    set(TRACY_STATIC OFF)
    add_subdirectory(tracy)
endif()

if(BUILD_QTTESTING)
    set(QT_TESTING_WITH_PYTHON FALSE)

    set(BUILD_EXAMPLES OFF)
    set(QtTesting_QT_VERSION "${FREECAD_QT_MAJOR_VERSION}")
    set(QtTesting_INSTALL_BIN_DIR "${CMAKE_INSTALL_BINDIR}")
    # set(QtTesting_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/vtkqttesting")
    set(QtTesting_INSTALL_LIB_DIR "${CMAKE_INSTALL_LIBDIR}")
    # set(QtTesting_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}")
    set(QtTesting_INSTALL_NO_DEVELOPMENT TRUE)
    set(QT_TESTING_EVENT_PLAYBACK_DELAY 100)
    set(QtTesting_ENABLE_EXAMPLES OFF)
    set(QtTesting_ENABLE_TESTING OFF)
    # set(QT_TESTING_INSTALL_EXPORT_NAME "ParaView")
    set(QTTESTING_BUILD_AS_VTK_MODULE OFF)
    set(BUILD_TESTING OFF)

    add_subdirectory(qttesting)
endif()

