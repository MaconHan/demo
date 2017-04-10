#include "HttpClientMethod.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/URI.h"
#include "Poco/StreamCopier.h"

using namespace zteomm::pub;
#define _safe_delete(p) if(p){delete p; p=NULL;} 

HttpClientMethod::HttpClientMethod()
{
	CREATE_LOG();
}

bool  HttpClientMethod::isHttps(const string &strUri)
{
	string::size_type pos = strUri.find("https");
	return (pos == 0)?(true ):(false);
}

Poco::Net::HTTPClientSession* HttpClientMethod::getSession(const string &strUri)
{
	try{
		Poco::Net::HTTPClientSession* session=NULL;
		URI uriObj(strUri);		
		if(isHttps(strUri))		{
			session = new Poco::Net::HTTPSClientSession(uriObj.getHost(), uriObj.getPort());
		}
		else{
			session = new Poco::Net::HTTPClientSession(uriObj.getHost(), uriObj.getPort());
		} 
		if(NULL == session) return NULL;
		Timespan timeOut(5,0); //³¬Ê±Ê±³¤Îª5Ãë
		session->setTimeout(timeOut);
		return session;
	}
	catch (Poco::Exception& exc)
	{
		logError("getSession uri [%s] . exception message:[%s]",strUri.c_str(),exc.displayText().c_str());
	}
	catch(...){
	   logError("getSession uri [%s] . unkown exception message",strUri.c_str());
	}
	return NULL;
}

Poco::UInt16 HttpClientMethod::HttpRequest(const string &strMethod,const string &strUri,const string &strbody,string &strRbody, map<string,string> &mapExtraHeader,const string &content_type)
{
    logDebug("request message is [%s], Method [%s] struri [%s].",strbody.c_str(),strMethod.c_str(),strUri.c_str());
    Poco::Net::HTTPClientSession  *session = getSession(strUri);
    if(NULL == session)
    {
		logError("getsession failed uri [%s], getsession failed.", strUri.c_str());
		return PORTAL_SYS_ERROR;
    }
    try
    {		
		URI uriObj(strUri);		
        HTTPRequest req(strMethod, uriObj.getPathEtc(), HTTPMessage::HTTP_1_1);
        req.setContentType(content_type);
        req.setContentLength( (int)strbody.length() );
        for(map<string,string>::iterator it = mapExtraHeader.begin(); it!=mapExtraHeader.end(); ++it)
        {
            logDebug("extra information is %s : %s",it->first.c_str(),it->second.c_str());
            req.set(it->first,it->second);
        }
        std::ostream& send = session->sendRequest(req);//sendRequest(...)¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿
        if(strbody.length()>0)  send << strbody <<std::flush;
        //HTTP¿¿¿¿
         HTTPResponse response;
         std::istream& rs =session->receiveResponse(response); //session.receiveResponse(res) >> strRbody;
         stringstream responseStream;
         Poco::StreamCopier::copyStream(rs, responseStream);
         strRbody = responseStream.str();

        logDebug("Method [%s], status is [%d], reason is [%s] Rbody is [%s] response content length is [%d].",
			strMethod.c_str(), response.getStatus(), response.getReason().c_str(), strRbody.c_str(), response.getContentLength());
        _safe_delete(session);
        return  response.getStatus();
    }
    catch (Poco::Exception& exc)
    {
        logError("Method [%s],  uri [%s] request body [%s] extra  size [%d]. exception message:[%s]",
			strMethod.c_str(),strUri.c_str(),strbody.c_str(),mapExtraHeader.size(),exc.displayText().c_str());
    }
    catch(...)
    {
        logError("Method [%s],  uri [%s] request body [%s] extra  size [%d]. unkown exception message",
			strMethod.c_str(),strUri.c_str(),strbody.c_str(),mapExtraHeader.size());
    }
    _safe_delete(session);
    return PORTAL_SYS_ERROR;
}

Poco::UInt16 HttpClientMethod::doPost(const string &strUri, const std::string &strbody, std::string &strRbody,const string &content_type)
{
    logDebug("POST request message is [%s] struri [%s].",strbody.c_str(),strUri.c_str());
    map<string,string> mapExtraHeaderNull;
    return HttpRequest(HTTPRequest::HTTP_POST,strUri,strbody,strRbody,mapExtraHeaderNull,content_type);
}

Poco::UInt16 HttpClientMethod::doPut(const string &strUri, const std::string &strbody, std::string &strRbody, map<string,string> &mapExtraHeader)
{
	logDebug("Put request message is [%s] struri [%s].",strbody.c_str(),strUri.c_str());	
	return HttpRequest(HTTPRequest::HTTP_PUT,strUri,strbody,strRbody,mapExtraHeader);
}

Poco::UInt16 HttpClientMethod::doPut(const string &strUri,string &strRbody)
{
	logDebug("PUT, no body message. URI is [%s].",strUri.c_str());
	map<string,string> mapExtraHeaderNull;
	return HttpRequest(HTTPRequest::HTTP_PUT,strUri,"",strRbody,mapExtraHeaderNull);
}

Poco::UInt16 HttpClientMethod::doDelete(const string &strUri)
{
	logDebug("delete,  URI is [%s].",strUri.c_str());
	string strRbody;
	map<string,string> mapExtraHeaderNull;
	return HttpRequest(HTTPRequest::HTTP_DELETE,strUri,"",strRbody,mapExtraHeaderNull);
}

Poco::UInt16 HttpClientMethod::doDelete(const string &strUri,const std::string &strbody, std::string &strRbody)
{
    logDebug("delete,  URI is [%s].",strUri.c_str());
    map<string,string> mapExtraHeaderNull;
    return HttpRequest(HTTPRequest::HTTP_DELETE,strUri,strbody,strRbody,mapExtraHeaderNull);
}

Poco::UInt16 HttpClientMethod::doGet(const string &strUri,string &strRbody)
{
	logDebug("Get, no body message. URI is [%s].",strUri.c_str());
	map<string,string> mapExtraHeaderNull;
	return HttpRequest(HTTPRequest::HTTP_GET,strUri,"",strRbody,mapExtraHeaderNull);
}

Poco::UInt16 HttpClientMethod::doGet(const string &strUri,string &strRbody, map<string,string> &mapExtraHeader)
{
	logDebug("Get, no body message,has extra header message. URI is [%s].",strUri.c_str());
	return HttpRequest(HTTPRequest::HTTP_GET,strUri,"",strRbody,mapExtraHeader);
}
