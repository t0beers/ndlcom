cmake_minimum_required(VERSION 2.8)

project(ndlcom)

# nothing special here
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# adding fPIC to be able to later link as part of a shared library
add_definitions(-fPIC)

# the usual create-library-blocks
set(SOURCES_lib
    src/Message.cpp
    src/Encoder.c
    src/Header.c
    src/Parser.c
    src/Crc.c
)
set(HEADERS_lib
    include/${PROJECT_NAME}/Message.hpp
    include/${PROJECT_NAME}/Protocol.h
    include/${PROJECT_NAME}/Header.h
    include/${PROJECT_NAME}/ParserState.h
    include/${PROJECT_NAME}/Types.h
    include/${PROJECT_NAME}/Crc.h
    # this is a templated "static routing table". if you don't need it, just
    # don't include it. in this case, your compiler would never see this file
    include/${PROJECT_NAME}/RoutingTable.hpp
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
    set_target_properties(${PROJECT_NAME} PROPERTIES
        COMPILE_FLAGS "-fPIC"       #for use in shared libraries (MARS, Rock and so on)
    )
    # testing has to be enabled explicitly!
    enable_testing()
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
