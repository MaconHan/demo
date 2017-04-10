#ifndef __SIMU_TCFSADAPTER_H__
#define __SIMU_TCFSADAPTER_H__

#include "TCFSAdapter.h"
#include "TerminalMessage.h"

NS_VCN_OAM_BEGIN(ROUTER)

class Simu_TCFSAdapter : public TCFSAdapterBase
{
public:
    Simu_TCFSAdapter();
    ~Simu_TCFSAdapter();

public:
    void execute(const MessageInfo &message);
};

NS_VCN_OAM_END
#endif
