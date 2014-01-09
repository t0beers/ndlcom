cmake_minimum_required(VERSION 2.8)

project(ndlcom_core)

include_directories(include)

# we are generating header files using a xml -- they are put into this directory:
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# for the generated DeviceId-files, we need a tool named xmlstarlet. not
# everyone has this installed on his computer, so we check for them to remind
# them...
find_program(XMLSTARLET xmlstarlet)
if(${XMLSTARLET} STREQUAL "XMLSTARLET-NOTFOUND")
    message(FATAL_ERROR "${PROJECT_NAME}: install xmlstarlet -- doing somethink like 'apt-get install xmlstarlet'")
endif(${XMLSTARLET} STREQUAL "XMLSTARLET-NOTFOUND")

# the named xml source files:
set(DEVICES_XML ${CMAKE_CURRENT_SOURCE_DIR}/xml/DeviceIds.xml)
set(DEVICES_XSD ${CMAKE_CURRENT_SOURCE_DIR}/xml/DeviceIds.xsd)
# and a temporary file we create to indicate a successfull validation of the schema
set(DEVICES_XML_VALIDATED ${CMAKE_CURRENT_BINARY_DIR}/xml/DeviceIds.xml.validated)

# the transforming xml stuff:
set(DEVICE_IDS_C_XSL ${CMAKE_CURRENT_SOURCE_DIR}/xml/DeviceIds.c.xsl)
set(DEVICE_IDS_H_CSL ${CMAKE_CURRENT_SOURCE_DIR}/xml/DeviceIds.h.xsl)
set(DEVICES_VHD_XSL ${CMAKE_CURRENT_SOURCE_DIR}/xml/Devices.vhd.xsl)
set(DEVICE_IDS_PERL_XSL ${CMAKE_CURRENT_SOURCE_DIR}/xml/NDLComDeviceIds.pm.xsl)
set(DEVICE_IDS_YML_XSL ${CMAKE_CURRENT_SOURCE_DIR}/xml/NDLComDeviceIds.yml.xsl)
# and the resulting files we wanna create:
set(DEVICE_IDS_C ${CMAKE_CURRENT_BINARY_DIR}/src/DeviceIds.c)
set(DEVICE_IDS_H ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/DeviceIds.h)
# ATTENTION: there are in the source-tree...
set(DEVICES_VHD ${CMAKE_CURRENT_SOURCE_DIR}/vhdl/Devices.vhd)
set(DEVICE_IDS_PERL ${CMAKE_CURRENT_SOURCE_DIR}/perl/NDLComDeviceIds.pm)
set(DEVICE_IDS_YML ${CMAKE_CURRENT_SOURCE_DIR}/yml/NDLComDeviceIds.yml)

# and the block to do that validation
add_custom_command(
    OUTPUT ${DEVICES_XML_VALIDATED}
    COMMAND mkdir -p `dirname ${DEVICES_XML_VALIDATED}`
    COMMAND xmllint --noout --schema ${DEVICES_XSD} ${DEVICES_XML} && touch ${DEVICES_XML_VALIDATED}
    DEPENDS ${DEVICES_XML} ${DEVICES_XSD}
    )
add_custom_target(${PROJECT_NAME}-validate)
add_dependencies(${PROJECT_NAME}-validate ${DEVICES_XML_VALIDATED})

# three "generate" commands to create c, h and vhd files using the provided xml
# infrastructure. using ".. || rm $file" to remove half-created files on failure
add_custom_command(
    OUTPUT ${DEVICE_IDS_C}
    COMMAND mkdir -p `dirname ${DEVICE_IDS_C}`
    COMMAND ${XMLSTARLET} tr ${DEVICE_IDS_C_XSL} ${DEVICES_XML} > ${DEVICE_IDS_C} || rm ${DEVICE_IDS_C}
    DEPENDS ${DEVICES_XML} ${DEVICE_IDS_C_XSL}
    )
add_custom_command(
    OUTPUT ${DEVICE_IDS_H}
    COMMAND mkdir -p `dirname ${DEVICE_IDS_H}`
    COMMAND ${XMLSTARLET} tr ${DEVICE_IDS_H_CSL} ${DEVICES_XML} > ${DEVICE_IDS_H} || rm ${DEVICE_IDS_H}
    DEPENDS ${DEVICES_XML} ${DEVICE_IDS_H_CSL}
    )
add_custom_command(
    OUTPUT ${DEVICES_VHD}
    COMMAND mkdir -p `dirname ${DEVICES_VHD}`
    COMMAND ${XMLSTARLET} tr ${DEVICES_VHD_XSL} ${DEVICES_XML} > ${DEVICES_VHD} || rm ${DEVICES_VHD}
    DEPENDS ${DEVICES_XML} ${DEVICES_VHD_XSL}
    )
add_custom_command(
    OUTPUT ${DEVICE_IDS_PERL}
    COMMAND mkdir -p `dirname ${DEVICE_IDS_PERL}`
    COMMAND ${XMLSTARLET} tr ${DEVICE_IDS_PERL_XSL} ${DEVICES_XML} > ${DEVICE_IDS_PERL} || rm ${DEVICE_IDS_PERL}
    DEPENDS ${DEVICES_XML} ${DEVICE_IDS_PERL_XSL}
    )
add_custom_command(
    OUTPUT ${DEVICE_IDS_YML}
    COMMAND mkdir -p `dirname ${DEVICE_IDS_YML}`
    COMMAND ${XMLSTARLET} tr ${DEVICE_IDS_YML_XSL} ${DEVICES_XML} > ${DEVICE_IDS_YML} || rm ${DEVICE_IDS_YML}
    DEPENDS ${DEVICES_XML} ${DEVICE_IDS_YML_XSL}
    )

# and create a custom target from this, so that cmake can work with this
add_custom_target(${PROJECT_NAME}-generate ALL DEPENDS ${DEVICES_VHD} ${DEVICE_IDS_PERL} ${DEVICE_IDS_YML})
add_dependencies(${PROJECT_NAME}-generate ${PROJECT_NAME}-validate ${DEVICE_IDS_C} ${DEVICE_IDS_H} ${DEVICES_VHD} ${DEVICE_IDS_PERL} ${DEVICE_IDS_YML})

# allows cleaning the created stuff, so we don't leave a mess
set_directory_properties(PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES ${DEVICES_XML_VALIDATED}
    ADDITIONAL_MAKE_CLEAN_FILES ${DEVICE_IDS_C}
    ADDITIONAL_MAKE_CLEAN_FILES ${DEVICE_IDS_H}
    ADDITIONAL_MAKE_CLEAN_FILES ${DEVICES_VHD}
    ADDITIONAL_MAKE_CLEAN_FILES ${DEVICE_IDS_PERL}
    ADDITIONAL_MAKE_CLEAN_FILES ${DEVICE_IDS_YML}
    )

# and followed by the usual create-library-blocks
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
    ${DEVICE_IDS_H}
)

# define the lib
add_library(${PROJECT_NAME} SHARED
    ${SOURCES_lib}
    ${HEADERS_lib}
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
