/**@file
* 版权所有(C)2016, 深圳市中兴通讯股份有限公司<br>
* 文件名称：OAMRouterService.cpp<br>
* 内容摘要：
* 其它说明：无
* @version 1.0
* @author
* @since  2016-04-20
*/
#include <stdlib.h>
#include <thread>
#include <memory>
#include <set>

#include "Poco/SharedPtr.h"
#include "Poco/NumberParser.h"
#include "Poco/StringTokenizer.h"

#include "OAMRouterService.h"
#include "Configuration.h"

#include "TCFSAdapter.h"
#include "Simu_TCFSAdapter.h"
#include "RESTfulServer.h"

using namespace std;
using namespace Poco;

NS_VCN_OAM_BEGIN(ROUTER)

//模拟模式开关
static bool simu_mode = false;

TCFSAdapterBasePtr create_adapter()
{
    TCFSAdapterBase *p = nullptr;
    if (simu_mode)
        p = new Simu_TCFSAdapter();
    else
        p = new TCFSAdapter();

    return static_cast<TCFSAdapterBasePtr>(p);
}

void OAMRouterService::init()
{
    Log::start("./conf/log4cpp_router.conf");
	CREATE_LOG();
	
    if (::getenv("simu")){
        simu_mode = true;
        logError("running oamrouter with simulator mode");
    }

    this->load_prop();
}

void OAMRouterService::load_prop()
{
    std::set<std::string> env_access_set;
    char *env_access = ::getenv("router_access");
    logDebug("env access:%s", env_access);
    if (env_access){
        StringTokenizer token(env_access, ",", StringTokenizer::TOK_IGNORE_EMPTY|StringTokenizer::TOK_TRIM);
        for(std::string type : token){
            env_access_set.insert(Poco::trimInPlace(type));
        }
    }

    auto access_list = Configuration::load_access_list();
    StringTokenizer token(access_list, ",", StringTokenizer::TOK_IGNORE_EMPTY|StringTokenizer::TOK_TRIM);
    for(std::string type : token){
        Poco::trimInPlace(type);
        if (!env_access_set.size() || env_access_set.count(type) > 0){
            try{
                create_service(type);
            }
            catch(Poco::Exception &e){
                logDebug("failed to create service(%s), %s", type.c_str(), e.displayText().c_str());
            }
            catch(...){
                logDebug("failed to create service(%s), unknown reason", type.c_str());
            }
        }
    }
}

void OAMRouterService::create_service(const std::string &name)
{
    std::string type_ = Configuration::load_access_type(name);
    Poco::toLowerInPlace(type_);
    if (type_.find("telnet/") == 0){
        TelnetProperty telnet_prop;
        Configuration::load(name, telnet_prop);

        TCFSAdapterBasePtr worker = create_adapter();
        auto &base = *worker;
        base["src_session"] = telnet_prop.Session_Type;
        base["pub_session"] = telnet_prop.Peer_Session_Type;
        base["pub_instance"]= telnet_prop.Peer_Name;
        worker->start();

        TelnetTermServer *p = new TelnetTermServer(worker, telnet_prop);
        p->start();
        m_telnets.push_back(static_cast<TelnetTermServerPtr>(p));
    }
    else if (type_.find("restful/") == 0){
        RESTfulProperty restful_prop;
        Configuration::load(name, restful_prop);

        TCFSAdapterBasePtr worker = create_adapter();
        auto &base = *worker;
        base["src_session"] = restful_prop.Session_Type;
        base["pub_session"] = restful_prop.Peer_Session_Type;
        base["pub_instance"]= restful_prop.Peer_Name;
        worker->start();

        RESTfulServer *p = new RESTfulServer(worker, restful_prop);
        p->start();
        m_RESTfuls.push_back(static_cast<RESTfulServerPtr>(p));
    }
    else{
        logError("undefined access type \"%s\"", type_.c_str());
    }
}

NS_VCN_OAM_END
