#include <memory>
#include <iostream>
#include <sstream>

#include "Poco/String.h"

#include "Configuration.h"

NS_VCN_OAM_BEGIN(ROUTER)

//配置文件
const static std::string Configuration_File = "./conf/router/deploy-default.properties";

const static std::string Property_Prefix = "vcn.oam.router.";

Poco::SingletonHolder<Configuration> Configuration::Singleton_Conf;

std::string TelnetProperty::toString() const
{
    std::stringstream stream;
    stream << "type:                " << Type << std::endl
           << "IP:                  " << IP << std::endl
           << "port:                " << Port << std::endl
           << "session:             " << Session_Type << std::endl
           << "peer.session:        " << Peer_Session_Type << std::endl
           << "peer.name:           " << Peer_Name << std::endl
           << "client.max:          " << Max_Client << std::endl
           << "timeout.login:       " << Login_Timeout << std::endl
           << "timeout.login.idle:  " << LoginIdle_Timeout << std::endl
           << "timeout.input.idle:  " << InputIdle_Timeout;

    return stream.str();
}

std::string RESTfulProperty::toString() const
{
    std::stringstream stream;
    stream << "type:                " << Type << std::endl
           << "IP:                  " << IP << std::endl
           << "port:                " << Port << std::endl
           << "session:             " << Session_Type << std::endl
           << "peer.session:        " << Peer_Session_Type << std::endl
           << "peer.name:           " << Peer_Name << std::endl
           << "client.max:          " << Max_Client << std::endl
           << "timeout:             " << Timeout;

    return stream.str();
}

Configuration::Configuration()
{
    try{
        Poco::Util::PropertyFileConfiguration::load(Configuration_File);
    }
    catch(Poco::Exception &e){
        std::cout << "failed to load properties '"<< Configuration_File <<"', " << e.displayText() << std::endl;
    }
    catch(...){
        std::cout << "failed to load properties '"<< Configuration_File << "'" << std::endl;
    }
}

std::string Configuration::load_access_list()
{
    PropertyFileConfiguration &prop = *(Singleton_Conf.get());
    return prop.getString("vcn.oam.router.access.list", "");
}

std::string Configuration::load_access_type(const std::string &key)
{
    PropertyFileConfiguration &prop = *(Singleton_Conf.get());

    std::string prefix = Property_Prefix + Poco::toLower(key);
    return prop.getString(prefix + ".type", "");
}

bool Configuration::load(const std::string &key, TelnetProperty &telnet)
{
    PropertyFileConfiguration &prop = *(Singleton_Conf.get());

    //默认的关于session定义
    auto default_session       = prop.getString("vcn.oam.router.session",  "30003");
    auto default_peer_session  = prop.getString("vcn.oam.router.peer.session", "30001");
    auto default_peer_name     = prop.getString("vcn.oam.router.peer.name", "COMM_1::OAM_DISP_FSM");

    std::string prefix = Property_Prefix + Poco::toLower(key);
    telnet.Type     = prop.getString(prefix + ".type", "");

    telnet.IP       = prop.getString(prefix + ".ip", "0.0.0.0");
    telnet.Port     = prop.getInt(prefix + ".port", 0);

    telnet.Session_Type     = prop.getString(prefix + ".session", default_session);
    telnet.Peer_Session_Type= prop.getString(prefix + ".peer.session", default_peer_session);
    telnet.Peer_Name        = prop.getString(prefix + ".peer.name", default_peer_name);

    telnet.Max_Client       = prop.getInt(prefix + ".client.max", 8);

    telnet.Login_Timeout        = prop.getInt(prefix + ".timeout.login", 15);
    telnet.LoginIdle_Timeout    = prop.getInt(prefix + ".timeout.login.idle", 60);
    telnet.InputIdle_Timeout    = prop.getInt(prefix + ".timeout.input.idle", 15 * 60);

    telnet.Heartbeat            = prop.getInt(prefix + ".heartbeat", 0);
    return true;
}

bool Configuration::load(const std::string &key, RESTfulProperty &restful)
{
    PropertyFileConfiguration &prop = *(Singleton_Conf.get());

    //默认的关于session定义
    auto default_session       = prop.getString("vcn.oam.router.session",  "30003");
    auto default_peer_session  = prop.getString("vcn.oam.router.peer.session", "30001");
    auto default_peer_name     = prop.getString("vcn.oam.router.peer.name", "COMM_1::OAM_DISP_FSM");

    std::string prefix  = Property_Prefix + key;
    restful.Type        = prop.getString(prefix + ".type", "");
    restful.IP          = prop.getString(prefix + ".ip", "0.0.0.0");
    restful.Port        = prop.getInt(prefix + ".port", 0);

    restful.Session_Type     = prop.getString(prefix + ".session", default_session);
    restful.Peer_Session_Type= prop.getString(prefix + ".peer.session", default_peer_session);
    restful.Peer_Name        = prop.getString(prefix + ".peer.name", default_peer_name);

    restful.Max_Client  = prop.getInt(prefix + ".client.max", 32);
    restful.Timeout     = prop.getInt(prefix + ".timeout", 30);
    return true;
}

NS_VCN_OAM_END
