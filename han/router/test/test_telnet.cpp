#include <stdio.h>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <future>
#include <map>
#include <tuple>
#include <functional>
#include <stdlib.h>

#include "Poco/String.h"
#include "Poco/Types.h"
#include "Poco/Timespan.h"
#include "Poco/StringTokenizer.h"
#include "Poco/StreamCopier.h"
#include "Poco/StreamTokenizer.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/SocketStream.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"

#include "gtest/gtest.h"
#include "json/json.h"

#include "TelnetHandle.h"
#include "TerminalServer.h"
#include "TCFSAdapter.h"
#include "OAMRouterService.h"

using namespace Poco;
using namespace Poco::Net;

NS_VCN_OAM_BEGIN(ROUTER)

struct Test_Router_Telnet : public testing::Test{
    Test_Router_Telnet()
    {
        tp.Type = "telnet/normal";
        tp.IP   = "127.0.0.1";
        tp.Port = 7777;
    }

    void SetUp() override{
    }

    void TearDown() override{
    }

    TelnetProperty tp;
};

struct TelnetInputToken: public Token
{
    TelnetInputToken()
    {
    }

    bool start(char c, std::istream& istr)
    {
        _value = c;
        return !(c == EOF || c == '\n' || c == '\r');
    }

    void finish(std::istream& istr)
    {
        do{
            auto c = istr.get();
            if (c == EOF || c == '\n' || c == '\r')
                break;

            _value += c;
            if (std::find(m_token_v.begin(), m_token_v.end(), _value) != m_token_v.end())
                break;
        }while(1);
    }

protected:
    std::vector<std::string> m_token_v = {TelnetConst::LOGIN_PROMPT, TelnetConst::PASSWORD_PROMPT, TelnetConst::TELNET_PROMPT};
};

struct OMM_Control_Worker : public TCFSAdapterBase
{
    void response(const MessageInfo &message)
    {
        auto it = m_handler_map.find(message.type());
        if (it == m_handler_map.end())
            return;

        auto dispatcher = [this](const MessageInfo &message){
            this->dispatch(message);
        };
        it->second(message, dispatcher);
    }

    template<typename F>
    void register_handler(Poco::UInt32 type, F f)
    {
        m_handler_map[type] = f;
    }

private:
    using MESSAGE_HANDLER = function<void (const MessageInfo&, MESSAGE_DISPATCH_FUNC)>;
    std::map<Poco::UInt32, MESSAGE_HANDLER> m_handler_map;
};
using ControlWorkerPtr = std::shared_ptr<OMM_Control_Worker>;

struct RoundWorkBinder
{
    typedef std::function<void ()>    OPERATE_TYPE;

    RoundWorkBinder(TCFSAdapterBasePtr source, TCFSAdapterBasePtr destination) :
        m_source(source),
        m_destination(destination),
        m_stop(false)
    {
        m_source->callback_dispatch_function(*this, &RoundWorkBinder::dispatch);
        m_destination->callback_dispatch_function(*this, &RoundWorkBinder::response);

        using namespace std::placeholders;
        std::function<void (std::mutex &mutex, std::list<OPERATE_TYPE> &data)> handler_func = std::bind(&RoundWorkBinder::handler, this, _1, _2);

        m_handler_thr_list.push_back(std::thread(handler_func, std::ref(this->m_msg_mutex), std::ref(this->m_request_list)));
        m_handler_thr_list.push_back(std::thread(handler_func, std::ref(this->m_msg_mutex), std::ref(this->m_request_list)));
        m_handler_thr_list.push_back(std::thread(handler_func, std::ref(this->m_msg_mutex), std::ref(this->m_response_list)));
    }

    ~RoundWorkBinder()
    {
        std::lock_guard<std::mutex> locker(m_msg_mutex);

        m_stop = true;
        for(auto i = 0; i != m_handler_thr_list.size(); ++i)
            m_handler_thr_list[i].join();
    }

    void dispatch(const MessageInfo &message)
    {
        std::string message_str = message;
        Poco::UInt32 type = message.type();

        auto func = [this, type, message_str](){
            std::cout << "request: " << message_str << std::flush;
            MessageInfo *message = MessageInfo::json2message(type, message_str);
            std::shared_ptr<MessageInfo> protect_ptr(message);
            this->m_destination->response(*message);
        };

        std::lock_guard<std::mutex> locker(m_msg_mutex);
        m_request_list.push_back(func);
    }

