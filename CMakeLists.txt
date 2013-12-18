cmake_minimum_required(VERSION 2.8)

project(ndlcom_core)

include_directories(include)

# and followed by the usual create-library-blocks
set(SOURCES_lib
    src/Encoder.c
    src/Header.c
    src/Parser.c
)
set(HEADERS_lib
    include/${PROJECT_NAME}/Protocol.h
    include/${PROJECT_NAME}/Header.h
    include/${PROJECT_NAME}/ParserState.h
    include/${PROJECT_NAME}/Types.h
)

# define the lib
add_library(${PROJECT_NAME} SHARED
    ${SOURCES_lib}
    )

# installing:
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${HEADERS_lib} DESTINATION include/${PROJECT_NAME})

# testing the decoder with some corner-cases
if (NOT CMAKE_CROSSCOMPILING)
    # this has to be enabled explicitly! either here or in the "main" CMakeLists, including this one
    #enable_testing()
    add_subdirectory(test)
endif (NOT CMAKE_CROSSCOMPILING)

# pkg-config, to be installed:
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
# pkg-config for use inside our buildtree
configure_file(${PROJECT_NAME}-uninstalled.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)
