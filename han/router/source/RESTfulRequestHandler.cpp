#include <iostream>
#include <exception>
#include <stdexcept>

#include "Poco/StreamCopier.h"
#include "Poco/NumberParser.h"

#include "RESTfulRequestHandler.h"

NS_VCN_OAM_BEGIN(ROUTER)

RESTfulRequestHandler::~RESTfulRequestHandler()
{
    m_notify.set();

    if (m_destroy_func)
        m_destroy_func(m_ctx->sequence());
}

void RESTfulRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response)
{
    using Poco::NumberParser;
    response.setContentType(request.get("Accept", "text/plain"));

    std::string timeout = request.get("Timeout", "");

    RESTfulRequest req(m_ctx->sequence());
    req.client          = request.clientAddress().toString();
    req.url             = request.getURI();
    req.method          = request.getMethod();
    req.username        = request.get("username", "");
    req.auth_code       = request.get("Z-AUTH-CODE", "");
    req.language_option = request.get("language-option", "");
    req.timeout         = NumberParser::tryParseUnsigned(timeout, req.timeout) ? req.timeout : m_term.prop.Timeout;
    req.type            = m_term.prop.Type;

    //针对DELETE操作, 对于未知内容长度的http请求, 暂时不获取body部分
    if (!(req.method == Poco::Net::HTTPRequest::HTTP_DELETE && request.getContentLength() == Poco::Net::HTTPMessage::UNKNOWN_CONTENT_LENGTH))
        Poco::StreamCopier::copyToString(request.stream(), req.body);

    int result = 0;
    try{
        result = m_dispatch_func(req);
    }
    catch(Poco::Exception &e){
        response.setStatus(HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR);
        response.send() << e.displayText() << "\r\n";
        return;
    }
    catch(std::exception &e){
        response.setStatus(HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR);
        response.send() << e.what() << "\r\n";
        return;
    }
    catch(...){
        response.setStatus(HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR);
        response.send() << "Unknown Error\r\n";
        return;
    }

    if (result != 0){
        response.setStatusAndReason(HTTPServerResponse::HTTP_SERVICE_UNAVAILABLE, "Internal Service Unavailable");
        response.send() << "error code:" << result << "\r\n";
        return;
    }
    if (!m_notify.tryWait(req.timeout * 1000)){
        response.setStatus(HTTPServerResponse::HTTP_REQUEST_TIMEOUT);
        response.send() << "Internal Server Timeout\r\n";
        return;
    }

    if (m_response){
        response.setStatus((HTTPServerResponse::HTTPStatus)(m_response->error_code()));
        response.send() << m_response->body();
    }
    else{
        response.setStatus(HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR);
        response.send() << "null pointer\r\n";
    }
}

void RESTfulRequestHandler::response(const MessageInfo &message)
{
    const RESTfulResponse *resp = dynamic_cast<const RESTfulResponse*>(&message);
    RESTfulResponse *p = nullptr;
    if (unlikely(resp == nullptr)){
        ErrorInfo err = {HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR, ""};
        p = new RESTfulResponse(message.sequence(), err, "incorrect message");
    }
    else{
        p = new RESTfulResponse(*resp);
    }

    m_response = std::shared_ptr<RESTfulResponse>(p);
    m_notify.set();
}

NS_VCN_OAM_END
