cmake_minimum_required(VERSION 2.8)

project(ndlcom)

# nothing special here
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# the usual create-library-blocks
set(SOURCES_lib
    src/Encoder.c
    src/HeaderPrepare.c
    src/Parser.c
    src/Crc.c
    src/Routing.c
)
set(HEADERS_lib
    include/${PROJECT_NAME}/Parser.h
    include/${PROJECT_NAME}/HeaderPrepare.h
    include/${PROJECT_NAME}/ParserState.h
    include/${PROJECT_NAME}/Encoder.h
    include/${PROJECT_NAME}/Types.h
    include/${PROJECT_NAME}/Crc.h
    include/${PROJECT_NAME}/Routing.h
)

# define the lib
add_library(${PROJECT_NAME}
    ${SOURCES_lib}
    )
#
# installing:
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION lib
    LIBRARY DESTINATION lib
)

if (NOT CMAKE_CROSSCOMPILING)
    # for use in shared libraries (MARS, Rock and so on). deliberately only
    # added in the "non-cross" case to keep codesizes small in embedded.
    add_definitions(-fPIC)
    # testing has to be enabled explicitly!
    #enable_testing()
    #add_subdirectory(test)
    add_subdirectory(tools)
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
