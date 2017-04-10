/**@file
* ��Ȩ����(C)2001, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�.h<br>
* ����ժҪ��
* ����˵������
* @version 1.0
* @author
* @since  2009-06-04
*/

#ifndef TERMINAL_INFO_H
#define TERMINAL_INFO_H

/* ======================== ���� ============================ */
#include <string>
#include <memory>

#include "Poco/Types.h"
#include "Configuration.h"

using namespace std;
/* ======================== �����ռ� ========================= */

NS_VCN_OAM_BEGIN(ROUTER)

enum ServiceType
{
    SOCKET_TYPE_TELNET  = (Poco::UInt16)250,
    SOCKET_TYPE_RESTFUL = (Poco::UInt16)251,
    SOCKET_TYPE_NO_TYPE = (Poco::UInt16)255  //< 255δ��������
};

#define INVALID_SEQUENCE    0x00000000

class TerminalInfo;

class Session
{
private:
    Session(const TerminalInfo &term);

public:
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    const TerminalInfo& terminal() const;

    Poco::UInt32 sequence() const;

private:
    const TerminalInfo &m_term;
    Poco::UInt16        m_sequence;

    friend TerminalInfo;
};
using SessionPtr = std::shared_ptr<Session>;

class TerminalInfo
{
public:
    TerminalInfo()
    {}

    TerminalInfo(ServiceType type, const std::string &name);

    std::string toString() const;

    Poco::UInt32 key() const;

    SessionPtr new_session();

public:
    static Poco::UInt32 convert2terminal(Poco::UInt32 sequence)
    {
        return (sequence >> 16);
    }

public:
    ServiceType     type;
    std::string     name;
    std::string     local_addr;
    std::string     peer_addr;
    Poco::UInt16    socket_fd;

private:
    Poco::UInt16    m_seq_index;
};

struct Telnet_TerminalInfo : public TerminalInfo
{
    Telnet_TerminalInfo(const TelnetProperty &telnet_prop) : prop(telnet_prop)
    {
    }

    const TelnetProperty &prop;
};

struct RESTful_TerminalInfo : public TerminalInfo
{
    RESTful_TerminalInfo(const RESTfulProperty &restful_prop) : prop(restful_prop)
    {
    }

    const RESTfulProperty &prop;
};

NS_VCN_OAM_END

#endif
