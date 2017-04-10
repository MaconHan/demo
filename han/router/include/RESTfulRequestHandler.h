#ifndef RESTFULREQUESTHANDLER_H
#define RESTFULREQUESTHANDLER_H

#include <memory>
#include <functional>

#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Event.h"

#include "vcn_defs.h"
#include "TerminalInfo.h"
#include "TerminalMessage.h"

using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

NS_VCN_OAM_BEGIN(ROUTER)

class RESTfulRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    RESTfulRequestHandler(const RESTful_TerminalInfo &term, const SessionPtr &ctx) : m_term(term), m_ctx(ctx)
    {
    }

    virtual ~RESTfulRequestHandler();

public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response);

    void response(const MessageInfo &message);

    template<typename F>
    void callback_dispatch_function(F f)
    {
        m_dispatch_func = f;
    }

    template<typename F>
    void callback_destroy_function(F f)
    {
        m_destroy_func = f;
    }

private:
    const RESTful_TerminalInfo &m_term;
    const SessionPtr            m_ctx;

    Poco::Event                         m_notify;
    std::shared_ptr<RESTfulResponse>    m_response;

    std::function<int (const MessageInfo&)> m_dispatch_func;
    std::function<void (Poco::UInt32)>      m_destroy_func;
};

NS_VCN_OAM_END
#endif // RESTFULREQUESTHANDLER_H