    void response(const MessageInfo &message)
    {
        std::string message_str = message;
        Poco::UInt32 type = message.type();

        auto func = [this, type, message_str](){
            std::cout << "response: " << message_str << std::flush;
            MessageInfo *message = MessageInfo::json2message(type, message_str);
            std::shared_ptr<MessageInfo> protect_ptr(message);
            this->m_source->response(*message);
        };

        std::lock_guard<std::mutex> locker(m_msg_mutex);
        m_response_list.push_back(func);
    }

private:
    void handler(std::mutex &mutex, std::list<OPERATE_TYPE> &data)
    {
        while(!m_stop){
            if (data.empty()){
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            OPERATE_TYPE func;
            {
                std::lock_guard<std::mutex> locker(mutex);
                if (data.empty())
                    continue;

                func = data.front();
                data.pop_front();
            }

            func();
        }
    }

private:
    TCFSAdapterBasePtr m_source;
    TCFSAdapterBasePtr m_destination;
    bool               m_stop;

    std::mutex                      m_msg_mutex;
    std::list<OPERATE_TYPE>         m_request_list;
    std::list<OPERATE_TYPE>         m_response_list;
    std::vector<std::thread>        m_handler_thr_list;
};

TEST_F(Test_Router_Telnet, Login)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    std::string user = "admin", pwd = "admin123";
    auto login_func = [user, pwd](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        EXPECT_EQ(login.user(), user);
        EXPECT_EQ(login.password(), pwd);

        dispatcher(LoginResponse(login.sequence(), {0, ""}, "abc123"));
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();

        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << user << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << pwd << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            str << TelnetConst::EOT << std::flush;
        }
    }
}

TEST_F(Test_Router_Telnet, LOGIN_FAIL)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    Poco::Int32 login_error_code = 1;
    std::string login_error_msg = "invalid user";
    auto login_func = [login_error_code, login_error_msg](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {login_error_code, login_error_msg}, ""));
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    int login_count = 0;
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
            ++login_count;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
    }

    EXPECT_EQ(login_count, 3);
}

TEST_F(Test_Router_Telnet, CMD_ECHO_SingleAck)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    std::string token = "abc123";
    auto login_func = [token](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, token));
    };
    auto command_func = [token](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const CommandRequest &cmd = dynamic_cast<const CommandRequest&>(message);
        EXPECT_EQ(token, cmd.token());
        dispatcher(CommandResponse(cmd.sequence(), {0, ""}, cmd.command()));
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);
    control_worker->register_handler(EV_ROUTER_TELNET_COMMAND_REQ, command_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    std::string mml = "show user;";
    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            str << mml << TelnetConst::CRLF << std::flush;

            //输入回显到客户端
            std::getline(str, line);

            //输出运行结果
            std::getline(str, line);
            EXPECT_EQ(mml, Poco::trimRight(line));

            str << TelnetConst::EOT;
            str.flush();
            break;
        }
    }
}

TEST_F(Test_Router_Telnet, CMD_ECHO_MultiAck)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    auto login_func = [](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, "abc123"));
    };
    auto ack_count = 3;
    auto command_func = [ack_count](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        std::cout << "CMD_ECHO_MultiAck::command_func" << std::endl << std::flush;
        const CommandRequest &cmd = dynamic_cast<const CommandRequest&>(message);
        std::string ack = cmd.command() + TelnetConst::CRLF;
        for(auto i = 1; i <= ack_count; ++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            dispatcher(CommandResponse(cmd.sequence(), {0, ""}, ack, i == ack_count));
        }
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);
    control_worker->register_handler(EV_ROUTER_TELNET_COMMAND_REQ, command_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    std::string mml = "show user:name=\"admin\";";
    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            str << mml << TelnetConst::CRLF << std::flush;

            //获取输入回显到客户端
            std::getline(str, line);

            //接收应答回显信息
            for(auto i = 0; i < ack_count; ++i){
                std::getline(str, line);
                EXPECT_EQ(mml, Poco::trim(line));
            }

            //输入标示符, zte:>
            std::string delim = ">";
            std::getline(str, line, delim[0]);
            EXPECT_EQ(TelnetConst::TELNET_PROMPT, Poco::trim(line) + delim);

            str << TelnetConst::EOT << std::flush;
            break;
        }
    }
}

