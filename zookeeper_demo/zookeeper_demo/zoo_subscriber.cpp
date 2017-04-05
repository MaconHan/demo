#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <thread>

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
#include "config.h"

#include <zookeeper.h>
#include <proto.h>

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

static void my_data_completion(int rc, const char *value, int value_len, const struct Stat *stat, const void *data) {
    fprintf(stderr, "%s: rc = %d\n", (char*)data, rc);
    if (value) {
        fprintf(stderr, " value_len = %d\n", value_len);
        fprintf(stderr, " value     = %.*s\n", value_len, value);
    }
    fprintf(stderr, "\nStat:\n");
}

static void my_strings_completion(int rc, const struct String_vector *strings, const void *data)
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
            if (rc == 0)
                fprintf(stderr, "  dir = %s value = %.*s\n", path.c_str(), buffer_len, buffer);

            String_vector strings;
            zoo_get_children(zh, path.c_str(), 1, &strings);
        }
    }
    fprintf(stderr, "\nStat:\n");
}

static void watcher(zhandle_t *zzh, int type, int state, const char *path, void* context)
{
    fprintf(stderr, "Watcher %s state = %s path = %s\n", type2String(type), state2String(state), path);

    if (type == ZOO_CREATED_EVENT){
        
    }
    if (type == ZOO_CHANGED_EVENT){
        char *data = strdup(path);
        int rc = zoo_awget(zzh, path, watcher, "", my_data_completion, data);
    }
    else if (type == ZOO_CHILD_EVENT){
        char *data = strdup(path);
        int rc = zoo_awget_children(zzh, path, watcher, "", my_strings_completion, data);
    }
    else if (type == ZOO_DELETED_EVENT){

    }
}

static void thr_watch()
{
    Stat stat;
    zoo_exists(zh, CLAA_SERVICE_PATH, 1, &stat);
    zoo_exists(zh, CLAA_SERVICE_NS, 1, &stat);
    zoo_exists(zh, CLAA_SERVICE_JS, 1, &stat);

    zoo_awget_children(zh, CLAA_SERVICE_PATH, watcher, "", my_strings_completion, strdup(CLAA_SERVICE_PATH));
    //zoo_awget_children(zh, CLAA_SERVICE_JS, watcher, "", my_strings_completion, strdup(CLAA_SERVICE_JS));

    while(true)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main_sub(int argc, const char* argv[])
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    zh = zookeeper_init(argv[1], watcher, 30 * 1000, &myid, NULL, 0);

    std::thread thr(thr_watch);
    thr.join();
    return 0;
}
