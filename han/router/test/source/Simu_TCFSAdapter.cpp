#include <map>
#include <tuple>
#include <random>
#include <chrono>
#include <thread>
#include <string>
#include <iostream>
#include <functional>

#include "json/json.h"

#include "Simu_TCFSAdapter.h"

NS_VCN_OAM_BEGIN(ROUTER)

struct Telnet_Login{
    void operator()(const LoginRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        if (message.user() == "zte" && message.password() == "han"){
            dispatcher(LoginResponse(message.sequence(), {0, ""}, "abc123"));
        }
        else{
            dispatcher(LoginResponse(message.sequence(), {1, "invalid user"}, ""));
        }
    }
};

struct Telnet_Command{
    void operator()(const CommandRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        const std::string &command = message.command();
        if (command.find("help") == 0){
            std::string response = "";
            response += "nothing    Returns an empty string\r\n";
            response += "single     Returns a single record\r\n";
            response += "danger     Execute interactive commands, e.g. dangerous command\r\n";
            response += "many       Returns many records\r\n";
            response += "multi      Returns multi-packages\r\n";
            response += "intelli    Returns intelligent";
            dispatcher(CommandResponse(message.sequence(), {0, ""}, response));
        }
        else if (command.find("nothing") == 0){
            dispatcher(CommandResponse(message.sequence(), {0, ""}, ""));
        }
        else if (command.find("single") == 0){
            execute_single(message, dispatcher);
        }
        else if (command.find("many") == 0){
            execute_many(message, dispatcher);
        }
        else if (command.find("multi") == 0){
            execute_multi(message, dispatcher);
        }
        else if (command.find("intelli") == 0){
            execute_intelli(message, dispatcher);
        }
        else if (command.find("danger") == 0){
            execute_danger(message, dispatcher);
        }
        else{
            dispatcher(CommandResponse(message.sequence(), {2, "unknown command"}, ""));
        }
    }

    void operator()(const MultiPacketsRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        std::string output;
        MULTI_COUNTER &counter = m_multi_sequence[message.sequence()];
        Poco::UInt16 &n = std::get<1>(counter);
        bool lastpack = n > std::get<0>(counter);
        if (lastpack){
            m_multi_sequence.erase(message.sequence());
            output += "end of multi-packet responses";
        }
        else{
            n += generate_multi_package(n, output);
        }

        dispatcher(MultiPacketsResponse(message.sequence(), {0, ""}, output, lastpack));
    }

    void operator()(const NestCommandResponse &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        if (message.result() == "y"){
            dispatcher(CommandResponse(message.sequence(), {0, ""}, "end of danger response"));
        }
        else {
            dispatcher(CommandResponse(message.sequence(), {3, "command cancelled"}, "end of danger response"));
        }
    }

    void execute_single(const CommandRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        dispatcher(CommandResponse(message.sequence(), {0, ""}, "end of single response"));
    }

    void execute_many(const CommandRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        std::random_device generator;
        std::uniform_int_distribution<int> dis_value(0, 16);
        for(auto i = 0, n = dis_value(generator); i < n; ++i){
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            std::string response = "response many packages: ";
            response += std::to_string(i + 1);
            response += "\r\n";
            dispatcher(CommandResponse(message.sequence(), {0, ""}, response, false));
        }

        dispatcher(CommandResponse(message.sequence(), {0, ""}, "end of many responses", true));
    }

    void execute_multi(const CommandRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        std::string output;
        int n = generate_multi_package(0, output);

        std::random_device generator;
        std::uniform_int_distribution<int> dis_rows(50, 200);
        m_multi_sequence[message.sequence()] = MULTI_COUNTER(dis_rows(generator), n);

        dispatcher(MultiPacketsResponse(message.sequence(), {0, ""}, output, false));
    }

    int generate_multi_package(int n, std::string &output)
    {
        const std::size_t buf_size = 256;
        std::random_device generator;
        std::uniform_int_distribution<int> dis_rows(15, 40);
        std::uniform_int_distribution<int> dis_columns(20, 250);
        std::uniform_int_distribution<int> dis_value(0, 50);

        char str[buf_size];
        auto rows = dis_rows(generator);
        for(auto i = 0; i < rows; ++i){
            auto j = sprintf(str, "%d: ", ++n);
            for(auto n = dis_columns(generator); j < n; ++j)
                str[j] = '0' + dis_value(generator);
            str[j++] = '\n';

            output.append(str, j);
        }

        return rows;
    }

