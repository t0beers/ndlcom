add_executable(test-decoder test-decoder.c)
target_link_libraries(test-decoder NDLCom)
add_test(NAME test-decoder COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test-decoder)

add_executable(timing-decoder timing-decoder.cpp)
# i played a little bit with c++11 -- my first time, actually, while in "production"
set_source_files_properties(timing-decoder.cpp PROPERTIES COMPILE_FLAGS "-std=c++0x")
# the modern compilers (gcc 4.7) renamed this flag:
#set_source_files_properties(timing-decoder.cpp PROPERTIES COMPILE_FLAGS "-std=c++11")
target_link_libraries(timing-decoder NDLCom)
add_test(NAME timing-decoder COMMAND ${CMAKE_CURRENT_BINARY_DIR}/timing-decoder 10000000)
