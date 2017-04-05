// libevent_demo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


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

static const char MESSAGE[] = "Hello, World!\n";
static const int PORT = 9995;

static void udp_callback(evutil_socket_t, short, void *);
static void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_readcb(struct bufferevent *bev, void *user_data);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);

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

    listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
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

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 1);
    sockaddr_in udp_addr;
    memset(&udp_addr, 0x00, sizeof(sockaddr_in));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_addr.sin_port = htons(9996);
    bind(udp_fd, (sockaddr*)&udp_addr, sizeof(sockaddr));
    
    event *udp_event = event_new(base, udp_fd, EV_READ|EV_PERSIST, udp_callback, NULL);
    event_add(udp_event, NULL);

    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);

    printf("done\n");
    return 0;
}

static void udp_callback(evutil_socket_t fd, short event, void *user_data)
{
    char data[4096];
    int size = recvfrom(fd, data, 4096, 0, NULL, NULL);
    printf("udp_callback: %.*s\n", size, data);
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *sa, int socklen, void *user_data)
{
    struct event_base *base = (event_base *)user_data;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }

    bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, NULL);
    bufferevent_enable(bev, EV_WRITE|EV_READ|EV_PERSIST);
}

static void conn_writecb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *output = bufferevent_get_output(bev);
    size_t output_size = evbuffer_get_length(output);

    if (output_size == 0) {
        printf("flushed answer\n");
        //bufferevent_free(bev);
    }
    else{
        printf("ready to write %ld data\n", output_size);
    }
}

static void conn_readcb(struct bufferevent *bev, void *user_data)
{
    char data[4096];
    int size = bufferevent_read(bev, data, sizeof(data));
    printf("conn_readcb: %.*s\n", size, data);

    bufferevent_write(bev, data, size);

    evbuffer *output = bufferevent_get_output(bev);
    size = evbuffer_get_length(output);
    printf("write buffer size is %ld\n", size);
}

static void conn_eventcb(struct bufferevent *bev, short events, void *user_data)  
{  
    if (events & BEV_EVENT_EOF) {
        printf("Connection closed.\n");
    } 
    else if (events & BEV_EVENT_ERROR) {
        printf("Got an error on the connection: %s\n", strerror(errno));/*XXX win32*/  
    }

    /* None of the other events can happen here, since we haven't enabled 
     * timeouts */
    bufferevent_free(bev);
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = (event_base *)user_data;
    struct timeval delay = { 2, 0 };

    printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

    event_base_loopexit(base, &delay);
}

