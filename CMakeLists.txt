cmake_minimum_required(VERSION 2.8)

project(ndlcom_core)

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
#
# TODO: due to internet outtage, the link to cmake-mailing list where exactly
# this question was asked (and answered) could not be looked up...
set_source_files_properties(${DEVICE_IDS_C} PROPERTIES GENERATED true)
set_source_files_properties(${DEVICE_IDS_H} PROPERTIES GENERATED true)
# to find the generated "DeviceIds.h"
include_directories(${CMAKE_BINARY_DIR}/xml/include)
# you did it. from now on, the rest of the cmakelists is quite ordinary

# nothing special here
include_directories(include)

# the usual create-library-blocks
set(SOURCES_lib
    src/Encoder.c
    src/Header.c
    src/Parser.c
    src/Crc.c
    ${DEVICE_IDS_C}
)
set(HEADERS_lib
    include/${PROJECT_NAME}/Protocol.h
    include/${PROJECT_NAME}/Header.h
    include/${PROJECT_NAME}/ParserState.h
    include/${PROJECT_NAME}/Types.h
    include/${PROJECT_NAME}/Crc.h
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
# pkg-config for use inside the buildtree
configure_file(${PROJECT_NAME}-uninstalled.pc.in
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)

# all the headers to be installed (this includes the generated one)
install(FILES ${HEADERS_lib}
    DESTINATION include/${PROJECT_NAME})
# the pkg-config for external use has also to be installed
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION lib/pkgconfig)
