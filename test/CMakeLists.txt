# i played a little bit with c++11 -- my first time, actually, while in "production"
# sadly, not all compiler will eat this shit...
# see http://stackoverflow.com/questions/4058565/check-gcc-minor-in-cmake/4065761#4065761
execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
if (GCC_VERSION VERSION_GREATER 4.6 OR GCC_VERSION VERSION_EQUAL 4.6)

    add_executable(timing-decoder timing-decoder.cpp)
    add_executable(encoderDecoder_pingPong encoderDecoder_pingPong.cpp)

    # even if some of the compilers will eat the code, they have to be told so, differently
    if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
        set_source_files_properties(timing-decoder.cpp PROPERTIES COMPILE_FLAGS "-std=c++11")
        set_source_files_properties(encoderDecoder_pingPong.cpp PROPERTIES COMPILE_FLAGS "-std=c++11")
    else()
        set_source_files_properties(timing-decoder.cpp PROPERTIES COMPILE_FLAGS "-std=c++0x")
        set_source_files_properties(encoderDecoder_pingPong.cpp PROPERTIES COMPILE_FLAGS "-std=c++0x")
    endif()

    target_link_libraries(timing-decoder ndlcom)
    target_link_libraries(encoderDecoder_pingPong ndlcom rt)
    add_test(NAME timing-decoder COMMAND ${CMAKE_CURRENT_BINARY_DIR}/timing-decoder 10000000)
    add_test(NAME encoderDecoder_pingPong COMMAND ${CMAKE_CURRENT_BINARY_DIR}/encoderDecoder_pingPong 100000)

endif()

# these are two executales, each of them encapsulating the encoder/decoder in a
# small cmdline tool. both read from characters from stdin in a certain format
# (see the respective cpp-files), try to do their tasks and output to stdout in
# their respective other format.
#
# q auick note: the two tools are not perfect, and don't catch all sidecases.
# but they are a start into automatic testing and comparing vhdl/cpp code on
# the desk, not the robot!
add_executable(testEncoder testEncoder.cpp)
target_link_libraries(testEncoder ndlcom)
add_executable(testDecoder testDecoder.cpp)
target_link_libraries(testDecoder ndlcom)

# simple c-program that does some common checks with the ndlcomDoCrc-function.
# intended to show the weakness of the current 8bit-xor and for later use with
# the crc16...
add_executable(testCrc testCrc.c)
target_link_libraries(testCrc ndlcom)

# will print the precomputed table for the crc16
add_executable(printTable printTable.c)

# i hate cmake nowadays, their "language" is broken as shit and trying to
# formulate a "task" such as these trivally done in bash is hard... since that,
# i hacked together this mini-shell script which does exactly the same as the
# test-macro...
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testEncoderDecoder.sh.in ${CMAKE_CURRENT_BINARY_DIR}/testEncoderDecoder.sh @ONLY)
add_test(NAME testEncoderDecoder COMMAND ${CMAKE_CURRENT_BINARY_DIR}/testEncoderDecoder.sh)
