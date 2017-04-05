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

static void watcher(zhandle_t *zzh, int type, int state, const char *path, void* context)
{
}

void thr_func()
{
    char path_buffer[64];
    int path_buffer_len = 64;
    auto AS_Create = [&](){
        int rc = zoo_create(zh, CLAA_SERVICE_AS, "", 0, &ZOO_OPEN_ACL_UNSAFE, 0, path_buffer, path_buffer_len);
        std::cout << "Create Path: " << path_buffer << std::endl;
    };

    auto AS_AppendChild = [&](int size){
        for (auto i = 0; i < size; ++i){
            std::string path = std::string(CLAA_SERVICE_AS);
            path += "/";
            path += 'a' + i;

            std::string value(i + 1, 'a' + i);
            int rc = zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, path_buffer, path_buffer_len);
            std::cout << "Create Path: " << path_buffer << std::endl;
        }
    };

    auto AS_Set = [](){
        String_vector strings;
        int rc = zoo_get_children(zh, CLAA_SERVICE_AS, 0, &strings);
        for(auto i = 0; i < strings.count; ++i){
            std::string path = CLAA_SERVICE_AS;
            path += '/';
            path += strings.data[i];

            std::string value(i + 1, 'A' + i);
            zoo_set(zh, path.c_str(), value.c_str(), value.size(), -1);
            std::cout << "Set Value: " << path.c_str() << " " << value.c_str()  << std::endl;
        }
    };

    auto AS_Delete = [](){
        String_vector strings;
        int rc = zoo_get_children(zh, CLAA_SERVICE_AS, 0, &strings);
        for(auto i = 0; i < strings.count; ++i){
            std::string path = CLAA_SERVICE_AS;
            path += '/';
            path += strings.data[i];

            zoo_delete(zh, path.c_str(), -1);
            std::cout << "Delete Path: " << path.c_str() << std::endl;
        }

        zoo_delete(zh, CLAA_SERVICE_AS, -1);
        std::cout << "Delete Path: " << CLAA_SERVICE_AS << std::endl;
    };

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(3));
        AS_Create();

        std::this_thread::sleep_for(std::chrono::seconds(1));
        AS_AppendChild(rand() % 8);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        AS_Set();

        std::this_thread::sleep_for(std::chrono::seconds(2));
        AS_Delete();
    }
}


void main(int argc, const char* argv[])
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    zh = zookeeper_init(argv[1], watcher, 30 * 1000, &myid, NULL, 0);

    std::thread thr(thr_func);
    thr.join();
}
