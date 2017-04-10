#ifndef RESTFULREQUESTFACTORY_H
#define RESTFULREQUESTFACTORY_H

#include <unordered_map>
#include <memory>
#include <iostream>
#include "string.h"

#include "Poco/RWLock.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/String.h"

#include "TerminalInfo.h"
#include "RESTfulRequestHandler.h"

#include "TCFSAdapter.h"
#include "vcn_events_router.h"

using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPRequestHandler;
using namespace vcn::omm;

NS_VCN_OAM_BEGIN(ROUTER)

class RESTfulRequestFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
    template<typename C>
    RESTfulRequestFactory(const RESTful_TerminalInfo &term, C &c) : m_term(term), m_worker(c)
    {
        m_worker->callback_response_function(*this, &RESTfulRequestFactory::response);

        CREATE_LOG();
    }

    ~RESTfulRequestFactory();

public:
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request);

    int dispatch(const MessageInfo &message);
    void response(const MessageInfo &message);

    void remove_handler(Poco::UInt32 sequence);

private:
    RESTful_TerminalInfo    m_term;
    TCFSAdapterBasePtr      m_worker;

    Poco::RWLock m_mtx;
    std::unordered_map<Poco::UInt32, RESTfulRequestHandler*> m_handler_map;

    DECLARE_LOG();
};

NS_VCN_OAM_END
#endif // RESTFULREQUESTFACTORY_H
