#ifndef TCFSHEADPARSER_H_INCLUDED
#define TCFSHEADPARSER_H_INCLUDED

#include <string>
#include <functional>
#include <memory>
#include <map>
#include <initializer_list>

#include "tcfs.h"
#include "Poco/String.h"

#include "vcn_defs.h"
#include "TerminalMessage.h"

using namespace vcn::omm;

NS_VCN_OAM_BEGIN(ROUTER)

struct TCFS_Header_Parser{
    static std::string encode(const std::string &src, const std::string &dst)
    {
        std::string header;
        header.reserve(64);
        header = src;
        header += '|';
        header += dst;
        header += '\n';
        return header;
    }

    static const char* decode(const char* data, std::string &header, std::string &src, std::string &dst)
    {
        const char *body = strchr(data, '\n');
        if (body == nullptr)
            return nullptr;

        body += 1;
        header.assign(data, body - data);
        std::size_t pos = header.find('|');
        if (pos == std::string::npos)
            return nullptr;

        src = header.substr(0, pos);
        dst = header.substr(pos + 1);
        return body;
    }
};

using MESSAGE_DISPATCH_FUNC = std::function<void (const MessageInfo&)>;

class TCFSAdapterBase
{
public:
    TCFSAdapterBase()
    {
        CREATE_LOG();
    }

    virtual ~TCFSAdapterBase()
    {
    }

    template<typename C>
    void callback_dispatch_function(C &c, void (C::*f)(const MessageInfo&))
    {
        m_dispatch_handler = [&c, f](const MessageInfo &message) -> void{
            (c.*f)(message);
        };
    }

    template<typename C>
    void callback_response_function(C &c, void (C::*f)(const MessageInfo&))
    {
        m_response_handler = [&c, f](const MessageInfo &message) -> void{
            (c.*f)(message);
        };
    }

    virtual int dispatch(const MessageInfo &message)
    {
        if (!m_dispatch_handler)
            return -1;

        m_dispatch_handler(message);
        return 0;
    }

    virtual void response(const MessageInfo &message)
    {
        if (!m_response_handler)
            return;

        m_response_handler(message);
    }

    virtual void start()
    {
    }

    std::string& operator[](const std::string &key)
    {
        return m_parameters[key];
    }

protected:
    MESSAGE_DISPATCH_FUNC m_dispatch_handler;
    MESSAGE_DISPATCH_FUNC m_response_handler;

    DECLARE_LOG();

private:
    std::map<std::string, std::string> m_parameters;
};
using TCFSAdapterBasePtr = std::shared_ptr<TCFSAdapterBase>;

class TCFSAdapter : public TCFSAdapterBase
{
public:
    TCFSAdapter()
    {
    }

    ~TCFSAdapter()
    {
    }

public:
    void start() override
    {
        if (m_worker)
            return;

        m_worker_session_type   = (*this)["src_session"];
        m_pub_session_type      = (*this)["pub_session"];
        m_pub_instance          = (*this)["pub_instance"];

        m_worker = std::shared_ptr<WorkerBase>(new WorkerBase());
        this->callback_dispatch_function(*this, &TCFSAdapter::tcfs_publish);

        m_worker_instance = m_worker_session_type + "/" + m_worker->instance();
        m_worker->callback(*this, &TCFSAdapter::tcfs_callback);
        m_worker->subscribe(m_worker_session_type, m_worker->instance());
    }

private:
    void tcfs_callback(Poco::UInt32 event, const void *data, std::size_t length)
    {
        std::string session_type, session_instance;
        m_worker->current_session(session_type, session_instance);

        logDebug("tcfs callback: session_type=%s, instance=%s, msg_id=%u, data=%s",
                 session_type.c_str(),
                 session_instance.c_str(),
                 event,
                 std::string((char*)data, length).c_str());

        std::string header, src, dst;
        if (nullptr == (data = TCFS_Header_Parser::decode((const char*)data, header, src, dst))){
            logError("TCFS Header Parser fail");
            return;
        }

        MessageInfo *message = MessageInfo::json2message(event, (const char*)data);
        std::shared_ptr<MessageInfo> protect_ptr(message);
        response(*message);
    }

    void tcfs_publish(const MessageInfo &message)
    {
        Poco::UInt32 event = message.type();
        std::string header = TCFS_Header_Parser::encode(m_worker_instance, m_pub_instance);
        std::string message_str = header + static_cast<std::string>(message);

        int ret = m_worker->publish(m_pub_session_type, m_worker->instance(), event, message_str.c_str(), message_str.length());
        logDebug("tcfs publish(%d): session_type=%s, instance=%s, msg_id=%u, data=%s", 
                 ret, 
                 m_pub_session_type.c_str(),
                 m_worker->instance().c_str(),
                 event,
                 message_str.c_str());
    }

private:
    std::shared_ptr<WorkerBase> m_worker;
    std::string                 m_worker_instance;
    std::string                 m_worker_session_type;
    std::string                 m_pub_session_type;
    std::string                 m_pub_instance;
};
NS_VCN_OAM_END

#endif // TCFSHEADPARSER_H_INCLUDED
