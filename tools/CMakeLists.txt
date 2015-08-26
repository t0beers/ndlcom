# some native POSIX tooling is provided. they are optional and don't make sense
# in non-posix environments
include(CheckFunctionExists)
check_function_exists(usleep SEEMS_TO_BE_POSIX)

# only add the tooling-section if we might be compiling in a posix environment
if(SEEMS_TO_BE_POSIX)
    message(STATUS "${PROJECT_NAME}: assuming POSIX and building tools")

    add_executable(ndlcomPacketCreator ndlcomPacketCreator.cpp)
    target_link_libraries(ndlcomPacketCreator ndlcom
        # ubuntu 12.04 needs "rt" explicitly
        rt)

endif(SEEMS_TO_BE_POSIX)
