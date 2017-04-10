/**@file
* 版权所有(C)2016, 深圳市中兴通讯股份有限公司<br>
* 文件名称：.h<br>
* 内容摘要：
* 其它说明：无
* @version 1.0
* @author
* @since  2016-04-20
*/

#ifndef OAMROUTERSERVICE_H_INCLUDED
#define OAMROUTERSERVICE_H_INCLUDED

#include <iostream>
#include <memory>
#include <vector>

#include "Poco/Types.h"

#include "log.h"
#include "vcn_defs.h"

#include "RESTfulServer.h"
#include "TelnetHandle.h"

using namespace std;
using namespace Poco;
using Poco::Net::HTTPServer;

NS_VCN_OAM_BEGIN(ROUTER)

class OAMRouterService
{
public:
    OAMRouterService()
    {
    }

    ~OAMRouterService()
    {
    }

    void init();

    /**
    * @brief 加载配置文件
    */
    void load_prop();

    /**
    * @brief 创建服务
    */
    void create_service(const std::string &name);

    inline std::size_t service_count() const
    {
        return m_RESTfuls.size() + m_telnets.size();
    }

private:
    using RESTfulServerPtr      = std::shared_ptr<RESTfulServer>;
    using TelnetTermServerPtr   = std::shared_ptr<TelnetTermServer>;
    using RESTfulServerPtrV     = std::vector<RESTfulServerPtr>;
    using TelnetTermServerPtrV  = std::vector<TelnetTermServerPtr>;

    RESTfulServerPtrV      m_RESTfuls;
    TelnetTermServerPtrV   m_telnets;

    DECLARE_LOG();
};

NS_VCN_OAM_END
#endif // OAMROUTERSERVICE_H_INCLUDED
