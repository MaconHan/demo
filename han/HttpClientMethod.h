#pragma once

#include <string>
#include <iostream>

//#include "json/json.h"
#include "Poco/String.h"
#include "Poco/Format.h"
#include "Poco/Types.h"
#include "Poco/Exception.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "log.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

namespace zteomm{
namespace pub{

#define PORTAL_SYS_ERROR   0xFFFF


class HttpClientMethod
{
public:
	HttpClientMethod();
	
	Poco::UInt16 doPost(const string &strUri,const string &strbody,string &strRbody, const string &content_type="application/json");

	Poco::UInt16 doPut(const string &strUri,const string &strbody,string &strRbody, map<string,string> &mapExtraHeader);

	Poco::UInt16 doPut(const string &strUri,string &strRbody);

	Poco::UInt16 doDelete(const string &strUri);
        
        Poco::UInt16 doDelete(const string &strUri,const std::string &strbody, std::string &strRbody);

	Poco::UInt16 doGet(const string &strUri,string &strRbody);

	Poco::UInt16 doGet(const string &strUri,string &strRbody, map<string,string> &mapExtraHeader);

private:
	DECLARE_LOG();
	bool isHttps(const string &strUri);
	Poco::Net::HTTPClientSession* getSession(const string &strUri);
	Poco::UInt16 HttpRequest(const string &strMethod,const string &strUri,const string &strbody,string &strRbody, map<string,string> &mapExtraHeader,const string &content_type="application/json");
};

	}
}

