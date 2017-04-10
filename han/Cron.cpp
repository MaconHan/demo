/**
@file
* 版权所有(C)2001, 深圳市中兴通讯股份有限公司<br/>
* 文件名称：Cron.h<br/>
* 内容摘要：计划任务<br/>
* 其它说明：无<br/>
@version 1.0
@author han.yamin
@since  2013-5
*/

#include <ctype.h>

#ifdef POCO_OS_LINUX
#include <pthread.h>
#endif

#include "Poco/StringTokenizer.h"
#include "Poco/LocalDateTime.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/String.h"

#include "Cron.h"

namespace zteomm
{
namespace pub
{

#define SET(v, n)       (v = (((Poco::UInt64)1 << n) | v))
#define UNSET(v, n)     (v = (~((Poco::UInt64)1 << n) & v))
#define CHECK(v, n)     (v & ((Poco::UInt64)1 << n))

/**min, max, count, module, name*/
/**最小值，最大值，总数，模值, name*/
static const int SCHE_VALUE[5][5] = {
{0, 59, 60, 60, 'M'}, 
{0, 23, 24, 24, 'h'}, 
{1, 31, 31, 32, 'd'}, 
{1, 12, 12, 13, 'm'}, 
{0, 7,  7,  7,  'w'}
};

#define  SCHE_MASK_ALL   ~(Poco::UInt64)0

CREATE_STATIC_NAME_LOG(Cron, "Cron");
zteomm::pub::Log &log = Cron::log;
Poco::SingletonHolder<Cron>  Cron::CronSingleton;

/**
@brief 线程类封装执行目标，本对象自己销毁
*/
template<typename C, typename P>
class ThreadTarget : public Poco::Runnable
{
public:
    typedef void (C::*Callback)(P sche);

protected:
    ThreadTarget(C &p, Callback method, P sche) : _p(&p), _method(method), _sche(sche), _status(true)
    {}

    ~ThreadTarget()
    {}

