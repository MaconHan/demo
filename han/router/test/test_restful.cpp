#include "test_restful.h"

NS_VCN_OAM_BEGIN(ROUTER)

void Test_Router_RESTful::start_server(Response_Type type)
{
    std::shared_ptr<RESTful_Adapter> worker = std::shared_ptr<RESTful_Adapter>(new RESTful_Adapter(type));
    server = std::shared_ptr<RESTfulServer>(new RESTfulServer(worker, restful_prop));
    server->start();
}

void Test_Router_RESTful::start_client()
{
    client_session = std::shared_ptr<HTTPClientSession>(new HTTPClientSession("localhost", 7778));
    client_session->setKeepAliveTimeout(Poco::Timespan(300, 0));
}

void Test_Router_RESTful::send_request()
{
    client_session->sendRequest(*request) << body;
}

void Test_Router_RESTful::set_accept_type(const std::string &accept_type)
{
    request->set("Accept", accept_type);
}

void Test_Router_RESTful::generate_request()
{
    body.assign("Hello");
    request = std::shared_ptr<HTTPRequest>(new HTTPRequest("GET", "/fm/alarm?token=1234567890"));
    request->setContentLength((int)body.length());
    request->set("Username", "admin");
    request->set("Z-AUTH-CODE", "12345");
    request->set("language-option", "zh_CN");
    request->set("Timeout", "5");
}

void Test_Router_RESTful::response_result_should_be(const HTTPResponse::HTTPStatus expect_status, const std::string &expect_response_body, const std::string &accept_type )
{
    HTTPResponse response;
    try {
        std::istream &stream = client_session->receiveResponse(response);
        std::ostringstream ostr;
        Poco::StreamCopier::copyStream(stream, ostr);
        std::string response_body = ostr.str();

        ASSERT_TRUE(response.getStatus() == expect_status);
        ASSERT_TRUE(response_body == expect_response_body);
        ASSERT_TRUE(response.getContentType() == accept_type);

        }
    catch(Poco::Exception &e) {
        std::cout << "response error:" << e.displayText() << std::endl;
    }
}

/**
* @brief ��Ӧ����
*/
TEST_F(Test_Router_RESTful, response_normal)
{
    start_server(RESPONSE_NORMAL);
    start_client();
    generate_request();
    set_accept_type("text/plain");
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_OK, "Success!", "text/plain");

    ASSERT_TRUE( request_info["url"].asString() == "/fm/alarm?token=1234567890" );
    ASSERT_TRUE( request_info["method"].asString() == "GET" );
    ASSERT_TRUE( request_info["username"].asString() == "admin" );
    ASSERT_TRUE( request_info["auth_code"].asString() == "12345" );
    ASSERT_TRUE( request_info["language_option"].asString() == "zh_CN" );
    ASSERT_TRUE( request_info["timeout"].asUInt() == 5 );
    ASSERT_TRUE( request_info["type"].asString() ==  restful_prop.Type );
    ASSERT_TRUE( request_info["body"].asString() == "Hello" );
}

/**
* @brief �������ͣ�text/html
*/
TEST_F(Test_Router_RESTful, accept_type_html)
{
    start_server(RESPONSE_NORMAL);
    start_client();
    generate_request();
    set_accept_type("text/html");
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_OK, "Success!", "text/html");
}

/**
* @brief Ĭ���������ͣ�text/plain
*/
TEST_F(Test_Router_RESTful, accept_type_default)
{
    start_server(RESPONSE_NORMAL);
    start_client();
    generate_request();
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_OK, "Success!", "text/plain");
}

/**
* @brief ���񲻴���
*/
TEST_F(Test_Router_RESTful, http_service_unavailable)
{
    start_server(HTTP_SERVICE_UNAVAILABLE);
    start_client();
    generate_request();
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_SERVICE_UNAVAILABLE, "error code:-1\r\n", "text/plain");
}

/**
* @brief ��Ӧ��ʱ
*/
TEST_F(Test_Router_RESTful, request_timeout)
{
    start_server(REQUEST_TIMEOUT);
    start_client();
    generate_request();
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_REQUEST_TIMEOUT, "Internal Server Timeout\r\n", "text/plain");
}

/**
* @brief �Ƿ�����
*/
TEST_F(Test_Router_RESTful, bad_request)
{
    start_server(BAD_REQUEST);
    start_client();
    generate_request();
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_BAD_REQUEST, "Bad request", "text/plain");
}

/**
* @brief �ڲ�����-��Ӧ��Ϣ����
*/
TEST_F(Test_Router_RESTful, incorrect_message)
{
    start_server(INCORRECT_MESSAGE);
    start_client();
    generate_request();
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR, "incorrect message", "text/plain");
}

/**
* @brief �ڲ������������쳣��Poco���쳣��
*/
TEST_F(Test_Router_RESTful, poco_exception)
{
    start_server(POCO_EXCEPTION);
    start_client();
    generate_request();
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR, "Exception: poco exception\r\n", "text/plain");
}

/**
* @brief �ڲ������������쳣��std���쳣��
*/
TEST_F(Test_Router_RESTful, std_exception)
{
    start_server(STD_EXCEPTION);
    start_client();
    generate_request();
    send_request();

    response_result_should_be(HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR, "std exception\r\n", "text/plain");
}
NS_VCN_OAM_END
