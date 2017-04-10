INCLUDE_DIRECTORIES(${GTEST_INCLUDE})
FILE(GLOB TEST_FILES test_main.cpp test_telnet.cpp test_restful.cpp test_terminalmessage.cpp)
ADD_DEFINITIONS(
    ${G_COVERAGE}
    -D__TEST__
    -g
)

ADD_EXECUTABLE(test_oamroutersrv ${TEST_FILES} ${SRC_FILES})
TARGET_LINK_LIBRARIES(test_oamroutersrv ${OAM_ROUTER_LIBS} ${GTEST_LIBS} gcov)
