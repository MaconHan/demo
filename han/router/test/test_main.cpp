#ifdef __TEST__

#include "gtest/gtest.h"

int main(int argc, char* argv[])
{
    std::string output_file = "./gtest";
    if (argc >= 2)
        output_file = argv[1];
    output_file += "/test_oamrouter.xml";

    testing::GTEST_FLAG(output) = output_file;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#endif