    ThreadTarget(const ThreadTarget& te);
    ThreadTarget& operator = (const ThreadTarget& te);

public:
    static void startThread(C &p, Callback method, P sche)
    {
        ThreadTarget<C, P> *thr_target = new(std::nothrow) ThreadTarget<C, P>(p, method, sche);
        Poco::Thread *thr = new(std::nothrow) Poco::Thread;
        if (thr_target == NULL || thr == NULL){
            SAFE_DELETE(thr);
            SAFE_DELETE(thr_target);
            
            throw Poco::Exception("failed to new ThreadTarget<Cron>", 0);
        }

        Poco::Exception exc("", 0);
        try{
            thr->start(*thr_target);
        }
        catch(Poco::Exception &e){
            exc = e;
        }
        catch(std::exception &e){
            exc = Poco::Exception(e.what(), errno);
        }
        catch(...){
            exc = Poco::Exception("cannot start thread, unknown reason", errno);
        }

        if (thr->isRunning()){
            thr_target->_status.set();
        }
        else{
            SAFE_DELETE(thr);
            SAFE_DELETE(thr_target);

            throw exc;
        }
    }

protected:
    void run()
    {
        logDebug("start the thread\n");

        //执行处理函数
        try{
            (_p->*_method)(_sche);
        }
        catch(...){
        }
        
        //等待确认startThread调用结束
        try{
            if (!_status.tryWait(10 * 1000)){
                logError("wait to the event of exit timeout\n");
                return;
            }
        }
        catch(...){
            logError("failed to wait to the event of exit\n");
            return;
        }
        
        //销毁自己
        delete this;
        
        //销毁线程对象
        Poco::Thread *thr = NULL;
        try{
            thr = Poco::Thread::current();
            SAFE_DELETE(thr);
        }
        catch(...){
        }

        logDebug("destory the thread\n");
    }

private:
    /**@brief 执行模板类对象*/
    C           *_p;
    /**@brief 任务定义的对象*/
    P           _sche;
    /**@brief 执行模板类的回调函数*/
    Callback    _method;
    /**@brief 回调函数线程状态事件，用于控制销毁自身对象和线程对象*/
    Poco::Event _status;
};

Cron::Cron() : _global_id(0), _stop_event(new Poco::Event(false))
{
    _run_list = new Poco::Tuple<std::list<SCHE>, Poco::FastMutex>();
    _stop_event->reset();

    _check_thread.setPriority(Poco::Thread::PRIO_HIGH);
    _check_thread.start(*this);
}

Cron::~Cron()
{
    Poco::RWLock::ScopedLock wlock(_sche_rwlock, true);
    _stop_event->set();

    /**等待检测线程结束*/
    while(_check_thread.isRunning()){
        Poco::Thread::sleep(10);
    }
}

cron_id_t Cron::parse_part(Poco::UInt8 index, const std::string &pattern, Poco::UInt64 &set)
{
    set = 0;
    Poco::UInt64 set_ = 0;
    bool every_ = false;
    int ri, rj;
    const char *c = pattern.c_str(), *p = c;
    const char *e = c + pattern.size();
    while(c < e){
        /**通配所有范围*/
        if (*c == '*'){
            set_ = SCHE_MASK_ALL;

            ri = SCHE_VALUE[index][0];
            rj = SCHE_VALUE[index][1];
            every_ = true;
        }
        /**取数字*/
        else if (isdigit(*c)){
            int n = atoi(p);
            if (SCHE_VALUE[index][0] > n || SCHE_VALUE[index][1] < n)
                return -1;
            SET(set_, n);

            /**下移到一个非数字的位置*/
            for(; c < e && isdigit(*c); ++c);
            continue;
        }
        /**离散数字*/
        else if (*c == ','){
            every_ = false;
            p = c + 1;
        }
        /**范围*/
        else if (*c == '-'){
            every_ = true;
            ri = atoi(p);
            rj = atoi((p = ++c));
            if (!isdigit(*c))
                return -1;
            if (SCHE_VALUE[index][0] > ri || SCHE_VALUE[index][1] < ri)
                return -1;
            if (SCHE_VALUE[index][0] > rj || SCHE_VALUE[index][1] < rj)
                return -1;
            
            if (rj < ri)    rj += SCHE_VALUE[index][2];
            for(int j = ri; j <= rj; ++j){
                int i = j;
                if (i > SCHE_VALUE[index][1])
                    i = i % SCHE_VALUE[index][3] + SCHE_VALUE[index][0];
                i %= SCHE_VALUE[index][3];
                SET(set_, i);
            }

            /**下移到一个非数字的位置*/
            for(; c < e && isdigit(*c); ++c);
            continue;
        }
        /**每*/
        else if (*c == '/'){
            if (!every_)
                return -1;
            p = ++c;

            int interval = atoi(p);
            if (interval <= 0)
                return -1;
            for (int j = ri; j <= rj; ++j){
                if ((j - ri) % interval == 0)
                    continue;

                int i = j;
                if (i > SCHE_VALUE[index][1])
                    i = i % SCHE_VALUE[index][3] + SCHE_VALUE[index][0];
                i %= SCHE_VALUE[index][3];
                UNSET(set_, i);
            }

            /**下移到一个非数字的位置*/
            for(; c < e && isdigit(*c); ++c);
            continue;
        }
        else{
            return -1;
        }

         ++c;
    }

    set = set_;
    return 0;
}

Poco::Int32 Cron::parse(date_rec &rec)
{
    Poco::UInt32 ret = 0;

    /**去掉重复分割符*/
    Poco::replaceInPlace(rec.str, "\t", " ");
    for(size_t i = rec.str.size(), j = 0; i != j; ){
        Poco::replaceInPlace(rec.str, "  ", " ");
        j = i;
        i = rec.str.size();
    }

    Poco::StringTokenizer token(rec.str, " ");
    if (token.count() != 5)
        return -1;
    for(Poco::UInt8 i = 0; i < token.count(); ++i){
        Poco::UInt64 &set = rec.sche.parts[i];
        if (parse_part(i, token[i], set) != 0)
            return -1;
    }

    if (rec.sche.part.days == SCHE_MASK_ALL && rec.sche.part.weeks != SCHE_MASK_ALL){
        rec.sche.part.days = 0;
    }
    else if (rec.sche.part.days != SCHE_MASK_ALL && rec.sche.part.weeks == SCHE_MASK_ALL){
        rec.sche.part.weeks = 0;
    }

    return ret;
}

cron_id_t Cron::Register(const std::string &crontab, CronCaller &callback, Poco::UInt32 flags)
{
    date_rec rec;
    rec.sche.part.minutes   = 0;
    rec.sche.part.hours     = 0;
    rec.sche.part.days      = 0;
    rec.sche.part.months    = 0;
    rec.sche.part.weeks     = 0;
    rec.str                 = crontab;

    Poco::UInt32 ret = parse(rec);
    if (ret)
        return INVALID_CRON_ID;
    
    Poco::UInt16 id_;
    {
        Poco::FastMutex::ScopedLock lock(_id_mutex);
        {
            Poco::RWLock::ScopedLock rlock(_sche_rwlock);
            id_ = ++_global_id;
            int i = _global_id;
            do 
            {
                if (_sche_map.find(id_) == _sche_map.end())
                    break;
                id_ = ++_global_id;
            } while (i != id_);
        }
        
        Poco::RWLock::ScopedLock wlock(_sche_rwlock, true);
        if (_sche_map.find(id_) != _sche_map.end())
            return INVALID_CRON_ID;

        SCHE sche = new _SCHE();
        _sche_map[id_] = sche;
        sche->set<SCHE_ID>(id_);
        sche->get<SCHE_DATE>().push_back(rec);
        sche->set<SCHE_CALLER>(callback);
        sche->get<SCHE_COUNTER>()(0);
        sche->set<SCHE_FLAG>(flags);
        sche->set<SCHE_THREAD>(0);
    }
    
    return id_;
}

cron_id_t Cron::Register(cron_id_t id_, const std::string &crontab)
{
    date_rec rec;
    rec.sche.part.minutes   = 0;
    rec.sche.part.hours     = 0;
    rec.sche.part.days      = 0;
    rec.sche.part.months    = 0;
    rec.sche.part.weeks     = 0;
    rec.str                 = crontab;

    Poco::UInt32 ret = parse(rec);
    if (ret)
        return INVALID_CRON_ID;

    Poco::RWLock::ScopedLock wlock(_sche_rwlock, true);
    if (_sche_map.find(id_) == _sche_map.end())
        return INVALID_CRON_ID;
    
    SCHE sche = _sche_map[id_];
    sche->get<SCHE_DATE>().push_back(rec);
    return id_;
}

void Cron::Unregister(cron_id_t id_)
{
    SCHE sche;
    
    {
        Poco::RWLock::ScopedLock wlock(_sche_rwlock);
        if (_sche_map.find(id_) == _sche_map.end())
            return;

        sche = _sche_map[id_];
        Counter &counter = sche->get<SCHE_COUNTER>();
        Poco::UInt32 &flags = sche->get<SCHE_FLAG>();
        flags |= CO_INVALID;

        {
            std::list<SCHE> &list_ = _run_list->get<0>();
            Poco::FastMutex &mutex_= _run_list->get<1>();
            Poco::FastMutex::ScopedLock lock(mutex_);
            std::list<SCHE>::iterator it = list_.begin();
            while(it != list_.end()){
                if ((*it)->get<SCHE_ID>() != id_){
                    ++it;
                    continue;
                }

                --counter;
                it = list_.erase(it);
            }
        }
    }
    
    Counter &counter = sche->get<SCHE_COUNTER>();
    thread_t &thr    = sche->get<SCHE_THREAD>();

#ifdef POCO_OS_FAMILY_UNIX
    if (pthread_equal(thr, pthread_self()) == 0)
        counter.wait();
#elif defined(POCO_OS_FAMILY_WINDOWS)
    if (::GetCurrentThreadId() != thr)
        counter.wait();
#endif

    Poco::RWLock::ScopedLock wlock(_sche_rwlock, true);
    _sche_map.erase(id_);
}

std::string Cron::toStr(cron_id_t id_)
{
    SCHE sche;
    {
        Poco::RWLock::ScopedLock rlock(_sche_rwlock);
        if (_sche_map.find(id_) == _sche_map.end())
            return "";
        sche = _sche_map[id_];
    }

    std::string formatstr(1024, 0x00);
    formatstr = "";

    char buf[256];
    const int o = 5;
    memset(buf, ' ', sizeof(buf));
    for (int i = 0; i <= 60; i += 5){
        int j = sprintf(buf + o + i * 2, "%d", i);
        buf[o + i * 2 + j] = ' ';
    }
    buf[o + 60 * 2 + 2] = '\n';
    buf[o + 60 * 2 + 2 + 1] = '\0';
    formatstr += buf;

    std::vector<date_rec> &recs = sche->get<SCHE_DATE>();
    for (int i = 0; i < recs.size(); ++i){
        date_rec &rec = recs[i];
        for(Poco::UInt8 i = 0; i < 5; ++i){
            Poco::UInt64 &set = rec.sche.parts[i];

            memset(buf, ' ', sizeof(buf));
            buf[0] = SCHE_VALUE[i][4];
            buf[o + SCHE_VALUE[i][1] * 2 + 2] = '\0';
            for(int j = 0; j <= SCHE_VALUE[i][1]; ++j){
                if (j < SCHE_VALUE[i][0] || j > SCHE_VALUE[i][1])
                    buf[o + j * 2] = '0';
                else 
                    buf[o + j * 2] = '0' + ((set >> j) & 0x01);
            }
            formatstr += buf;
            formatstr += '\n';
        }
        formatstr += '\n';
    }

    return formatstr;
}

Cron::CO_STAT Cron::getStatus(cron_id_t id_)
{
    SCHE sche;
    {
        Poco::RWLock::ScopedLock rlock(_sche_rwlock);
        if (_sche_map.find(id_) == _sche_map.end())
            return Cron::CO_DEAD;
        sche = _sche_map[id_];
    }

    Poco::UInt32 &flags = sche->get<SCHE_FLAG>();
    if (flags & CO_INVALID)
        return Cron::CO_DEAD;

    Counter &counter = sche->get<SCHE_COUNTER>();
    if (counter() == 0)
        return Cron::CO_SLEEP;

    thread_t &thr = sche->get<SCHE_THREAD>();
    if (thr)
        return Cron::CO_RUNNING;
    return Cron::CO_RUNNABLE;
}

void Cron::Start(cron_id_t id_)
{
    SCHE sche;
    {
        Poco::RWLock::ScopedLock rlock(_sche_rwlock);
        if (_sche_map.find(id_) == _sche_map.end())
            return;
        sche = _sche_map[id_];
    }

    Poco::LocalDateTime now;
    logDebug("%s, manual start a task(%d)\n", Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::SORTABLE_FORMAT).c_str(), id_);
    startTask(sche);
}

void Cron::startTask(SCHE sche)
{
    Poco::UInt32 &flags = sche->get<SCHE_FLAG>();
    if (flags & CO_INVALID)
        return;

    Poco::FastMutex::ScopedLock lock(_start_mutex);

    cron_id_t id_ = sche->get<SCHE_ID>();
    Counter &counter = sche->get<SCHE_COUNTER>();
    /**不允许重叠*/
    if (!(flags & CO_OVERLAP) && counter() > 0){
        logDebug("task(%d) is on running, can't start a non-overlapping task", id_);
        return;
    }
    ++counter;

    /**独立运行*/
    if (flags & CO_ALONE){
        //启动私有计划任务执行线程，这些对象处理结束会自动销毁
        try{
            ThreadTarget<Cron, SCHE>::startThread(*this, &Cron::runPrivate, sche);
        }
        catch (Poco::Exception &e){
            --counter;
            logError("failed to start a private task(%d), %s\n", id_, e.displayText().c_str());
        }
        catch (...){
            --counter;
            logError("failed to start a private task(%d)\n", id_);
        }
    }
    else{
        std::list<SCHE> &list_ = _run_list->get<0>();
        Poco::FastMutex &mutex_= _run_list->get<1>();
        Poco::FastMutex::ScopedLock lock(mutex_);
        list_.push_back(sche);
    }
}

void Cron::run()
{
    //启动公共计划任务执行线程，这些对象由公共线程负责销毁
    //不能有错误
    ThreadTarget<Cron, int>::startThread(*this, &Cron::runPublic, 0);
    
    Poco::LocalDateTime now;
    while(1){
        now = Poco::LocalDateTime();
        if (_stop_event->tryWait((60 - now.second()) * 1000)){
            break;
        }

        now = Poco::LocalDateTime();
        printf("%s\n", Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::SORTABLE_FORMAT).c_str());
        Poco::UInt8 minute  = now.minute();
        Poco::UInt8 hour    = now.hour();
        Poco::UInt8 day     = now.day();
        Poco::UInt8 month   = now.month();
        Poco::UInt8 week    = now.dayOfWeek();

        Poco::RWLock::ScopedLock rlock(_sche_rwlock);
        for(std::map<cron_id_t, SCHE>::iterator it = _sche_map.begin(); it != _sche_map.end(); ++it){
            SCHE sche = it->second;
            Poco::UInt32 &flags = sche->get<SCHE_FLAG>();
            if (flags & CO_INVALID)
                continue;

            cron_id_t id_ = INVALID_CRON_ID;
            std::vector<date_rec> &recs = sche->get<SCHE_DATE>();
            for(int i = 0; i < recs.size(); ++i){
                date_rec &rec = recs[i];
                if (!CHECK(rec.sche.part.months, month))
                    continue;
                if (!(CHECK(rec.sche.part.days, day) || CHECK(rec.sche.part.weeks, week)))
                    continue;
                if (!CHECK(rec.sche.part.hours, hour))
                    continue;
                if (!CHECK(rec.sche.part.minutes, minute))
                    continue;

                id_ = sche->get<SCHE_ID>();
                break;
            }
            if (id_ == INVALID_CRON_ID)
                continue;
            
            startTask(sche);
        }
    }
}

