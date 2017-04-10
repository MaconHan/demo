/**@file
* 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
* 文件名称：zteomm/oms/NDFhandle.h<br>
* 文件标识：
* 内容摘要：SSS 2.0 NDF通信服务头文件<br>
* 其它说明：无<br>
* @version 1.0<br>
* @author <br>
* @since 2009年5月12日<br>
*/

#ifndef NDF_MSG_H
#define NDF_MSG_H

#include "json/json.h"
#include "syscomm.h"
#include "vcn_defs.h"
#include "vcn_events_router.h"
#include "TerminalInfo.h"

using namespace zteomm::pub;

NS_VCN_OAM_BEGIN(ROUTER)

struct MessageInfo
{
    MessageInfo(Poco::UInt32 type, Poco::UInt32 sequence = INVALID_SEQUENCE) : m_type(type), m_sequence(sequence)
    {
    }

    virtual ~MessageInfo()
    {
    }

    Poco::UInt32 type() const
    {
        return m_type;
    }

    Poco::UInt32 sequence() const
    {
        return m_sequence;
    }

    const MessageInfo& operator >> (Json::Value &val) const
    {
        val["sequence"] = m_sequence;
        val["event"]    = m_type;
        return *this;
    }

    virtual operator Json::Value () const
    {
        Json::Value val;
        *this >> val;
        return val;
    }

    operator std::string () const
    {
        Json::Value val = *this;
        return Json::FastWriter().write(val);
    }

    static MessageInfo* json2message(Poco::UInt32 type, const std::string &json_str);

private:
    Poco::UInt32    m_type;
    Poco::UInt32    m_sequence;
};

struct ErrorInfo{
    Poco::Int32 code;
    std::string descript;

    const ErrorInfo& operator >> (Json::Value &val) const
    {
        val["errcode"] = code;
        val["errmsg"]  = descript;
        return *this;
    }

    static ErrorInfo make(const Json::Value &val)
    {
        return {val["errcode"].asInt(), val["errmsg"].asString()};
    }
};

struct MessageResult : public MessageInfo
{
    MessageResult(Poco::UInt32 type, Poco::UInt32 sequence, const ErrorInfo &error = {0, ""}) : MessageInfo(type, sequence), m_error(error)
    {
    }

    Poco::Int32 error_code() const
    {
        return m_error.code;
    }

    const std::string& error_descript() const
    {
        return m_error.descript;
    }

    const MessageResult& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        m_error >> val;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

private:
    ErrorInfo m_error;
};

struct HeartbeatRequest : public MessageInfo
{
    HeartbeatRequest(const std::string &token) : MessageInfo(EV_ROUTER_TELNET_HEARTBEAT_REQ, 0), m_token(token)
    {
    }

    const std::string& token() const
    {
        return m_token;
    }

    const HeartbeatRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["token"] = m_token;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        *this >> val;
        return val;
    }

    static HeartbeatRequest* make(const Json::Value &val)
    {
        HeartbeatRequest *message = new HeartbeatRequest(val["token"].asString());
        return message;
    }

private:
    std::string m_token;
};

struct LoginRequest : public MessageInfo
{
    LoginRequest(Poco::UInt32 sequence, const std::string &user, const std::string &password, const std::string &client, const std::string &type, Poco::UInt16 timeout = 15) :
        MessageInfo(EV_ROUTER_TELNET_LOGIN_REQ, sequence),
        m_user(user),
        m_password(password),
        m_client(client),
        m_type(type),
        m_timeout(timeout)
    {
    }

    const std::string& user() const
    {
        return m_user;
    }

    const std::string& password() const
    {
        return m_password;
    }

    const std::string& client() const
    {
        return m_client;
    }

    const std::string& type() const
    {
        return m_type;
    }

    Poco::UInt16 timeout() const
    {
        return m_timeout;
    }

    const LoginRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["user"]         = m_user;
        val["password"]     = m_password;
        val["client"]       = m_client;
        val["type"]         = m_type;
        val["timeout"]      = m_timeout;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static LoginRequest* make(const Json::Value &val)
    {
        LoginRequest *message = new LoginRequest(val["sequence"].asUInt(),
                                                 val["user"].asString(),
                                                 val["password"].asString(),
                                                 val["client"].asString(),
                                                 val["type"].asString(),
                                                 val["timeout"].asUInt());
        return message;
    }

private:
    std::string     m_user;
    std::string     m_password;
    std::string     m_client;
    std::string     m_type;
    Poco::UInt16    m_timeout;
};

