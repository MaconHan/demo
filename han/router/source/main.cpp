#ifndef __TEST__

#include <iostream>
#include <thread>
#include <chrono>

#include "Poco/SharedPtr.h"

#include "tcfs.h"
#include "OAMRouterService.h"

using namespace std;
using namespace Poco;

NS_VCN_OAM_USING(ROUTER);

int main(int argc, const char* argv[])
{
    tcfs_init_app();
    Poco::SharedPtr<OAMRouterService> router_service = Poco::SharedPtr<OAMRouterService>(new OAMRouterService());
    router_service->init();

    while(1) std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}


#endif // __TEST__