TEST_F(Test_Router_Telnet, CMD_ECHO_AsyncAck)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    auto login_func = [](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, "abc123"));
    };
    auto ack_count = 3;
    auto command_func = [ack_count](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const CommandRequest &cmd = dynamic_cast<const CommandRequest&>(message);
        std::string ack = cmd.command() + TelnetConst::CRLF;
        for(auto i = 1; i <= ack_count; ++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            dispatcher(CommandResponse(cmd.sequence(), {0, ""}, ack, i == ack_count, true));
        }
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);
    control_worker->register_handler(EV_ROUTER_TELNET_COMMAND_REQ, command_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    std::string mml = "show user:name=\"admin\";";
    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            str << mml << TelnetConst::CRLF << std::flush;

            //输入回显到客户端
            std::getline(str, line);

            //第一条
            std::getline(str, line);
            EXPECT_EQ(mml, Poco::trimRight(line));

            //输入标示符, zte:>
            std::string delim = ">";
            std::getline(str, line, delim[0]);
            EXPECT_EQ(TelnetConst::TELNET_PROMPT, Poco::trim(line) + delim);

            //第二条
            std::getline(str, line);
            EXPECT_EQ(mml, Poco::trimRight(line));

            //第三条
            std::getline(str, line);
            EXPECT_EQ(mml, Poco::trimRight(line));

            str << TelnetConst::EOT << std::flush;
            break;
        }
    }
}

TEST_F(Test_Router_Telnet, CMD_ECHO_NestCommand_Yes)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    Poco::UInt32 sequence = -1;
    std::string nest_result;
    std::string danger_tips = TelnetConst::NEST_PREFIX + "Are you sure to delete?(Y/N):";
    Poco::Event nest_event;
    auto login_func = [](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, "abc123"));
    };
    auto command_func = [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const CommandRequest &cmd = dynamic_cast<const CommandRequest&>(message);
        sequence = cmd.sequence();
        std::string ack = cmd.command();

        nest_event.reset();
        dispatcher(NestCommandRequest(sequence, danger_tips, cmd.token()));
        nest_event.wait();

        EXPECT_EQ("Y", Poco::trim(Poco::toUpper(nest_result)));
        dispatcher(CommandResponse(sequence, {0, ""}, ack));
    };
    auto nestcommand_func =  [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const NestCommandResponse &nest = dynamic_cast<const NestCommandResponse&>(message);
        EXPECT_EQ(sequence, nest.sequence());
        nest_result = nest.result();
        nest_event.set();
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);
    control_worker->register_handler(EV_ROUTER_TELNET_COMMAND_REQ, command_func);
    control_worker->register_handler(EV_ROUTER_TELNET_NEST_ACK, nestcommand_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    std::string mml = "show user:name=\"admin\";";
    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            str << mml << TelnetConst::CRLF << std::flush;
            //输入回显到客户端,读取输入显式在客户端上
            std::getline(str, line);

            //嵌套请求，危险命令。获取提示信息
            std::getline(str, line, ':');
            EXPECT_EQ(danger_tips, line + ":");

            std::string yes = "y";
            str << yes << TelnetConst::CRLF << std::flush;

            //读取输入yes显式在客户端上
            std::getline(str, line);

            //执行结果
            std::getline(str, line);
            EXPECT_EQ(mml, Poco::trim(line));

            //输入标示符, zte:>
            std::string delim = ">";
            std::getline(str, line, delim[0]);
            EXPECT_EQ(TelnetConst::TELNET_PROMPT, Poco::trim(line) + delim);

            str << TelnetConst::EOT << std::flush;
            break;
        }
    }
}

