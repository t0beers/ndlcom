cmake_minimum_required(VERSION 2.8)

project(NDLCom)

include_directories(include)

set(SOURCES_lib
	src/ProtocolEncode.c
	src/ProtocolParser.c
)

set(HEADERS_lib
	include/${PROJECT_NAME}/Protocol.h
	include/${PROJECT_NAME}/Header.h
	include/${PROJECT_NAME}/ParserState.h
)

# define lib
add_library(${PROJECT_NAME}
    ${SOURCES_lib}
    ${HEADERS_lib}
    )

set_target_properties(${PROJECT_NAME} PROPERTIES
    COMPILE_FLAGS "-fPIC"       #for use in MARS plugin (which is a shared library)
)

# installing:
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${HEADERS_lib} DESTINATION include/${PROJECT_NAME})

# still and extra subdir
add_subdirectory(test)

# pkg-config, to be installed:
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
# pkg-config for use inside our buildtree
configure_file(${PROJECT_NAME}-uninstalled.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)
