// libevent_demo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include <thread>

static const char MESSAGE[] = "Hello, World!\n";
static const int PORT = 9995;

static const char *NS_ADDR = "10.47.73.161:9000";
static void signal_cb(evutil_socket_t, short, void *);

class GWIF{
public:
    GWIF(event_base *base) : _base(base), _timeout(NULL), _tcp_fd(-1), _tcp_ev(NULL), _udp_fd(-1), _udp_ev(NULL)
    {
        memset(&_ns_addr, 0x00, sizeof(_ns_addr));
        const char *port = strchr(NS_ADDR, ':');
        _ns_addr.sin_family             = AF_INET;
        _ns_addr.sin_addr.S_un.S_addr   = inet_addr(std::string(NS_ADDR, port - NS_ADDR).c_str());
        _ns_addr.sin_port               = htons(atoi(port + 1));
    }

    virtual ~GWIF()
    {
        if (_udp_ev)
            event_free(_udp_ev);
        if (_tcp_ev)
            bufferevent_free(_tcp_ev);
        if (_timeout)
            event_free(_timeout);

        if (_udp_fd != -1)
            closesocket(_udp_fd);
        if (_tcp_fd != -1)
            closesocket(_tcp_fd);
    }

public:
    static void listener(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
    {
        struct event_base *base = (event_base *)user_data;
        
        GWIF *gw = new GWIF(base);
        gw->create_tcp(fd);
        gw->create_udp(0);
        gw->create_timeout(1000);
    }

    static void conn_writecb(struct bufferevent *bev, void *user_data)
    {
        GWIF *gw = (GWIF*)user_data;

        struct evbuffer *output = bufferevent_get_output(bev);
        size_t output_size = evbuffer_get_length(output);

        if (output_size == 0) {
            printf("flushed answer\n");
        }
        else{
            printf("ready to write %ld data\n", output_size);
        }
    }

    static void conn_readcb(struct bufferevent *bev, void *user_data)
    {
        GWIF *gw = (GWIF*)user_data;

        char data[16 * 1024];
        int size = 0;
        while(1){
            int n = bufferevent_read(bev, data + size, sizeof(data) - 1 - size);
            if (n <= 0)
                break;

            size += n;
            data[size] = '\0';
            char *p1 = data, *p2, *end = data + size;
            while(p1 < end){
                p2 = strchr(p1, '\n');
                if (p2 == NULL)
                    break;

                int len = p2 - p1 + 1;
                sendto(gw->_udp_fd, p1, len, 0, (sockaddr*)&gw->_ns_addr, sizeof(gw->_ns_addr));
                printf("tcp read: %.*s\n", size, data);
                p1 += len;
            }

            size = size - (p1 - data);
            memmove(data, p1, size);
        }
    }

    static void conn_eventcb(struct bufferevent *bev, short events, void *user_data)  
    {
        GWIF *gw = (GWIF*)user_data;

        if (events & BEV_EVENT_EOF) {
            printf("Connection closed.\n");
        } 
        else if (events & BEV_EVENT_ERROR) {
            printf("Got an error on the connection: %s\n", strerror(errno));/*XXX win32*/  
        }

        delete gw;
    }

    static void udp_callback(evutil_socket_t fd, short event, void *user_data)
    {
        GWIF *gw = (GWIF*)user_data;
        assert(gw->_udp_fd == fd);

        if (event & EV_READ){
            char data[64 * 1024];
            sockaddr_in from;
            int fromlen = sizeof(sockaddr_in);
            int size = recvfrom(gw->_udp_fd, data, sizeof(data), 0, (sockaddr*)&from, &fromlen);
            if (size == -1)
                return;
            bufferevent_write(gw->_tcp_ev, data, size);

            char *addr  = inet_ntoa(from.sin_addr);
            int port    = ntohs(from.sin_port);
            printf("udp read(%s:%hu) - %.*s\n", addr, port, size, data);
        }
        else if (event & EV_CLOSED){
            printf("udp closed\n");
        }
    }

    static void timeout(evutil_socket_t fd, short event, void *user_data)
    {
        GWIF *gw = (GWIF*)user_data;

        sockaddr_in addr;
        socklen_t addr_len = sizeof(sockaddr_in);
        getpeername(gw->_tcp_fd, (struct sockaddr *)&addr, &addr_len);
        printf("tcp timeout - %s:%hu\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    }

private:
    void create_udp(short port = 0)
    {
        _udp_fd = socket(AF_INET, SOCK_DGRAM);
        sockaddr_in udp_addr;
        memset(&udp_addr, 0x00, sizeof(sockaddr_in));
        udp_addr.sin_family     = AF_INET;
        udp_addr.sin_addr.s_addr= htonl(INADDR_ANY);
        udp_addr.sin_port       = htons(port);
        bind(_udp_fd, (sockaddr*)&udp_addr, sizeof(sockaddr));
    
        _udp_ev = event_new(_base, _udp_fd, EV_CLOSED|EV_READ|EV_PERSIST|EV_ET, GWIF::udp_callback, (void*)this);
        event_del_block(_udp_ev);
        event_add(_udp_ev, NULL);
    }

    void create_tcp(intptr_t fd)
    {
        _tcp_fd = fd;
        _tcp_ev = bufferevent_socket_new(_base, _tcp_fd, BEV_OPT_CLOSE_ON_FREE);

        bufferevent_setcb(_tcp_ev, GWIF::conn_readcb, GWIF::conn_writecb, GWIF::conn_eventcb, this);
        bufferevent_enable(_tcp_ev, EV_READ|EV_PERSIST|EV_ET);
    }

    void create_timeout(int milliseconds = 10000)
    {
        if (milliseconds == -1)
            return;

        timeval tv = {int(milliseconds / 1000), int(milliseconds % 1000) * 1000};
        _timeout = event_new(_base, -1, EV_PERSIST|EV_TIMEOUT, GWIF::timeout, this);
        event_add(_timeout, &tv);
    }

private:
    event_base  *_base;
    event       *_timeout;

    intptr_t    _tcp_fd;
    bufferevent *_tcp_ev;

    intptr_t    _udp_fd;
    event       *_udp_ev;
    sockaddr_in _ns_addr;
};

int main(int argc, char **argv)
{
    struct event_base *base;
    struct evconnlistener *listener;
    struct event *signal_event;

    struct sockaddr_in sin;
#ifdef WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family  = AF_INET;
    sin.sin_port    = htons(PORT);

    listener = evconnlistener_new_bind(base, GWIF::listener, (void *)base,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin,
        sizeof(sin));
    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        return 1;
    }

    signal_event = evsignal_new(base, SIGINT|SIGTERM, signal_cb, (void *)base);
    if (!signal_event || event_add(signal_event, NULL)<0) {
        fprintf(stderr, "Could not create/add a signal event!\n");
        return 1;
    }

    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);

    printf("done\n");
    return 0;
}



static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = (event_base *)user_data;
    struct timeval delay = { 2, 0 };

    printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

    event_base_loopexit(base, &delay);
}