TEST_F(Test_Router_Telnet, CMD_ECHO_NestCommand_No)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    std::string nest_result;
    std::string danger_tips = TelnetConst::NEST_PREFIX + "Are you sure to delete?(Y/N):";
    std::string error = "cancelled";
    Poco::Event nest_event;
    auto login_func = [](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, "abc123"));
    };
    auto command_func = [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const CommandRequest &cmd = dynamic_cast<const CommandRequest&>(message);
        Poco::UInt32 sequence = cmd.sequence();
        //std::string ack = cmd.command();

        nest_event.reset();
        dispatcher(NestCommandRequest(sequence, danger_tips, cmd.token()));
        nest_event.wait();

        EXPECT_NE("Y", Poco::trim(Poco::toUpper(nest_result)));
        dispatcher(CommandResponse(sequence, {1, error}, ""));
    };
    auto nestcommand_func =  [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const NestCommandResponse &nest = dynamic_cast<const NestCommandResponse&>(message);
        nest_result = nest.result();
        nest_event.set();
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);
    control_worker->register_handler(EV_ROUTER_TELNET_COMMAND_REQ, command_func);
    control_worker->register_handler(EV_ROUTER_TELNET_NEST_ACK, nestcommand_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    std::string mml = "show user:name=\"admin\";";
    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            str << mml << TelnetConst::CRLF << std::flush;

            //读取输入显式在客户端上
            std::getline(str, line);

            //嵌套请求，危险命令
            std::string no = "n";
            std::getline(str, line, ':');
            EXPECT_EQ(danger_tips, line + ":");
            str << no << TelnetConst::CRLF << std::flush;

            //读取输入no显式在客户端上
            std::getline(str, line);

            //失败打印
            std::getline(str, line);
            EXPECT_NE(mml, line);
            EXPECT_EQ(error, line.substr(0, error.size()));

            //输入标示符, zte:>
            std::string delim = ">";
            std::getline(str, line, delim[0]);
            EXPECT_EQ(TelnetConst::TELNET_PROMPT, Poco::trim(line) + delim);

            str << TelnetConst::EOT << std::flush;
            break;
        }
    }
}

TEST_F(Test_Router_Telnet, CMD_ECHO_NestCommand_Break)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    Poco::Event nest_event, break_event, return_break;
    Poco::UInt32 sequence = -1;
    const std::string danger_tips = TelnetConst::NEST_PREFIX + "Are you sure to delete?(Y/N):";
    const std::string token = "abc123";

    auto login_func = [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, token));
    };
    auto command_func = [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const CommandRequest &cmd = dynamic_cast<const CommandRequest&>(message);
        sequence = cmd.sequence();

        nest_event.reset();
        dispatcher(NestCommandRequest(sequence, danger_tips, cmd.token()));
        nest_event.wait();

        EXPECT_EQ(true, break_event.tryWait(0));
        return_break.set();
    };
    auto breakcommand_func = [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const BreakCommandRequest &breakcmd = dynamic_cast<const BreakCommandRequest&>(message);
        EXPECT_EQ(sequence, breakcmd.sequence());
        EXPECT_EQ(token, breakcmd.token());

        break_event.set();
        nest_event.set();
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);
    control_worker->register_handler(EV_ROUTER_TELNET_COMMAND_REQ, command_func);
    control_worker->register_handler(EV_ROUTER_TELNET_BREAKCOMMAND_REQ, breakcommand_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    std::string mml = "show user:name=\"admin\";";
    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            str << mml << TelnetConst::CRLF << std::flush;

            //读取输入显式在客户端上
            std::getline(str, line);

            //嵌套请求，危险命令
            std::getline(str, line, ':');
            EXPECT_EQ(danger_tips, line + ":");

            //中断执行
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            str << TelnetConst::SC << std::flush;

            //输入标示符, zte:>
            std::string delim = ">";
            std::getline(str, line, delim[0]);
            EXPECT_EQ(TelnetConst::TELNET_PROMPT, Poco::trim(line) + delim);

            str << TelnetConst::EOT << std::flush;
            break;
        }
    }

    //需要等待command_func和breakcommand_func执行完,否则这两个函数还未执行完,该函数就已经退出了.
    return_break.wait();
}

