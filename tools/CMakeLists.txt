# some native tooling for POSIX based systems is provided. they are optional
# and don't make sense in non-posix environments
check_function_exists(usleep SEEMS_TO_BE_POSIX)

# only add the tooling-section if we might be compiling in a posix environment
if(SEEMS_TO_BE_POSIX)
    message(STATUS "${PROJECT_NAME}: assuming POSIX and building tools")

    add_executable(ndlcomPacketProducer ndlcomPacketProducer.cpp)
    target_link_libraries(ndlcomPacketProducer ndlcom
        # ubuntu 12.04 needs "rt" explicitly
        rt)

    add_executable(ndlcomPacketConsumer ndlcomPacketConsumer.cpp)
    target_link_libraries(ndlcomPacketConsumer ndlcom
        # ubuntu 12.04 needs "rt" explicitly
        rt)

    add_executable(ndlcomBridge
        ndlcomBridge.cpp
        )
    target_link_libraries(ndlcomBridge
        ndlcom
        # ubuntu 12.04 needs "rt" explicitly
        rt
        )

    add_executable(minimalExample_cpp minimalExample.cpp)
    target_link_libraries(minimalExample_cpp ndlcom)
    add_executable(minimalExample_c minimalExample.c)
    target_link_libraries(minimalExample_c ndlcom)

    # TODO: write and add a shellbased-test-script which couples multiple
    # "ndlcomBridge", "ndlcomPacketProducer" and "ndlcomPacketConsumer"
    # together to tests some things

    # also install the tools
    install(TARGETS ndlcomBridge ndlcomPacketConsumer ndlcomPacketProducer
        RUNTIME DESTINATION bin)

endif(SEEMS_TO_BE_POSIX)