struct LoginResponse : public MessageResult
{
    LoginResponse(Poco::UInt32 sequence, const ErrorInfo &error, const std::string &token) :
        MessageResult(EV_ROUTER_TELNET_LOGIN_ACK, sequence, error),
        m_token(token)
    {
    }

    const std::string& token() const
    {
        return m_token;
    }

    const LoginResponse& operator >> (Json::Value &val) const
    {
        (MessageResult&)(*this) >> val;
        val["token"] = m_token;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        *this >> val;
        return val;
    }

    static LoginResponse* make(const Json::Value &val)
    {
        LoginResponse *message = new LoginResponse(val["sequence"].asUInt(), ErrorInfo::make(val), val["token"].asString());
        return message;
    }

private:
    std::string m_token;
};

struct LogoutRequest : public MessageInfo
{
    LogoutRequest(const std::string &token) : MessageInfo(EV_ROUTER_TELNET_LOGOUT_REQ, 0), m_token(token)
    {
    }

    const std::string& token() const
    {
        return m_token;
    }

    const LogoutRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["token"] = m_token;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        *this >> val;
        return val;
    }

    static LogoutRequest* make(const Json::Value &val)
    {
        LogoutRequest *message = new LogoutRequest(val["token"].asString());
        return message;
    }

private:
    std::string m_token;
};

struct CommandRequest : public MessageInfo
{
    CommandRequest(Poco::UInt32 sequence, const std::string &token, const std::string &command, const std::string &additional, const std::string &mode = "") :
        MessageInfo(EV_ROUTER_TELNET_COMMAND_REQ, sequence),
        m_token(token),
        m_command(command),
        m_additional(additional),
        m_mode(mode)
    {
    }

    const std::string& token() const
    {
        return m_token;
    }

    const std::string& command() const
    {
        return m_command;
    }

    const std::string& mode() const
    {
        return m_mode;
    }

    const std::string& additional() const
    {
        return m_additional;
    }

    const CommandRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["token"]      = m_token;
        val["data"]       = m_command;
        val["input_type"] = "text";
        val["output_type"]= "text/table";
        val["mutualInfo"] = m_additional;
        val["determiner"] = m_mode;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static CommandRequest* make(const Json::Value &val)
    {
        CommandRequest *message = new CommandRequest(val["sequence"].asUInt(),
                                                     val["token"].asString(),
                                                     val["data"].asString(),
                                                     val["mutualInfo"].asString(),
                                                     val["determiner"].asString());
        return message;
    }

private:
    std::string m_token;
    std::string m_command;
    //附加信息
    std::string m_additional;
    //命令执行模式：命令执行，智能提示等
    std::string m_mode;
};

struct CommandResponse : public MessageResult
{
    CommandResponse(Poco::UInt32 sequence, const ErrorInfo &error, const std::string &result, bool lastpack = true, bool asyncmsg = false, bool multipackets = false) :
        MessageResult(EV_ROUTER_TELNET_COMMAND_ACK, sequence, error),
        m_result(result),
        m_lastpack(lastpack),
        m_asyncmsg(asyncmsg),
        m_multipackets(multipackets)
    {
    }

    bool lastpack() const
    {
        return m_lastpack;
    }

    bool asyncmsg() const
    {
        return m_asyncmsg;
    }

    bool multipackets() const
    {
        return m_multipackets;
    }

    const std::string& result() const
    {
        return m_result;
    }

    const std::string& additional() const
    {
        return m_additional;
    }