TEST_F(Test_Router_Telnet, ChangePrompt)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    const std::string new_prompt = "ZTE:>";
    auto login_func = [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, "abc123"));

        //发送改变命令提示符的请求
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::thread thr(dispatcher, PromptRequest(login.sequence(), new_prompt, ""));
        thr.detach();
    };

    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            std::string delim = ">";
            //修改后的命令提示符, ZTE:>
            std::getline(str, line, delim[0]);
            EXPECT_EQ(new_prompt, Poco::trim(line) + delim);

            str << TelnetConst::EOT << std::flush;
            break;
        }
    }
}

TEST_F(Test_Router_Telnet, KeyboardControl)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    auto login_func = [](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, "abc123"));
    };
    auto echo_command_func = [](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const CommandRequest &cmd = dynamic_cast<const CommandRequest&>(message);
        dispatcher(CommandResponse(cmd.sequence(), {0, ""}, cmd.command()));
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);
    control_worker->register_handler(EV_ROUTER_TELNET_COMMAND_REQ, echo_command_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "admin" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            std::string cmd = "abc 123  def 456  hij 789 klm";

            //测试向左向右移动和删除
            //新的输入
            str << cmd << std::flush;
            //光标向左移动10次，光标移动到h和i之间;
            for(auto i = 0; i < 10; ++i)
                str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::D}) << std::flush;
            //光标向右移动2次，光标移动到j和空格;
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::C}) << std::flush;
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::C}) << std::flush;
            //向前删除两个字符，删除ij
            str << std::string({TelnetConst::DEL}) << std::flush;
            str << std::string({TelnetConst::DEL}) << std::flush;
            //向后删除两个字符，删除空格，光标在h和8之间
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::J, '~'}) << std::flush;
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::J, '~'}) << std::flush;
            //输入字符串000
            str << "000" << std::flush;
            //执行命令
            str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            std::string cmd_response_1 = "abc 123  def 456  h00089 klm";
            EXPECT_EQ(cmd_response_1, Poco::trimRight(line));

            //测试向左向右移动单词和删除
            //新的输入
            str << cmd << std::flush;
            //光标向左移动3个单词，并输入%
            str << std::string({TelnetConst::CB}) << std::flush;
            str << std::string({TelnetConst::CB}) << std::flush;
            str << std::string({TelnetConst::CB}) << std::flush;
            str << "%" << std::flush;
            //光标向右移动2个单词，并输入%
            str << std::string({TelnetConst::CN}) << std::flush;
            str << std::string({TelnetConst::CN}) << std::flush;
            str << "%" << std::flush;
            //光标向左移动1个字符
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::D}) << std::flush;
            //向前删除1个单词
            str << std::string({TelnetConst::DEL_BW}) << std::flush;
            //执行命令
            str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            std::string cmd_response_2 = "abc 123  def 456  %hij %klm";
            EXPECT_EQ(cmd_response_2, Poco::trimRight(line));

            //测试光标前的所有字符，或者删除后的所有字符
            //新的输入
            str << cmd << std::flush;
            //光标向左移动3个单词
            str << std::string({TelnetConst::CB}) << std::flush;
            str << std::string({TelnetConst::CB}) << std::flush;
            str << std::string({TelnetConst::CB}) << std::flush;
            //删除光标后的所有字符
            str << std::string({TelnetConst::DEL_AC}) << std::flush;
            //光标向左移动1个单词
            str << std::string({TelnetConst::CB}) << std::flush;
            //删除光标前的所有字符
            str << std::string({TelnetConst::DEL_BC}) << std::flush;
            //执行命令
            str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            std::string cmd_response_3 = "456";
            EXPECT_EQ(cmd_response_3, Poco::trimRight(line));

            //测试HOME和END
            //新的输入
            str << cmd << std::flush;
            //光标移动到输入的最前面，并输入一个^
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::H, '~'}) << std::flush;
            str << "^" << std::flush;
            //光标移动到输入的最后面，并输入一个$
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::I, '~'}) << std::flush;
            str << "$" << std::flush;
            //执行命令
            str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            std::string cmd_response_4 = "^abc 123  def 456  hij 789 klm$";
            EXPECT_EQ(cmd_response_4, Poco::trimRight(line));


            //测试历史命令
            //新的输入
            str << cmd << std::flush;
            //向上翻动3次
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::A}) << std::flush;
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::A}) << std::flush;
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::A}) << std::flush;
            //执行命令
            str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            EXPECT_EQ(cmd_response_2, Poco::trimRight(line));
            //向上翻动3次
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::A}) << std::flush;
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::A}) << std::flush;
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::A}) << std::flush;
            //向下翻动1次
            str << std::string({TelnetConst::ESC, TelnetConst::LSB, TelnetConst::B}) << std::flush;
            //执行命令
            str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            EXPECT_EQ(cmd_response_4, Poco::trimRight(line));

            //退出终端
            str << TelnetConst::EOT << std::flush;
            break;
        }
    }
}