    void execute_intelli(const CommandRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        const std::string &mode = message.mode();
        std::string output = message.command();
        std::random_device generator;
        std::uniform_int_distribution<int> dis_rows(20, 70);
        std::uniform_int_distribution<int> dis_value(0, 15);

        if (mode == "DM_TAB"){
            int errcode= dis_value(generator) > 12 ? 3 : 0;
            std::string errmsg = errcode ? std::string("not exist \"") + output + "\"" : "";
            std::string line   = output + " ";
            for(auto j = 0, n = dis_value(generator); !errcode && j < n; ++j)
                line.append(1, 'A' + dis_value(generator));

            dispatcher(CommandResponse(message.sequence(), {0, ""}, errmsg, false));
            dispatcher(CommandResponse(message.sequence(), {0, ""}, line));
        }
        else if (mode == "DM_BLANKQUESTION" || mode == "DM_QUESTION"){
            auto count = dis_rows(generator);
            if (count % 5 == 0){
                dispatcher(CommandResponse(message.sequence(), {count, std::string("not exist \"") + output + "\""}, ""));
                return;
            }

            for(auto i = 0; i < count; ++i){
                std::string line = output + " ";
                for(auto j = 0, n = dis_value(generator); j < n; ++j)
                    line.append(1, 'A' + dis_value(generator));
                line.append("\r\n");
                dispatcher(CommandResponse(message.sequence(), {0, ""}, line, false));
            }

            dispatcher(CommandResponse(message.sequence(), {0, ""}, ""));
        }
        else{
            dispatcher(CommandResponse(message.sequence(), {3, "undefined intelligent mode"}, ""));
        }
    }

    void execute_danger(const CommandRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        dispatcher(NestCommandRequest(message.sequence(), "Are you sure to delete?(Y/N): ", message.token()));
    }

protected:
    using MULTI_COUNTER = std::tuple<Poco::UInt16, Poco::UInt16>;
    std::map<Poco::UInt32, MULTI_COUNTER> m_multi_sequence;
};

struct RESTful_Request{
    void operator()(const RESTfulRequest &message, MESSAGE_DISPATCH_FUNC dispatcher)
    {
        std::random_device generator;
        std::uniform_int_distribution<int> dis_value(0, 5);
        int errcode = dis_value(generator) ? 200 : 400;

        Json::Value value;
        value["function"]   = "test";
        value["status"]     = "simulator";
        value["request"]    = static_cast<std::string>(message);
        std::string output = Json::FastWriter().write(value);
        dispatcher(RESTfulResponse(message.sequence(), {errcode, ""}, output));
    }
};

Simu_TCFSAdapter::Simu_TCFSAdapter()
{
    this->callback_dispatch_function(*this, &Simu_TCFSAdapter::execute);
}

Simu_TCFSAdapter::~Simu_TCFSAdapter()
{
}

void Simu_TCFSAdapter::execute(const MessageInfo &message)
{
    static Telnet_Login     login;
    static Telnet_Command   command;
    static RESTful_Request  restful;
    static auto dispatcher = [this](const MessageInfo &message){
        std::cout << "simu send: " << static_cast<std::string>(message);
        this->response(message);
    };

    std::cout << "simu recv: " << static_cast<std::string>(message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (message.type() == EV_ROUTER_TELNET_LOGIN_REQ){
        std::thread thr(std::ref(login), dynamic_cast<const LoginRequest&>(message), dispatcher);
        thr.detach();
    }
    else if (message.type() == EV_ROUTER_TELNET_COMMAND_REQ){
        std::thread thr(std::ref(command), dynamic_cast<const CommandRequest&>(message), dispatcher);
        thr.detach();
    }
    else if (message.type() == EV_ROUTER_TELNET_MULTIPACKETS_REQ){
        std::thread thr(std::ref(command), dynamic_cast<const MultiPacketsRequest&>(message), dispatcher);
        thr.detach();
    }
    else if (message.type() == EV_ROUTER_RESTFUL_REQ){
        std::thread thr(std::ref(restful), dynamic_cast<const RESTfulRequest&>(message), dispatcher);
        thr.detach();
    }
    else if (message.type() == EV_ROUTER_TELNET_NEST_ACK){
        std::thread thr(std::ref(command), dynamic_cast<const NestCommandResponse&>(message), dispatcher);
        thr.detach();
    }
}

NS_VCN_OAM_END
