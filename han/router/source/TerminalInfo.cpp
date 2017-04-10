/**@file
 * 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
 * 文件名称：.cpp<br>
 * 文件标识：<br>
 * 内容摘要：<br>
 * 其它说明：<br>
 * @version v1.0<br>
 * @author
 * @since 2009-06-04
 */
#include "Poco/Format.h"
#include "Poco/NumberFormatter.h"

#include "TerminalInfo.h"


NS_VCN_OAM_BEGIN(ROUTER)
TerminalInfo::TerminalInfo(ServiceType type, const std::string &name) : type(type), name(name), socket_fd(0), m_seq_index(0)
{
}

std::string TerminalInfo::toString() const
{
    return Poco::format("Type:%?u;Name:%s;fd:%?u;", Poco::UInt16(type), name, socket_fd);
}

Poco::UInt32 TerminalInfo::key() const
{
    return socket_fd;
}

SessionPtr TerminalInfo::new_session()
{
    Session *ctx = new Session(*this);
    ctx->m_sequence = ++m_seq_index;
    return SessionPtr(ctx);
}

Session::Session(const TerminalInfo &term) : m_term(term)
{
}

const TerminalInfo& Session::terminal() const
{
    return m_term;
}

Poco::UInt32 Session::sequence() const
{
    return m_term.socket_fd << 16 | m_sequence;
}

NS_VCN_OAM_END
