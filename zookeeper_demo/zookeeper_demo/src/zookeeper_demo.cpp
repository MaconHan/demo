// zookeeper_demo.cpp : 定义控制台应用程序的入口点。
//
#include <iostream>
#include <zookeeper.h>
#include <proto.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#else
#include "winport.h"
//#include <io.h> <-- can't include, conflicting definitions of close()
int read(int _FileHandle, void * _DstBuf, unsigned int _MaxCharCount);
int write(int _Filehandle, const void * _Buf, unsigned int _MaxCharCount);
#define ctime_r(tctime, buffer) ctime_s (buffer, 40, tctime)
#endif

#include <time.h>
#include <errno.h>
#include <assert.h>


static zhandle_t *zh;
static clientid_t myid;


static const char* state2String(int state){
  if (state == 0)
    return "CLOSED_STATE";
  if (state == ZOO_CONNECTING_STATE)
    return "CONNECTING_STATE";
  if (state == ZOO_ASSOCIATING_STATE)
    return "ASSOCIATING_STATE";
  if (state == ZOO_CONNECTED_STATE)
    return "CONNECTED_STATE";
  if (state == ZOO_EXPIRED_SESSION_STATE)
    return "EXPIRED_SESSION_STATE";
  if (state == ZOO_AUTH_FAILED_STATE)
    return "AUTH_FAILED_STATE";

  return "INVALID_STATE";
}

static const char* type2String(int state){
  if (state == ZOO_CREATED_EVENT)
    return "CREATED_EVENT";
  if (state == ZOO_DELETED_EVENT)
    return "DELETED_EVENT";
  if (state == ZOO_CHANGED_EVENT)
    return "CHANGED_EVENT";
  if (state == ZOO_CHILD_EVENT)
    return "CHILD_EVENT";
  if (state == ZOO_SESSION_EVENT)
    return "SESSION_EVENT";
  if (state == ZOO_NOTWATCHING_EVENT)
    return "NOTWATCHING_EVENT";

  return "UNKNOWN_EVENT_TYPE";
}

void my_data_completion(int rc, const char *value, int value_len, const struct Stat *stat, const void *data) {
    fprintf(stderr, "%s: rc = %d\n", (char*)data, rc);
    if (value) {
        fprintf(stderr, " value_len = %d\n", value_len);
        fprintf(stderr, " value     = %.*s\n", value_len, value);
    }
    fprintf(stderr, "\nStat:\n");
}

void my_strings_completion(int rc, const struct String_vector *strings, const void *data)
{
    fprintf(stderr, "%s: rc = %d\n", (char*)data, rc);
    std::string parent_dir = (const char*)data;
    free((void*)data);

    if (strings) {
        int buffer_len = 512;
        char *buffer = (char*)alloca(buffer_len);
        
        for(auto i = 0; i < strings->count; ++i, buffer_len = 512){
            std::string path = parent_dir;
            path += "/";
            path += strings->data[i];
            int rc = zoo_get(zh, path.c_str(), 1, buffer, &buffer_len, NULL);
            fprintf(stderr, " dir = %s value = %.*s\n", path.c_str(), buffer_len, buffer);
        }
    }
    fprintf(stderr, "\nStat:\n");
}

void watcher(zhandle_t *zzh, int type, int state, const char *path, void* context)
{
    fprintf(stderr, "Watcher %s state = %s path = %s\n", type2String(type), state2String(state), path);

    if (type == ZOO_CHANGED_EVENT){
        char *data = strdup(path);
        int rc = zoo_awget(zzh, path, watcher, "", my_data_completion, data);
    }
    else if (type == ZOO_CHILD_EVENT){
        char *data = strdup(path);
        int rc = zoo_awget_children(zzh, path, watcher, "", my_strings_completion, data);
    }
}

int main1(int argc, char* argv[])
{
    std::cout << argv[0] << std::endl;

    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
    zh = zookeeper_init(argv[1], watcher, 30 * 1000, &myid, 0, 0);

    Stat stat;
    char *watcher_ctx   = "claa_watcher";
    int rc = zoo_awget(zh, "/claa", watcher, watcher_ctx, my_data_completion, strdup("/claa"));
    rc = zoo_awget_children(zh, "/claa", watcher, watcher_ctx, my_strings_completion, strdup("/claa"));

    auto stat_completion = [](int rc, const struct Stat *stat,const void *data) ->void{};
    rc = zoo_awexists(zh, "/claa/list", watcher, watcher_ctx, stat_completion, strdup("/claa"));

    char tmp[512];
    std::cin >> tmp;
    zookeeper_close(zh);

    return 0;
}

