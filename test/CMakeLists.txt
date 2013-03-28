add_executable(test-decoder test-decoder.c)
target_link_libraries(test-decoder NDLCom)
add_test(NAME test-decoder COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-decoder)

# i played a little bit with c++11 -- my first time, actually, while in "production"
# sadly, not all compiler will eat this shit...
# see http://stackoverflow.com/questions/4058565/check-gcc-minor-in-cmake/4065761#4065761
execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
if (GCC_VERSION VERSION_GREATER 4.6 OR GCC_VERSION VERSION_EQUAL 4.6)

    add_executable(timing-decoder timing-decoder.cpp)
    add_executable(encoderDecoder_pingPong encoderDecoder_pingPong.cpp)

    # even if some of them will eat this, they have to be told so, differently
    if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
        set_source_files_properties(timing-decoder.cpp PROPERTIES COMPILE_FLAGS "-std=c++11")
        set_source_files_properties(encoderDecoder_pingPong.cpp PROPERTIES COMPILE_FLAGS "-std=c++11")
    else()
        set_source_files_properties(timing-decoder.cpp PROPERTIES COMPILE_FLAGS "-std=c++0x")
        set_source_files_properties(encoderDecoder_pingPong.cpp PROPERTIES COMPILE_FLAGS "-std=c++0x")
    endif()

    target_link_libraries(timing-decoder NDLCom)
    target_link_libraries(encoderDecoder_pingPong NDLCom)
    add_test(NAME timing-decoder COMMAND ${CMAKE_CURRENT_BINARY_DIR}/timing-decoder 10000000)
    add_test(NAME encoderDecoder_pingPong COMMAND ${CMAKE_CURRENT_BINARY_DIR}/encoderDecoder_pingPong 100000)

endif()
