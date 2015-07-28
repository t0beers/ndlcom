# i played a little bit with c++11 -- my first time, actually, while in
# "production" sadly, not all compiler will eat this shit...
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++0x" HAS_CPP_0X)
check_cxx_compiler_flag("-std=c++11" HAS_CPP_11)

if(HAS_CPP_11 OR HAS_CPP_0X)
    add_executable(timing-decoder timing-decoder.cpp)
    add_executable(encoderDecoder_pingPong encoderDecoder_pingPong.cpp)
    if (HAS_CPP_11)
        set_source_files_properties(timing-decoder.cpp
            PROPERTIES COMPILE_FLAGS "-std=c++11")
        set_source_files_properties(encoderDecoder_pingPong.cpp
            PROPERTIES COMPILE_FLAGS "-std=c++11")
    else()
        set_source_files_properties(timing-decoder.cpp
            PROPERTIES COMPILE_FLAGS "-std=c++0x")
        set_source_files_properties(encoderDecoder_pingPong.cpp
            PROPERTIES COMPILE_FLAGS "-std=c++0x")
    endif()

    target_link_libraries(timing-decoder ndlcom)
    target_link_libraries(encoderDecoder_pingPong ndlcom rt)
    add_test(NAME timing-decoder
        COMMAND timing-decoder 10000000)
    add_test(NAME encoderDecoder_pingPong
        COMMAND encoderDecoder_pingPong 100000)

endif()

# these are two executales, each of them encapsulating the encoder/decoder in a
# small cmdline tool. both read from characters from stdin in a certain format
# (see the respective cpp-files), try to do their tasks and output to stdout in
# their respective other format.
#
# quick note: the two tools are not perfect, and don't catch all sidecases.
# but they are a start into automatic testing and comparing vhdl/cpp code on
# the desk, not the robot!
#
# both are used in the "testEncoderDecoder" test
add_executable(testEncoder testEncoder.cpp)
target_link_libraries(testEncoder ndlcom)
add_executable(testDecoder testDecoder.cpp)
target_link_libraries(testDecoder ndlcom)

# simple c-program that does some common checks with the ndlcomDoCrc-function.
# intended to show the weakness of the current 8bit-xor and for later use with
# the crc16...
add_executable(testCrc testCrc.c)
target_link_libraries(testCrc ndlcom)
add_test(NAME testCrc COMMAND testCrc)

# will print the precomputed table for the crc16
add_executable(printTable printTable.c)
add_test(NAME printTable COMMAND printTable)

add_executable(ndlcomEncode ndlcomEncode.cpp)
target_link_libraries(ndlcomEncode ndlcom)

# i hate cmake nowadays, their "language" is broken as shit and trying to
# formulate a "task" such as these trivally done in bash is hard... since that,
# i hacked together this mini-shell script which does exactly the same as the
# test-macro...
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testEncoderDecoder.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/testEncoderDecoder.sh @ONLY)
add_test(NAME testEncoderDecoder
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/testEncoderDecoder.sh)
