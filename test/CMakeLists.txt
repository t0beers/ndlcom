add_executable(test-decoder test-decoder.c)
target_link_libraries(test-decoder NDLCom)
add_test(NAME test-decoder COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-decoder)
