#ifndef TEST_RESTFUL_H_INCLUDED
#define TEST_RESTFUL_H_INCLUDED

#include <thread>
#include <vector>
#include <memory>

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/StreamCopier.h"
#include "Poco/Timespan.h"
#include "Poco/ThreadPool.h"

#include "gtest/gtest.h"
#include "json/json.h"

#include "RESTfulServer.h"

using namespace Poco;
using namespace Poco::Net;

NS_VCN_OAM_BEGIN(ROUTER)

// ���Գ���
enum Response_Type
{
    RESPONSE_NORMAL = 0,
    INTERNAL_SERVICE_UNAVAIBLE,
    HTTP_SERVICE_UNAVAILABLE,
    BAD_REQUEST,
    REQUEST_TIMEOUT,
    NO_RESPONSE,
    INCORRECT_MESSAGE,
    POCO_EXCEPTION,
    STD_EXCEPTION
};

Json::Value request_info;

struct RESTful_Adapter : public TCFSAdapterBase
{
  public:
    RESTful_Adapter(Response_Type type = RESPONSE_NORMAL): m_type(type)
    {
        if ( m_type != HTTP_SERVICE_UNAVAILABLE )
        {
            this->callback_dispatch_function(*this, &RESTful_Adapter::execute);
        }
    }

    ~RESTful_Adapter()
    {
    }

  private:

    /**
    * @brief ��Ӧrestful����
    */
    void execute(const MessageInfo &message)
    {
        if(m_type == POCO_EXCEPTION)
        {
            throw Poco::Exception("poco exception",0);
        }

        else if (m_type == STD_EXCEPTION)
        {
            struct some_exception: public std::exception
            {
                const char* what()const noexcept
                {
                    return "std exception";
                }
            };

            some_exception e;
            throw e;
        }

        else
        {
            auto RESTful_handle = [this](const MessageInfo &message) noexcept
            {
                RESTfulRequest req = dynamic_cast<const RESTfulRequest&>(message);

                // �������˽��յ���������Ϣ
                req >> request_info;

                Poco::UInt32 timeout = req.timeout;
                Poco::UInt32 duration(0);

                if ( m_type == REQUEST_TIMEOUT )
                {
                    // ��ʱ2s�����Ӧ
                    duration = timeout + 2;
                    std::this_thread::sleep_for(std::chrono::seconds(duration));
                    return;
                }
                else
                {
                    duration = timeout - 2;
                    std::this_thread::sleep_for(std::chrono::seconds(duration));
                }

                Poco::UInt32 sequence = req.sequence();
                ErrorInfo response_error = {HTTPServerResponse::HTTP_OK, ""};
                std::string response_body("Success!");

                if ( m_type == BAD_REQUEST )
                {
                    response_error.code = HTTPServerResponse::HTTP_BAD_REQUEST;
                    response_error.descript = "";
                    response_body = "Bad request";
                }

                std::cout << "response: sequence = "         << sequence
                          << " , error_code = "              << response_error.code
                          << " , error_discription = "       << response_error.descript.c_str()
                          << " , response body = "           << response_body.c_str() << std::endl;

                if ( m_type == INCORRECT_MESSAGE )
                {
                    this->response(CommandResponse(sequence, response_error, response_body));
                }
                else
                {
                    this->response(RESTfulResponse(sequence, response_error, response_body));
                }

            };

            std::thread thr(RESTful_handle, std::ref(message));
            thr.detach();
        }
    }

    private:
    Response_Type m_type;
};

class Test_Router_RESTful : public testing::Test
{
    public:
    /**
    * @brief ����restful����
    */
    void start_server(Response_Type type);

    /**
    * @brief �����ͻ���
    */
    void start_client();

    /**
    * @brief ����restful����
    */
    void send_request();

    /**
    * @brief ����restful����
    */
    void generate_request();

    /**
    * @brief ������������
    */
    void set_accept_type(const std::string &accept_type);

    /**
    * @brief restful����Ԥ����Ӧ���
    */
    void response_result_should_be(const HTTPServerResponse::HTTPStatus expect_status, const std::string &expect_response_body, const std::string &accept_type);

  protected:
    void SetUp()override
    {
        // ����restful��������
        auto access_type = "restful_normal";
        Configuration::load(access_type, restful_prop);
    }

    void TearDown()override
    {
    }

  public:
    RESTfulProperty restful_prop;

  private:
    std::shared_ptr<RESTfulServer> server;
    std::shared_ptr<HTTPClientSession> client_session;
    std::shared_ptr<HTTPRequest> request;
    std::string body;
};

NS_VCN_OAM_END
#endif // TEST_RESTFUL_H_INCLUDED