void Cron::runPublic(int)
{
    Poco::SharedPtr<Poco::Event> stop_ = _stop_event;
    Poco::SharedPtr<Poco::Tuple<std::list<SCHE>, Poco::FastMutex> > run_list = _run_list;
    std::list<SCHE> &list_ = run_list->get<0>();
    Poco::FastMutex &mutex_= run_list->get<1>();
    
    while(1){
        if (stop_->tryWait(list_.empty() ? 300 : 0))
            break;
        if (list_.empty())
            continue;
        
        logDebug("start a public task\n");

        SCHE sche;
        {
            Poco::FastMutex::ScopedLock lock(mutex_);
            sche = list_.front();
            list_.pop_front();
        }
        
        cron_id_t id_       = sche->get<SCHE_ID>();
        CronCaller &caller  = sche->get<SCHE_CALLER>();
        Counter &counter    = sche->get<SCHE_COUNTER>();
        Poco::UInt32 flags  = sche->get<SCHE_FLAG>();
        //thread_t &thr       = sche->get<SCHE_THREAD>();

/* #ifdef POCO_OS_FAMILY_UNIX
        thr = pthread_self();
#elif defined(POCO_OS_FAMILY_WINDOWS)
        thr = GetCurrentThreadId();
#endif */

        try{
            /**有效的任务执行*/
            if (!(flags & CO_INVALID)){
                logDebug("launch a scheduled task(%d) in the public queue\n", id_);
                caller(id_);
            }
            else{
                logDebug("scheduled task(%d) had canceled in the public queue", id_);
            }
        }
        catch(Poco::Exception &e){
            logError("failed to execute a task(%d) in the public queue, %s\n", id_, e.displayText().c_str());
        }
        catch(...){
            logError("failed to execute a task(%d) in the public queue\n", id_);
        }
        
/* #ifdef POCO_OS_FAMILY_UNIX
        thr = 0;
#elif defined(POCO_OS_FAMILY_WINDOWS)
        thr = 0;
#endif */
        --counter;
    }
}