    const CommandResponse& operator >> (Json::Value &val) const
    {
        (MessageResult&)(*this) >> val;
        val["data"]       = m_result;
        val["lastpack"]   = m_lastpack ? 1 : 0;
        val["asyncmsg"]   = m_asyncmsg ? 1 : 0;
        val["mutualInfo"] = m_additional;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static CommandResponse* make(const Json::Value &val)
    {
        CommandResponse *message = new CommandResponse(val["sequence"].asUInt(),
                                                       ErrorInfo::make(val),
                                                       val["data"].asString(),
                                                       val["lastpack"].asUInt() != 0,
                                                       val["asyncmsg"].asUInt() != 0,
                                                       val["multipackets"].asUInt() != 0);

        message->m_additional = val["mutualInfo"].asString();
        return message;
    }

private:
    std::string     m_result;
    bool            m_lastpack;
    bool            m_asyncmsg;
    bool            m_multipackets;
    //附加信息
    std::string     m_additional;
};

struct BreakCommandRequest : public MessageInfo
{
    BreakCommandRequest(Poco::UInt32 sequence, const std::string &token) :
        MessageInfo(EV_ROUTER_TELNET_BREAKCOMMAND_REQ, sequence),
        m_token(token)
    {
        if (sequence == 0)
            return;
    }

    const std::string& token() const
    {
        return m_token;
    }

    const BreakCommandRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["token"] = m_token;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static BreakCommandRequest* make(const Json::Value &val)
    {
        BreakCommandRequest *message = new BreakCommandRequest(val["sequence"].asUInt(), val["token"].asString());
        return message;
    }

private:
    std::string m_token;
};

struct NestCommandRequest : public MessageInfo
{
    NestCommandRequest(Poco::UInt32 sequence, const std::string &command, const std::string &token) :
        MessageInfo(EV_ROUTER_TELNET_NEST_REQ, sequence),
        m_token(token),
        m_command(command)
    {
    }

    const std::string& token() const
    {
        return m_token;
    }

    const std::string& command() const
    {
        return m_command;
    }

    const std::string& additional() const
    {
        return m_additional;
    }

    const NestCommandRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["token"]        = m_token;
        val["data"]         = m_command;
        val["mutualInfo"]   = m_additional;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static NestCommandRequest* make(const Json::Value &val)
    {
        NestCommandRequest *message = new NestCommandRequest(val["sequence"].asUInt(), val["data"].asString(), val["token"].asString());
        message->m_additional = val["mutualInfo"].asString();
        return message;
    }

private:
    std::string m_token;
    std::string m_command;
    //附加信息
    std::string m_additional;
};

struct NestCommandResponse : public MessageResult
{
    NestCommandResponse(Poco::UInt32 sequence, const std::string &result, const std::string &token, const std::string &additional) :
        MessageResult(EV_ROUTER_TELNET_NEST_ACK, sequence),
        m_result(result),
        m_token(token),
        m_additional(additional)
    {
    }

    const std::string& token() const
    {
        return m_token;
    }

    const std::string& result() const
    {
        return m_result;
    }

    const std::string& additional() const
    {
        return m_additional;
    }

    const NestCommandResponse& operator >> (Json::Value &val) const
    {
        (MessageResult&)(*this) >> val;
        val["token"]        = m_token;
        val["data"]         = m_result;
        val["mutualInfo"]   = m_additional;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static NestCommandResponse* make(const Json::Value &val)
    {
        NestCommandResponse *message = new NestCommandResponse(val["sequence"].asUInt(),
                                                               val["data"].asString(),
                                                               val["token"].asString(),
                                                               val["mutualInfo"].asString());
        return message;
    }

private:
    std::string m_token;
    std::string m_result;
    //附加信息
    std::string m_additional;
};

struct PromptRequest : public MessageInfo
{
    PromptRequest(Poco::UInt32 sequence, const std::string &prompt, const std::string &token) :
        MessageInfo(EV_ROUTER_TELNET_CHANGE_PROMPT, sequence),
        m_token(token),
        m_prompt(prompt)
    {
    }

    const std::string& token() const
    {
        return m_token;
    }

    const std::string& prompt() const
    {
        return m_prompt;
    }

    const PromptRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["token"]    = m_token;
        val["data"]     = m_prompt;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static PromptRequest* make(const Json::Value &val)
    {
        PromptRequest *message = new PromptRequest(val["sequence"].asUInt(), val["data"].asString(), val["token"].asString());
        return message;
    }

private:
    std::string m_token;
    std::string m_prompt;
};

struct MultiPacketsRequest : public MessageInfo
{
    MultiPacketsRequest(Poco::UInt32 sequence, const std::string &token) :
        MessageInfo(EV_ROUTER_TELNET_MULTIPACKETS_REQ, sequence),
        m_token(token)
    {
    }