TEST_F(Test_Router_Telnet, IntelliCommand)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    std::string cmd_str = "intelli";
    std::string append_str = "123";
    std::string resp_str = cmd_str + " " + append_str;
    auto login_func = [](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const LoginRequest &login = dynamic_cast<const LoginRequest&>(message);
        dispatcher(LoginResponse(login.sequence(), {0, ""}, "abc123"));
    };
    auto echo_command_func = [&](const MessageInfo &message, MESSAGE_DISPATCH_FUNC dispatcher){
        const CommandRequest &cmd = dynamic_cast<const CommandRequest&>(message);
        const std::string &mode = cmd.mode();
        if (mode == "DM_TAB"){
            EXPECT_EQ(cmd_str, cmd.command());
            dispatcher(CommandResponse(cmd.sequence(), {0, ""}, resp_str));
        }
        else if (mode == "DM_BLANKQUESTION" || mode == "DM_QUESTION"){
            EXPECT_EQ(cmd_str, cmd.command());
            dispatcher(CommandResponse(cmd.sequence(), {0, ""}, resp_str + TelnetConst::CRLF, false));
            dispatcher(CommandResponse(cmd.sequence(), {0, ""}, resp_str + TelnetConst::CRLF, false));
            dispatcher(CommandResponse(cmd.sequence(), {0, ""}, resp_str));
        }
        else{
            dispatcher(CommandResponse(cmd.sequence(), {0, ""}, cmd.command()));
        }
    };
    control_worker->register_handler(EV_ROUTER_TELNET_LOGIN_REQ, login_func);
    control_worker->register_handler(EV_ROUTER_TELNET_COMMAND_REQ, echo_command_func);

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line)
            str << "admin" << TelnetConst::CRLF << std::flush;
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line)
            str << "admin" << TelnetConst::CRLF << std::flush;
        else if (TelnetConst::TELNET_PROMPT == line){
            //tab键智能提示输入
            str << cmd_str << std::flush;
            str << "\t" << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //等待智能信息回显到客户端上
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            //执行命令
            str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            EXPECT_EQ(Poco::trimInPlace(line), resp_str);


            //问号智能提示
            str << cmd_str << std::flush;
            str << "?" << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //删除客户端显示的当前行
            std::getline(str, line, '\r');
            //问号智能提示输出运行结果，输出三个结果
            for(auto i = 0; i < 3; ++i){
                std::getline(str, line);
                EXPECT_EQ(Poco::trimInPlace(line), resp_str);
            }
            //等待智能信息回显到客户端上
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            //执行命令
            str << " " << append_str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            EXPECT_EQ(Poco::trimInPlace(line), resp_str);


            //空格+问号智能提示
            str << cmd_str << std::flush;
            str << " ?" << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //删除客户端显示的当前行
            std::getline(str, line, '\r');
            //问号智能提示输出运行结果，输出三个结果
            for(auto i = 0; i < 3; ++i){
                std::getline(str, line);
                EXPECT_EQ(Poco::trimInPlace(line), resp_str);
            }
            //等待智能信息回显到客户端上
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            //执行命令
            str << append_str << TelnetConst::CRLF << std::flush;
            //输入回显到客户端
            std::getline(str, line);
            //输出运行结果
            std::getline(str, line);
            EXPECT_EQ(Poco::trimInPlace(line), resp_str);

            break;
            //退出终端
            //str << TelnetConst::EOT << std::flush;
        }
    }
}


