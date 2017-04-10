#include "Poco/Timespan.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"

#include "vcn_defs.h"
#include "RESTfulServer.h"
#include "RESTfulRequestFactory.h"

using namespace Poco;
using namespace Poco::Net;

NS_VCN_OAM_BEGIN(ROUTER)

RESTfulServer::RESTfulServer(TCFSAdapterBasePtr work_adapter, const RESTfulProperty &prop):
    m_work_adapter(work_adapter),
    m_prop(prop)
{
    CREATE_LOG();

    logDebug("start a RESTfulServer:\n%s", m_prop.toString().c_str());
}

RESTfulServer::~RESTfulServer()
{
    stop();
}

/**
* @brief Æô¶¯RESTful·þÎñ
*/
void RESTfulServer::start()
{
    using namespace Poco;
    using namespace Poco::Net;
    std::string error;

    try{
        int max_connections = std::max<Poco::UInt16>(m_prop.Max_Client, 8);
        ServerSocket ss = ServerSocket(SocketAddress(m_prop.IP, m_prop.Port), max_connections);

        RESTful_TerminalInfo term(m_prop);
        term.type           = SOCKET_TYPE_RESTFUL;
        term.socket_fd      = ss.impl()->sockfd();

        HTTPServerParams *http_params = new HTTPServerParams();
        http_params->setKeepAlive(false);

        RESTfulRequestFactory *factory  = new RESTfulRequestFactory(term, m_work_adapter);
        ThreadPool *thread_pool         = new ThreadPool(4, max_connections);
        HTTPServer *http_server         = new HTTPServer(factory, *thread_pool, ss, http_params);

        m_http_pool     = std::shared_ptr<ThreadPool>(thread_pool);
        m_http_server   = std::shared_ptr<HTTPServer>(http_server);
    }
    catch(Poco::Exception &e){
        error = e.displayText();
    }
    catch(...){
    }

    if(m_http_server){
        m_http_server->start();
    }
    else{
        logDebug("failed to start a RESTfulServer(%s:%hu), %s",
                 m_prop.IP.c_str(),
                 m_prop.Port,
                 error.c_str());
    }
}

void RESTfulServer::stop()
{
    if (m_http_pool)
        m_http_pool->stopAll();

    if (m_http_server)
        m_http_server->stop();
}

NS_VCN_OAM_END
