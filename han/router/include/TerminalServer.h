/**@file
* 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
* 文件名称：<br>
* 文件标识：
* 内容摘要：SSS 2.0 OMM系统终端服务头文件<br>
* 其它说明：无<br>
* @version 1.0<br>
* @author <br>
* @since 2009-06-08<br>
*/
#ifndef __TERMINALSERVER_H_
#define __TERMINALSERVER_H_

#include <thread>
#include <chrono>

#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/SocketConnector.h"
#include "Poco/Net/SocketAcceptor.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Thread.h"
#include "Poco/Runnable.h"
#include "Poco/String.h"
#include "Poco/Types.h"
#include "Poco/Format.h"

#include "syscomm.h"
#include "Configuration.h"

#include "TerminalMessage.h"
#include "TerminalInfo.h"
#include "TCFSAdapter.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;

NS_VCN_OAM_BEGIN(ROUTER)

/**
* @brief Acceptor-reactor 单线程非阻塞多通道网络模式
*/
template<typename TermHandler, typename ServiceHandler>
class TermSocketAcceptor: public SocketAcceptor<ServiceHandler>
{
public:
    TermSocketAcceptor(ServerSocket &socket, TermHandler *pTermServer):
        SocketAcceptor<ServiceHandler>::SocketAcceptor(socket),
        m_pTermServer(pTermServer)
    {
    }

    TermSocketAcceptor(ServerSocket &socket, SocketReactor &reactor, TermHandler *pTermServer):
        SocketAcceptor<ServiceHandler>::SocketAcceptor(socket, reactor),
        m_pTermServer(pTermServer)
    {
    }

    ~TermSocketAcceptor()
    {
    }

protected:
    virtual ServiceHandler* createServiceHandler(StreamSocket& socket)
    {
        return new ServiceHandler(socket, *this->reactor(), m_pTermServer);
    }

private:
    TermHandler *m_pTermServer = nullptr;
};


template <typename ServiceHandler>
class TermServer : public Poco::Runnable
{
    using TermConnectionPtr = ServiceHandler*;
    using _self_t = TermServer<ServiceHandler>;

public:
    TermServer(TCFSAdapterBasePtr work_adapter, const TelnetProperty &telnet_prop):
        m_work_adapter(work_adapter),
        m_prop(telnet_prop)
    {
        CREATE_LOG();

        m_work_adapter->callback_response_function(*this, &_self_t::response);
    }

    ~TermServer()
    {
        stop();
    }

    /**
    * @brief 服务是否运行
    */
    bool  isRunning() const
    {
        return m_running;
    }

    /**
    * @brief 启动服务
    */
    virtual void run()
    {
        auto func = [this](){
            std::size_t seconds = 2;
            while(!this->m_timeout_sync.tryWait(seconds * 1000)){
                Poco::ScopedLock<Poco::Mutex> lock(this->m_mutex);
                for(auto &it : m_mpConnections){
                    it.second->check_timeout(seconds);
                }
            }
        };
        m_timeout_thr = std::thread(func);

        logDebug("start a TelnetServer:\n%s", m_prop.toString().c_str());
        try{
            m_svr_socket.bind(Poco::Net::SocketAddress(m_prop.IP, m_prop.Port), true);
            m_svr_socket.setBlocking(false);
            m_svr_socket.listen();

            m_reactor.setTimeout(Poco::Timespan(0, 30 * 1000));
            m_acceeptor = std::shared_ptr<TermAcceptor>(new TermAcceptor(m_svr_socket, m_reactor, this));

            m_running = true;
            m_reactor.run();
        }
        catch(Poco::Exception &e){
            logError("failed to start telnet server, address: %s(%hu), %s",
                     m_prop.IP.c_str(),
                     m_prop.Port,
                     e.displayText().c_str());
        }
        catch (...){
            logError("failed to start telnet server, address: %s(%hu).", m_prop.IP.c_str(), m_prop.Port);
        }
    }

    void start()
    {
        if (m_tThread.isRunning())
            return;

        m_tThread.start(*this);
        while(!m_running)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    void stop()
    {
        if (!m_running)
            return;
        m_running = false;

        m_timeout_sync.set();
        m_timeout_thr.join();

        m_reactor.stop();
        m_tThread.join();

        for (auto &it : m_mpConnections){
            it.second->disconnect();
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        m_mpConnections.clear();
    }

    int dispatch(const MessageInfo &message)
    {
        m_work_adapter->dispatch(message);
    }

    void response(const MessageInfo &message)
    {
        auto sequence = message.sequence();

        Poco::ScopedLock<Poco::Mutex> lock(m_mutex);
        //有效的顺序号，根据顺序号查找终端
        if (INVALID_SEQUENCE != sequence){
            Poco::UInt32 term_id = TerminalInfo::convert2terminal(sequence);
            auto it = m_mpConnections.find(term_id);
            if (it != m_mpConnections.end())
                it->second->send2terminal(message);
        }
        //无效的顺序号，广播给所有的终端
        else{
            for (auto &it : m_mpConnections)
                it.second->send2terminal(message);
        }
    }

    /**
    * @brief 链路注册
    */
    void registryConn(TermConnectionPtr term_conn)
    {
        Poco::ScopedLock<Poco::Mutex> lock(m_mutex);
        //连接数目检查
        if (m_prop.Max_Client != 0 && m_links == m_prop.Max_Client)
            throw Poco::Exception(Poco::format("Exceeded maximum number(%hu) of online", m_prop.Max_Client));

        const TerminalInfo &term = term_conn->getTermInfo();
        m_mpConnections.insert(make_pair(term.key(), term_conn));

        ++m_links;
        logDebug("registry a TelnetServer, and links is %lu", m_links);
    }

    /**
    * @brief 链路注销
    */
    void unregistryConn(TermConnectionPtr term_conn)
    {
        Poco::ScopedLock<Poco::Mutex> lock(m_mutex);
        if (!m_running)
            return;

        const TerminalInfo &term = term_conn->getTermInfo();
        auto it = m_mpConnections.find(term.key());
        if (it == m_mpConnections.end() || it->second != term_conn)
            return;
        m_mpConnections.erase(it);

        --m_links;
        logDebug("unregistry a TelnetServer, and links is %lu", m_links);
    }

    const TelnetProperty& prop() const
    {
        return m_prop;
    }

protected:
    std::size_t m_links = 0;
    bool m_running      = false;

    TCFSAdapterBasePtr      m_work_adapter;
    TelnetProperty          m_prop;

    ServerSocket            m_svr_socket;
    SocketReactor           m_reactor;

    typedef TermSocketAcceptor<TermServer<ServiceHandler>, ServiceHandler> TermAcceptor;
    std::shared_ptr<TermAcceptor> m_acceeptor;

    /* 容器多线程操作同步互斥变量 */
    Poco::Mutex     m_mutex;
    map<Poco::UInt32, TermConnectionPtr> m_mpConnections;

    Poco::Thread        m_tThread;
    Poco::Event         m_timeout_sync;
    std::thread         m_timeout_thr;

private:
    DECLARE_LOG();
};

NS_VCN_OAM_END

#endif