void Cron::runPrivate(SCHE sche)
{
    cron_id_t id_       = sche->get<SCHE_ID>();
    CronCaller &caller  = sche->get<SCHE_CALLER>();
    Counter &counter    = sche->get<SCHE_COUNTER>();
    Poco::UInt32 flags  = sche->get<SCHE_FLAG>();
    //thread_t &thr       = sche->get<SCHE_THREAD>();

/* #ifdef POCO_OS_FAMILY_UNIX
    thr = pthread_self();
#elif defined(POCO_OS_FAMILY_WINDOWS)
    thr = GetCurrentThreadId();
#endif */

    logDebug("start a private task\n");

    try{
        /**有效的任务执行*/
        if (!(flags & CO_INVALID)){
            logDebug("launch a scheduled task(%d) in the private queue\n", id_);
            caller(id_);
        }
        else{
            logWarn("scheduled task(%d) had canceled in the private queue", id_);
        }
    }
    catch(Poco::Exception &e){
        logError("failed to execute a task(%d) in the private queue, %s\n", id_, e.displayText().c_str());
    }
    catch(...){
        logError("failed to execute a task(%d) in the private queue\n", id_);
    }

/* #ifdef POCO_OS_FAMILY_UNIX
    thr = 0;
#elif defined(POCO_OS_FAMILY_WINDOWS)
    thr = 0;
#endif */
    --counter;
}

}}

