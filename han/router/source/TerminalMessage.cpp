
#include "TerminalMessage.h"
NS_VCN_OAM_BEGIN(ROUTER)

MessageInfo* MessageInfo::json2message(Poco::UInt32 type, const std::string &json_str)
{
    Json::Value val;
    Json::Reader reader;
    if(!reader.parse(json_str, val))
        throw std::logic_error("failed to parse json string");

    switch(type){
    case EV_ROUTER_TELNET_HEARTBEAT_REQ:
        return HeartbeatRequest::make(val);
    case EV_ROUTER_TELNET_LOGIN_REQ:
        return LoginRequest::make(val);
    case EV_ROUTER_TELNET_LOGIN_ACK:
        return LoginResponse::make(val);
    case EV_ROUTER_TELNET_LOGOUT_REQ:
        return LogoutRequest::make(val);
    case EV_ROUTER_TELNET_COMMAND_REQ:
        return CommandRequest::make(val);
    case EV_ROUTER_TELNET_COMMAND_ACK:
        return CommandResponse::make(val);
    case EV_ROUTER_TELNET_BREAKCOMMAND_REQ:
        return BreakCommandRequest::make(val);
    case EV_ROUTER_TELNET_NEST_REQ:
        return NestCommandRequest::make(val);
    case EV_ROUTER_TELNET_NEST_ACK:
        return NestCommandResponse::make(val);
    case EV_ROUTER_TELNET_CHANGE_PROMPT:
        return PromptRequest::make(val);
    case EV_ROUTER_TELNET_MULTIPACKETS_ACK:
        return MultiPacketsResponse::make(val);
    case EV_ROUTER_RESTFUL_REQ:
        return RESTfulRequest::make(val);
    case EV_ROUTER_RESTFUL_ACK:
        return RESTfulResponse::make(val);
    default:
        throw std::logic_error(std::string("undefine message type:") + std::to_string(type));
    }
}

NS_VCN_OAM_END
