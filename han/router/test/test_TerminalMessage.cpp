#include "gtest/gtest.h"
#include "json/json.h"

#include "TerminalMessage.h"

using namespace Poco;

NS_VCN_OAM_BEGIN(ROUTER)

struct Test_Router_TerminalMessage : public testing::Test{
    Test_Router_TerminalMessage()
    {
    }

    void SetUp() override{
    }

    void TearDown() override{
    }
};

TEST_F(Test_Router_TerminalMessage, Invalid_Json)
{
    std::string json_str = "";
    try{
        auto msg = MessageInfo::json2message(EV_ROUTER_TELNET_HEARTBEAT_REQ, json_str);
        EXPECT_FALSE(1);
    }
    catch(std::exception &e){
        EXPECT_TRUE(1);
        return;
    }

    EXPECT_FALSE(1);
}

TEST_F(Test_Router_TerminalMessage, Invalid_Event)
{
    std::string json_str = static_cast<std::string>(HeartbeatRequest("abc123"));
    Poco::UInt32 invalid_event = 0;
    try{
        auto msg = MessageInfo::json2message(invalid_event, json_str);
        EXPECT_FALSE(1);
    }
    catch(std::exception &e){
        EXPECT_TRUE(1);
        return;
    }

    EXPECT_FALSE(1);
}

TEST_F(Test_Router_TerminalMessage, RESTful)
{
    RESTfulRequest request(1);
    request.client          = "a";
    request.url             = "b";
    request.method          = "c";
    request.username        = "d";
    request.auth_code       = "e";
    request.language_option = "f";
    request.timeout         = 'g';
    request.type            = "h";
    request.body            = "i";

    std::string json_str = static_cast<std::string>(request);
    MessageInfo *msg1 = MessageInfo::json2message(static_cast<MessageInfo&>(request).type(), json_str);
    RESTfulRequest *msg2 = dynamic_cast<RESTfulRequest*>(msg1);
    EXPECT_TRUE(NULL != msg2);
    EXPECT_TRUE(static_cast<std::string>(*msg2) == json_str);
    delete msg1;


    RESTfulResponse response(1, {0, ""}, "abc123");
    json_str = static_cast<std::string>(response);
    MessageInfo *msg3 = MessageInfo::json2message(response.type(), json_str);
    RESTfulResponse *msg4 = dynamic_cast<RESTfulResponse*>(msg3);
    EXPECT_TRUE(NULL != msg4);
    EXPECT_TRUE(static_cast<std::string>(*msg4) == json_str);
    delete msg3;
}

NS_VCN_OAM_END
