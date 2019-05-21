cmake_minimum_required(VERSION 2.8)

project(ndlcom)

option(NDLCOM_ENABLE_TESTING
    "will enable testing, add the 'test' subdirectory and add some additional
    compiler-flags for debugging"
    OFF)

# we will need to execute binaries, which is not possible while cross-compiling
if(CMAKE_CROSSCOMPILING AND NDLCOM_ENABLE_TESTING)
    message(FATAL_ERROR "${PROJECT_NAME}: cannot enable testing while in cross-compiling mode")
endif(CMAKE_CROSSCOMPILING AND NDLCOM_ENABLE_TESTING)

if(NDLCOM_ENABLE_TESTING)
    add_definitions(-g -O0 -Wall -Wpedantic)
endif(NDLCOM_ENABLE_TESTING)

# nothing special here
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# the usual create-library-blocks
set(SOURCES_lib
    src/Encoder.c
    src/HeaderPrepare.c
    src/Parser.c
    src/Crc.c
    src/Routing.c
    src/ExternalInterface.c
    src/NodeHandler.c
    src/BridgeHandler.c
    src/Bridge.c
    src/Node.c
)
set(HEADERS_lib
    include/${PROJECT_NAME}/Parser.h
    include/${PROJECT_NAME}/HeaderPrepare.h
    include/${PROJECT_NAME}/Encoder.h
    include/${PROJECT_NAME}/Types.h
    include/${PROJECT_NAME}/Crc.h
    include/${PROJECT_NAME}/Routing.h
    include/${PROJECT_NAME}/ExternalInterface.h
    include/${PROJECT_NAME}/NodeHandler.h
    include/${PROJECT_NAME}/BridgeHandler.h
    include/${PROJECT_NAME}/Bridge.h
    include/${PROJECT_NAME}/Node.h
    include/${PROJECT_NAME}/list.h
)

set (CMAKE_C_FLAGS "-std=gnu11 ${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")

include(CheckFunctionExists)
# the c++ wrapper classes and the native tooling is only intended for usage in
# POSIX based systems. they are optional and don't make sense in non-posix
# environments. 
check_function_exists(usleep SEEMS_TO_BE_POSIX)
if(SEEMS_TO_BE_POSIX)

message("-- ${PROJECT_NAME} detected 'usleep'. Adding tools, c++ wrapper, and increased rx-buffer size")

list(APPEND SOURCES_lib
    src/Bridge.cpp
    src/BridgeHandler.cpp
    src/ExternalInterface.cpp
    src/ExternalInterfaceBase.cpp
    src/Node.cpp
    src/NodeHandler.cpp
    src/InternalHandler.cpp
    src/HandlerCommon.cpp
    src/Payload.cpp
    )
list(APPEND HEADERS_lib
    include/${PROJECT_NAME}/Bridge.hpp
    include/${PROJECT_NAME}/BridgeHandler.hpp
    include/${PROJECT_NAME}/ExternalInterface.hpp
    include/${PROJECT_NAME}/ExternalInterfaceBase.hpp
    include/${PROJECT_NAME}/Node.hpp
    include/${PROJECT_NAME}/NodeHandler.hpp
    include/${PROJECT_NAME}/InternalHandler.hpp
    include/${PROJECT_NAME}/HandlerCommon.hpp
    include/${PROJECT_NAME}/Payload.hpp
    )

    # The buffer-size for reading bytes from an ExternalInterface is increased for
    # "real" platforms, so that our embedded targets don't run out of stack-memory
    # so easy
    set_property(SOURCE src/Bridge.c APPEND PROPERTY
        COMPILE_FLAGS "-DNDLCOM_BRIDGE_TEMPORARY_RXBUFFER_SIZE=4096")

endif(SEEMS_TO_BE_POSIX)

# define the lib
add_library(${PROJECT_NAME}
    ${SOURCES_lib}
    )
#
# installing:
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

if (NOT CMAKE_CROSSCOMPILING)
    # for use in shared libraries (MARS, Rock and so on). deliberately only
    # added in the "non-cross" case to keep codesizes small in embedded.
    add_definitions(-fPIC)
endif (NOT CMAKE_CROSSCOMPILING)

# some tooling for debugging and playing around
add_subdirectory(tools)

# testing has to be enabled explicitly!
if(NOT CMAKE_CROSSCOMPILING AND NDLCOM_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif(NOT CMAKE_CROSSCOMPILING AND NDLCOM_ENABLE_TESTING)

# doxygen:
configure_file(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(${PROJECT_NAME}-doc doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile && echo
    "-- ${PROJECT_NAME}: Documentation generated in ${CMAKE_CURRENT_BINARY_DIR}/html/index.html")
if(NOT TARGET doc)
    add_custom_target(doc)
endif(NOT TARGET doc)
add_dependencies(doc ${PROJECT_NAME}-doc)
# and install doxygen stuff as well.
# NOTE: cannot install documentation without calling "doc" before. looks like
# this bug: https://cmake.org/Bug/view.php?id=8438
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    DESTINATION share/doc/${PROJECT_NAME}
    OPTIONAL)

# pkg-config, to be installed:
configure_file(${PROJECT_NAME}.pc.in
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
# pkg-config for use of this library inside the buildtree of another project
# (via add_subdirectory)
configure_file(${PROJECT_NAME}-uninstalled.pc.in
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)

# all the headers to be installed (this includes the generated one)
install(FILES ${HEADERS_lib}
    DESTINATION include/${PROJECT_NAME})
# the pkg-config for external use has also to be installed
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION lib/pkgconfig)