    const std::string& token() const
    {
        return m_token;
    }

    const MultiPacketsRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["token"]    = m_token;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

private:
    std::string m_token;
};

struct MultiPacketsResponse : public MessageResult
{
    MultiPacketsResponse(Poco::UInt32 sequence, const ErrorInfo &error, const std::string &result, bool lastpack = true) :
        MessageResult(EV_ROUTER_TELNET_MULTIPACKETS_ACK, sequence, error),
        m_result(result),
        m_lastpack(lastpack)
    {
    }

    bool lastpack() const
    {
        return m_lastpack;
    }

    const std::string& result() const
    {
        return m_result;
    }

    const MultiPacketsResponse& operator >> (Json::Value &val) const
    {
        (MessageResult&)(*this) >> val;
        val["data"]       = m_result;
        val["lastpack"]   = m_lastpack ? 1 : 0;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static MultiPacketsResponse* make(const Json::Value &val)
    {
        MultiPacketsResponse *message = new MultiPacketsResponse(val["sequence"].asUInt(),
                                                                 ErrorInfo::make(val),
                                                                 val["data"].asString(),
                                                                 val["lastpack"].asUInt() != 0);
        return message;
    }

private:
    std::string     m_result;
    bool            m_lastpack;
};

struct RESTfulRequest : public MessageInfo
{
    RESTfulRequest(Poco::UInt32 sequence) : MessageInfo(EV_ROUTER_RESTFUL_REQ, sequence)
    {
    }

    const RESTfulRequest& operator >> (Json::Value &val) const
    {
        (MessageInfo&)(*this) >> val;
        val["client"]           = client;
        val["url"]              = url;
        val["method"]           = method;
        val["username"]         = username;
        val["auth_code"]        = auth_code;
        val["language_option"]  = language_option;
        val["timeout"]          = timeout;
        val["type"]             = type;
        val["body"]             = body;
        /*
        Json::Value body_json;
        if (body.empty())
            val["body"] = Json::Value::null;
        else if(Json::Reader().parse(body, body_json))
            val["body"] = body_json;
        else
            val["body"] = body;
        */
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static RESTfulRequest* make(const Json::Value &val)
    {
        RESTfulRequest *message = new RESTfulRequest(val["sequence"].asUInt());
        message->client     = val["client"].asString();
        message->url        = val["url"].asString();
        message->method     = val["method"].asString();
        message->username   = val["username"].asString();
        message->auth_code  = val["auth_code"].asString();
        message->language_option = val["language_option"].asString();
        message->timeout    = val["timeout"].asUInt();
        message->type       = val["type"].asString();
        message->body       = val["body"].asString();
        return message;
    }

public:
    std::string     client;
    std::string     url;
    std::string     method;
    std::string     username;
    std::string     auth_code;
    std::string     language_option;
    Poco::UInt32    timeout;
    std::string     type;
    std::string     body;
};

struct RESTfulResponse : public MessageResult
{
    RESTfulResponse(Poco::UInt32 sequence, const ErrorInfo &error, const std::string &body) : MessageResult(EV_ROUTER_RESTFUL_ACK, sequence, error), m_body(body)
    {
    }

    const std::string& body() const
    {
        return m_body;
    }

    const RESTfulResponse& operator >> (Json::Value &val) const
    {
        (MessageResult&)(*this) >> val;
        val["body"] = m_body;
        return *this;
    }

    operator Json::Value () const
    {
        Json::Value val;
        (*this) >> val;
        return val;
    }

    static RESTfulResponse* make(const Json::Value &val)
    {
        Json::Value sequence    = val["sequence"];
        ErrorInfo error         = ErrorInfo::make(val);
        Json::Value body        = val["body"];
        std::string body_str;
        if (body.isString()){
            body_str = body.asString();
        }
        else if (body.isObject()){
            body_str = Json::FastWriter().write(body);
        }

        RESTfulResponse *message = new RESTfulResponse(sequence.asUInt(), error, body_str);
        return message;
    }

private:
    std::string m_body;
};

NS_VCN_OAM_END

#endif

