
if(${PROJECT_NAME}_DEVICE_IDS)
    set(DEVICES_XML
        ${CMAKE_CURRENT_SOURCE_DIR}/${${PROJECT_NAME}_DEVICE_IDS}.xml)

    message("-- ${PROJECT_NAME}: using DeviceIds xml-file "${DEVICES_XML}  )

    if(NOT EXISTS ${DEVICES_XML})
        message(FATAL_ERROR "-- ${PROJECT_NAME}: xml-file ${DEVICES_XML} does not exist")
    endif()
else()
    message(FATAL_ERROR "-- ${PROJECT_NAME}: set cmake variable \"${PROJECT_NAME}_DEVICE_IDS\" in some top-level CMakeLists.txt (name of the file without ending)")
endif()


# for the generated DeviceId-files, we need a tool named xmlstarlet. not
# everyone has this installed on his computer, so we check for them to remind
# them...
find_program(XMLSTARLET xmlstarlet)
if(${XMLSTARLET} STREQUAL "XMLSTARLET-NOTFOUND")
    message(FATAL_ERROR "${PROJECT_NAME}: you have to install 'xmlstarlet' -- doing somethink like 'apt-get install xmlstarlet' on a debian system")
endif(${XMLSTARLET} STREQUAL "XMLSTARLET-NOTFOUND")

# the named xml source files:
#set(DEVICES_XML ${CMAKE_CURRENT_SOURCE_DIR}/DeviceIds.xml)
# the schema describing the device-id database, used for validation
set(DEVICES_XSD ${CMAKE_CURRENT_SOURCE_DIR}/schema/DeviceIds.xsd)
# and a temporary file we create to indicate a successfull validation of the schema
set(DEVICES_XML_VALIDATED ${CMAKE_CURRENT_BINARY_DIR}/DeviceIds.xml.validated)

# the transforming xml stuff:
set(DEVICE_IDS_C_XSL    ${CMAKE_CURRENT_SOURCE_DIR}/generate/DeviceIds.c.xsl)
set(DEVICE_IDS_H_XSL    ${CMAKE_CURRENT_SOURCE_DIR}/generate/DeviceIds.h.xsl)
set(DEVICE_IDS_VHD_XSL  ${CMAKE_CURRENT_SOURCE_DIR}/generate/Devices.vhd.xsl)
set(DEVICE_IDS_PERL_XSL ${CMAKE_CURRENT_SOURCE_DIR}/generate/NDLComDeviceIds.pm.xsl)
set(DEVICE_IDS_YML_XSL  ${CMAKE_CURRENT_SOURCE_DIR}/generate/NDLComDeviceIds.yml.xsl)

# and the resulting files we wanna create:
set(DEVICE_IDS_C        ${CMAKE_CURRENT_BINARY_DIR}/src/DeviceIds.c)
set(DEVICE_IDS_H        ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/DeviceIds.h)
set(DEVICE_IDS_VHD         ${CMAKE_CURRENT_BINARY_DIR}/vhdl/Devices.vhd)
set(DEVICE_IDS_PERL     ${CMAKE_CURRENT_BINARY_DIR}/perl/NDLComDeviceIds.pm)
set(DEVICE_IDS_YML      ${CMAKE_CURRENT_BINARY_DIR}/yml/NDLComDeviceIds.yml)

##
#  plumbing area. go away.
##

# the block to do that validation
add_custom_command(
    OUTPUT ${DEVICES_XML_VALIDATED}
    COMMAND mkdir -p `dirname ${DEVICES_XML_VALIDATED}`
    COMMAND xmllint --noout --schema ${DEVICES_XSD} ${DEVICES_XML} && touch ${DEVICES_XML_VALIDATED}
    DEPENDS ${DEVICES_XML} ${DEVICES_XSD}
    )
add_custom_target(${PROJECT_NAME}-validate)
add_dependencies(${PROJECT_NAME}-validate ${DEVICES_XML_VALIDATED})


# this macro uses the following script-local variable:
#    ${DEVICES_XML}
#    ${XMLSTARLET}
macro(add_custom_command_generated_from_xml xslfile outfile)
    # "generate" command to create a custom file using the provided xml and a
    # xsl file. the ".. || rm $file" block after the xmlstarlet command is to
    # remove half-created files on failure
    add_custom_command(
        OUTPUT ${outfile}
        COMMAND mkdir -p `dirname ${outfile}`
        COMMAND ${XMLSTARLET} tr ${xslfile} ${DEVICES_XML} > ${outfile} || rm ${outfile}
        DEPENDS ${DEVICES_XML} ${xslfile}
        )

    # cleaning up the created stuff, so we don't wanna leave a mess
    set_directory_properties(PROPERTIES
        ADDITIONAL_MAKE_CLEAN_FILES ${outfile}
        )

    # somehow using "add_dependencies(${PROJECT_NAME}-generate ${outfile})" for
    # each file independently does not work as expected?

endmacro()

# create a custom target from this, so that cmake can work with this
add_custom_target(${PROJECT_NAME}-generate ALL DEPENDS
    ${DEVICE_IDS_C}
    ${DEVICE_IDS_H}
    ${DEVICE_IDS_VHD}
    ${DEVICE_IDS_PERL}
    ${DEVICE_IDS_YML}
    )
add_dependencies(${PROJECT_NAME}-generate ${PROJECT_NAME}-validate)

# and call the macro to get the generator-expressions into the makefil
add_custom_command_generated_from_xml(${DEVICE_IDS_C_XSL}    ${DEVICE_IDS_C})
add_custom_command_generated_from_xml(${DEVICE_IDS_H_XSL}    ${DEVICE_IDS_H})
add_custom_command_generated_from_xml(${DEVICE_IDS_VHD_XSL}  ${DEVICE_IDS_VHD})
add_custom_command_generated_from_xml(${DEVICE_IDS_PERL_XSL} ${DEVICE_IDS_PERL})
add_custom_command_generated_from_xml(${DEVICE_IDS_YML_XSL}  ${DEVICE_IDS_YML})

# some of the stuff is needed by others. provide it!
# the choosen directory names are not representative, and might as well be changed
install(FILES ${DEVICE_IDS_H}    DESTINATION include/${PROJECT_NAME})
install(FILES ${DEVICE_IDS_PERL} DESTINATION lib/perl/${PROJECT_NAME})
install(FILES ${DEVICE_IDS_VHD}  DESTINATION lib/vhdl/${PROJECT_NAME})
#install(FILES ${DEVICE_IDS_RB}  DESTINATION lib/ruby/${PROJECT_NAME})
