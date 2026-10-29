########################################################################
# PothosWidgets: graphical control widgets for the Pothos GUI.
# Builds against an installed Pothos core (find_package(Pothos)) and
# Qt 6. The module uses only the public API of the core. Qt comes from
# find_package: from the system, or from a Qt install that the user
# gives with CMAKE_PREFIX_PATH or Qt6_DIR.
########################################################################
cmake_minimum_required(VERSION 3.25)
project(PothosWidgets CXX)

option(WIDGETS_BUILD_TESTS "Register the CTest tests of the widgets" ${PROJECT_IS_TOP_LEVEL})
option(WIDGETS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors (GCC and Clang)" OFF)

#the core defines the variable POTHOS_ROOT (its install prefix), and the
#runtime environment often sets POTHOS_ROOT too. find_package must not use
#them as search prefixes (CMP0144): it could find another core than
#CMAKE_PREFIX_PATH gives. NO_PACKAGE_ROOT_PATH keeps them out.
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

find_package(Pothos CONFIG REQUIRED NO_PACKAGE_ROOT_PATH)

#the core loads modules from its own prefix (POTHOS_ROOT): install there
#when the user does not give CMAKE_INSTALL_PREFIX
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${POTHOS_ROOT}" CACHE PATH "Install prefix (default: the prefix of the Pothos core)" FORCE)
    message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX} (the prefix of the Pothos core)")
endif()

########################################################################
# Qt 6
########################################################################
find_package(Qt6 6.5 CONFIG COMPONENTS Widgets)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "PothosWidgets: Qt 6 (>= 6.5) with the Widgets component is missing.\n"
        "Install the Qt 6 package of the system (for example qt6-base), "
        "or give CMAKE_PREFIX_PATH or Qt6_DIR. Refer to README.md.")
endif()
message(STATUS "PothosWidgets: Qt ${Qt6_VERSION}")

########################################################################
# Module
########################################################################
#widgets_warnings(<target>): the common warnings (GCC and Clang, as the core),
#errors with WIDGETS_WARNINGS_AS_ERRORS (CI)
function(widgets_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(WIDGETS_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

set(SOURCES
    QStringSupport.cpp
    Slider.cpp
    NumericEntry.cpp
    TextDisplay.cpp
    DropDown.cpp
    RadioGroup.cpp
    PlanarSelect.cpp
    ChatBox.cpp
    PushButton.cpp
    MyDoubleSlider.cpp
    Odometer.cpp
    indicator.cpp
    TextEntry.cpp
    ColoredBox.cpp
)

#the self-tests under /widgets/tests, run one by one by widgets.selftests
set(WIDGETS_TESTS
    test_qstring_convert
    test_qstring_serialize
)

POTHOS_MODULE_UTIL(
    TARGET ControlWidgets
    SOURCES ${SOURCES}
    LIBRARIES Qt6::Widgets
    DESTINATION widgets
    ENABLE_DOCS
)
set_target_properties(ControlWidgets PROPERTIES
    AUTOMOC ON
    CXX_EXTENSIONS OFF)
widgets_warnings(ControlWidgets)

#a Qt outside the system dirs (a Qt install of the user or of CI): the
#installed module must find the Qt libraries at run time. Add only the Qt
#library dir to the install RPATH, not all link dirs (the core prefix is
#known to the loader). On Windows the Qt bin dir is on PATH.
set(WIDGETS_QT_LIBDIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}")
if(UNIX AND NOT WIDGETS_QT_LIBDIR IN_LIST CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES)
    set_property(TARGET ControlWidgets APPEND PROPERTY INSTALL_RPATH "${WIDGETS_QT_LIBDIR}")
    message(STATUS "PothosWidgets: install RPATH ${WIDGETS_QT_LIBDIR}")
endif()

foreach(test IN LISTS WIDGETS_TESTS)
    set_property(GLOBAL APPEND PROPERTY POTHOS_WIDGETS_TESTS /widgets/tests/${test})
endforeach()

########################################################################
# Tests: the module is staged into a prefix inside the build tree and
# loaded with POTHOS_PLUGIN_PATH by the PothosUtil of the installed core.
# The widgets need no display: Qt uses the offscreen platform.
########################################################################
if(WIDGETS_BUILD_TESTS)
    enable_testing()
    set(WIDGETS_STAGE_DIR ${PROJECT_BINARY_DIR}/stage)
    set(WIDGETS_SELFTEST_TIMEOUT 600 CACHE STRING "Timeout in seconds for the self-tests")

    add_test(NAME widgets.stage
        COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR} --prefix ${WIDGETS_STAGE_DIR} --config $<CONFIG>)
    set_tests_properties(widgets.stage PROPERTIES FIXTURES_SETUP widgets_stage)

    get_property(widgets_tests GLOBAL PROPERTY POTHOS_WIDGETS_TESTS)
    add_test(NAME widgets.selftests
        COMMAND ${CMAKE_COMMAND}
            -DPREFIX=${POTHOS_ROOT}
            -DTEST_HOME_DIR=${PROJECT_BINARY_DIR}/test-home
            -DBINDIR=bin
            -DLIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DPLUGIN_PATH=${WIDGETS_STAGE_DIR}/${CMAKE_INSTALL_LIBDIR}/Pothos/modules${POTHOS_ABI_VERSION}
            "-DTEST_PATHS=${widgets_tests}"
            -P ${PROJECT_SOURCE_DIR}/cmake/PothosRunSelfTests.cmake)
    set_tests_properties(widgets.selftests PROPERTIES
        FIXTURES_REQUIRED widgets_stage
        ENVIRONMENT QT_QPA_PLATFORM=offscreen
        TIMEOUT ${WIDGETS_SELFTEST_TIMEOUT})
endif()
