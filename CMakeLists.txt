cmake_minimum_required(VERSION 2.8)

project(ndlcom)

# uiuiui... somebody will hang mir for this ;-)
#
# this submodule will look at the cmake-variable which hopefully defines the
# $NAME.xml to actually use for generating some c/h files and some other
# exports.
add_subdirectory(xml)

# this little block will take care that we use the generated c-file inside the
# compiled library
set(DEVICE_IDS_C ${CMAKE_BINARY_DIR}/xml/src/DeviceIds.c)
# this takes care that cmake tries to create the source file prior to compiling it
# see also http://www.cmake.org/pipermail/cmake/2014-January/056738.html
set_source_files_properties(${DEVICE_IDS_C} PROPERTIES GENERATED true)
set_source_files_properties(${DEVICE_IDS_H} PROPERTIES GENERATED true)
# to find the generated "DeviceIds.h" for use during compiling of the lib
include_directories(${CMAKE_BINARY_DIR}/xml/include)
# so. you did it. from now on, the rest of the cmakelists is quite ordinary...

# nothing special here
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# the usual create-library-blocks
set(SOURCES_lib
    src/Encoder.c
    src/Header.c
    src/Parser.c
    src/Crc.c
    # the generated c-file, containing a const char-array for a runtime mapping
    # of deviceName<->deviceId
    ${DEVICE_IDS_C}
)
set(HEADERS_lib
    include/${PROJECT_NAME}/Protocol.h
    include/${PROJECT_NAME}/Header.h
    include/${PROJECT_NAME}/ParserState.h
    include/${PROJECT_NAME}/Types.h
    include/${PROJECT_NAME}/Crc.h
    # the generated header file, containing defines (enums in fact) for a
    # compile-time-safe mapping of deviceName<->deviceId
    ${DEVICE_IDS_H}
    # this is a templated "static routing table". if you don't need it, just
    # don't include it. in this case, your compiler would never see this file
    include/${PROJECT_NAME}/RoutingTable.hpp
)

# define the lib
add_library(${PROJECT_NAME} SHARED
    ${SOURCES_lib}
    )

# this is needed explicitly to have the generated source/header files ready
# when we begin compiling the actual library
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-generate)

# installing:
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION lib
    LIBRARY DESTINATION lib
)

# testing the code with some corner-cases
if (NOT CMAKE_CROSSCOMPILING)
    # testing has to be enabled explicitly!
    #enable_testing()
    add_subdirectory(test)
endif (NOT CMAKE_CROSSCOMPILING)

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
