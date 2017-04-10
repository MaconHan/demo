#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include "vcn_defs.h"

#include "Poco/SingletonHolder.h"
#include "Poco/Util/PropertyFileConfiguration.h"
#include "Poco/Types.h"

NS_VCN_OAM_BEGIN(ROUTER)

struct TelnetProperty{
    std::string     Type;

    std::string     IP;
    Poco::UInt16    Port;

    std::string     Session_Type;
    std::string     Peer_Session_Type;
    std::string     Peer_Name;

    Poco::UInt16    Max_Client          = 8;

    Poco::UInt16    Login_Timeout       = 15;
    Poco::UInt16    LoginIdle_Timeout   = 60;
    Poco::UInt16    InputIdle_Timeout   = 5 * 60;

    Poco::UInt16    Heartbeat           = 0;

    std::string toString() const;
};

struct RESTfulProperty{
    std::string     Type;

    std::string     IP;
    Poco::UInt16    Port;

    std::string     Session_Type;
    std::string     Peer_Session_Type;
    std::string     Peer_Name;

    Poco::UInt16    Max_Client      = 32;
    Poco::UInt16    Timeout         = 30;

    std::string toString() const;
};


class Configuration : public Poco::Util::PropertyFileConfiguration{
public:
    static std::string load_access_list();
    static std::string load_access_type(const std::string &key);
    static bool load(const std::string &key, TelnetProperty &telnet);
    static bool load(const std::string &key, RESTfulProperty &restful);

private:
    Configuration();

private:
    static Poco::SingletonHolder<Configuration> Singleton_Conf;

    friend Poco::SingletonHolder<Configuration>;
};
NS_VCN_OAM_END

#endif