TEST_F(Test_Router_Telnet, ChangeTerminalSize)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    char term_size[9];
    term_size[0] = ProtocolNegotiator::IAC;
    term_size[1] = ProtocolNegotiator::SB;
    term_size[2] = ProtocolNegotiator::NAWS;
    term_size[3] = 0x00;
    term_size[4] = 0x7F;
    term_size[5] = 0x00;
    term_size[6] = 0x26;
    term_size[7] = ProtocolNegotiator::IAC;
    term_size[8] = ProtocolNegotiator::SE;

    str.write(term_size, sizeof(term_size));
    str << std::flush;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //退出终端
    str << TelnetConst::EOT << std::flush;
}

TEST_F(Test_Router_Telnet, SetTerminalType)
{
    TCFSAdapterBasePtr term_worker(new TCFSAdapterBase());
    ControlWorkerPtr control_worker(new OMM_Control_Worker());
    RoundWorkBinder binder(term_worker, control_worker);

    std::shared_ptr<TelnetTermServer> termServer(new TelnetTermServer(term_worker, tp));
    termServer->start();

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    char term_type[12];
    term_type[0] = ProtocolNegotiator::IAC;
    term_type[1] = ProtocolNegotiator::SB;
    term_type[2] = ProtocolNegotiator::TTYPE;
    term_type[4] = 0x00;
    term_type[5] = 'x';
    term_type[6] = 't';
    term_type[7] = 'e';
    term_type[8] = 'r';
    term_type[9] = 'm';
    term_type[10]= ProtocolNegotiator::IAC;
    term_type[11]= ProtocolNegotiator::SE;

    str.write(term_type, sizeof(term_type));
    str << std::flush;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //退出终端
    str << TelnetConst::EOT << std::flush;
}

TEST_F(Test_Router_Telnet, Create_OAMRouterService)
{
    ::setenv("simu", "", 0);

    auto access_list = Configuration::load_access_list();
    Poco::StringTokenizer token(access_list, ",", StringTokenizer::TOK_IGNORE_EMPTY|StringTokenizer::TOK_TRIM);

    {
        OAMRouterService router;
        router.init();
        EXPECT_EQ(router.service_count(), token.count());
    }

    {
        ::setenv("router_access", token[token.count() - 1].c_str(), 1);
        OAMRouterService router;
        router.init();
        EXPECT_EQ(router.service_count(), 1);
    }

    ::unsetenv("simu");
    ::unsetenv("router_access");
}

TEST_F(Test_Router_Telnet, Simulator_multi_packets)
{
    ::setenv("simu", "", 0);
    OAMRouterService router;
    router.init();

    StreamSocket ss;
    ss.connect(Poco::Net::SocketAddress(tp.IP, tp.Port));
    SocketStream str(ss);

    std::string mml = "multi";
    StreamTokenizer tokenizer(str);
    tokenizer.addToken(new TelnetInputToken());
    for(auto next = tokenizer.next(); next != nullptr && next->tokenClass() != Token::EOF_TOKEN; next = tokenizer.next()){
        auto line = next->tokenString();
        //用户名
        if (TelnetConst::LOGIN_PROMPT == line){
            str << "zte" << TelnetConst::CRLF << std::flush;
        }
        //密码
        else if (TelnetConst::PASSWORD_PROMPT == line){
            str << "han" << TelnetConst::CRLF << std::flush;
        }
        else if (TelnetConst::TELNET_PROMPT == line){
            str << mml << TelnetConst::CRLF << std::flush;

            //读取输入显式在客户端上
            std::getline(str, line);

            std::string buf;
            while(1){
                char c;
                str.get(c);
                buf.append(1, c);

                if (buf == TelnetConst::MORE){
                    std::this_thread::sleep_for(std::chrono::milliseconds(150));
                    if (c % 2 == 0)
                        str << " " << std::flush;
                    else
                        str << TelnetConst::CRLF << std::flush;

                    buf.clear();
                }
                else if (buf == TelnetConst::TELNET_PROMPT){
                    break;
                }
                else if (c == '\r' || c == '\n'){
                    buf.clear();
                }
            }

            str << TelnetConst::EOT << std::flush;
            break;
        }
    }

    ::unsetenv("simu");
}

NS_VCN_OAM_END
