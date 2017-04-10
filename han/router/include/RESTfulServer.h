#ifndef RESTFULSERVER_H_INCLUDED
#define RESTFULSERVER_H_INCLUDED
#include <memory>

#include "Poco/ThreadPool.h"
#include "Poco/Net/HTTPServer.h"

#include "syscomm.h"
#include "vcn_defs.h"
#include "TCFSAdapter.h"
#include "Configuration.h"

NS_VCN_OAM_BEGIN(ROUTER)


class RESTfulServer
{
public:
    RESTfulServer(TCFSAdapterBasePtr work_adapter, const RESTfulProperty &prop);

    ~RESTfulServer();

    /**
    * @brief Æô¶¯RESTful·þÎñ
    */
    void start();

    void stop();

protected:
    TCFSAdapterBasePtr                          m_work_adapter;

    std::shared_ptr<Poco::Net::HTTPServer>      m_http_server;
    std::shared_ptr<Poco::ThreadPool>           m_http_pool;

    RESTfulProperty                             m_prop;
private:
    DECLARE_LOG();
};

NS_VCN_OAM_END


#endif // RESTFULSERVER_H_INCLUDED
