#include "Poco/Net/NetException.h"
#include "RESTfulRequestFactory.h"

#include "Poco/Format.h"

NS_VCN_OAM_BEGIN(ROUTER)

RESTfulRequestFactory::~RESTfulRequestFactory()
{
    Poco::RWLock::ScopedLock lock(m_mtx, true);
    m_handler_map.clear();
}

HTTPRequestHandler* RESTfulRequestFactory::createRequestHandler(const HTTPServerRequest& request)
{
    if (m_term.prop.Max_Client && m_term.prop.Max_Client == m_handler_map.size()){
        logError("failed to create request handler, exceeded maximum number(%hu) of online", m_term.prop.Max_Client);
        throw Poco::Net::HTTPException("exceeds capacity", HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR);
    }

    //丢弃获取web服务图标的请求
    std::string uri = request.getURI();
    if (uri == "/favicon.ico")
        return nullptr;

    SessionPtr ctx;
    {
        //获取一个空闲的顺序号
        Poco::RWLock::ScopedLock lock(m_mtx);
        for(auto i = 0; i < Poco::UInt16(-1); ++i){
            ctx = m_term.new_session();
            if (m_handler_map.count(ctx->sequence()) == 0)
                break;
            ctx.reset();
        }
    }
    if (!ctx){
        logError("failed to allocate session, no idle session resources");
        throw Poco::Net::HTTPException("allocate session exceeds capacity", HTTPServerResponse::HTTP_INTERNAL_SERVER_ERROR);
    }

    RESTfulRequestHandler *handler = new RESTfulRequestHandler(m_term, ctx);
    if (!handler)
        return handler;
    handler->callback_dispatch_function([this](const MessageInfo &message) -> int{
                                            return this->dispatch(message);
                                        });
    handler->callback_destroy_function([this](Poco::UInt32 sequence) -> void{
                                            this->remove_handler(sequence);
                                       });

    Poco::RWLock::ScopedLock lock(m_mtx, true);
    m_handler_map[ctx->sequence()] = handler;
    return handler;
}

void RESTfulRequestFactory::remove_handler(Poco::UInt32 sequence)
{
    Poco::RWLock::ScopedLock lock(m_mtx, true);
    m_handler_map.erase(sequence);
}

int RESTfulRequestFactory::dispatch(const MessageInfo &message)
{
    return m_worker->dispatch(message);
}

void RESTfulRequestFactory::response(const MessageInfo &message)
{
    try{
        Poco::UInt32 sequence = message.sequence();

        Poco::RWLock::ScopedLock lock(m_mtx);
        auto it = m_handler_map.find(sequence);
        if (it == m_handler_map.end()){
            std::string err = Poco::format("can't find the match sequence(%u) in response, type:%u", sequence, message.type());
            throw Poco::Exception(err);
        }

        auto handler = it->second;
        handler->response(message);
    }
    catch(Poco::Exception &e){
        logError("failed to response, %s", e.displayText().c_str());
    }
    catch(...){
        logError("failed to response, unknown reason");
    }
}

NS_VCN_OAM_END
