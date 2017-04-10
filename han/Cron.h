/**
@file
* 版权所有(C)2001, 深圳市中兴通讯股份有限公司<br/>
* 文件名称：Cron.h<br/>
* 内容摘要：计划任务<br/>
* 其它说明：无<br/>
@version 1.0
@author han.yamin
@since  2013-5
@note   计划时间格式为：M h d m w(分 时 天 月 星期)，使用空格分割<br/>
        M: 分钟（0-59）<br/>
        h：小时（0-23）<br/>
        d：天（1-31）<br/>
        m: 月（1-12）<br/>
        w: 一星期内的天（0~6，0为星期天）。<br/>
        除了数字还有几个个特殊的符号就是"*"、"/"和"-"、","，*代表所有的取值范围内的数字，"/"代表每的意思,"＊/5"表示每5个单位，"-"代表从某个数字到某个数字,","分开几个离散的数字。
*/
#ifndef __CRON_H__
#define __CRON_H__

#include <string>
#include <map>
#include <list>

#include "Poco/Types.h"
#include "Poco/Runnable.h"
#include "Poco/Activity.h"
#include "Poco/Thread.h"
#include "Poco/Mutex.h"
#include "Poco/RWLock.h"
#include "Poco/Tuple.h"
#include "Poco/SharedPtr.h"
#include "Poco/SingletonHolder.h"

#include "exportlib.h"
#include "log.h"
#include "syscomm.h"

namespace zteomm
{
namespace pub
{

/**
@brief 计划任务调用器
*/
class CronCaller
{
private:
    struct CallerBase
    {
        virtual void operator()(int) = 0;
        virtual CallerBase *clone() = 0;
    };

    template<typename C>
    struct MemCaller : public CallerBase
    {
        typedef void (C::*Callback)(int);

        MemCaller(C &c, Callback method) : _p(&c), _method(method)
        {
        }

        void operator()(int id_)
        {
            (_p->*_method)(id_);
        }

        CallerBase *clone()
        {
            MemCaller<C> *p = new MemCaller(*_p, _method);
            return p;
        }

        C *_p;
        Callback _method;
    };

    struct FuncCaller : public CallerBase
    {
        typedef void (*Callback)(int);

        FuncCaller(Callback method) :_function(method)
        {
        }

        void operator()(int id_)
        {
            (*_function)(id_);
        }

        CallerBase *clone()
        {
            FuncCaller *p = new FuncCaller(_function);
            return p;
        }

        Callback _function;
    };


public:
    CronCaller() : _p(NULL)
    {
    }

    template<typename C>
    CronCaller(C &c, typename MemCaller<C>::Callback callback) : _p(new MemCaller<C>(c, callback))
    {
    }

    CronCaller(FuncCaller::Callback callback) : _p(new FuncCaller(callback))
    {
    }

    ~CronCaller()
    {
        if (_p)
            delete _p;
        _p = NULL;
    }

    CronCaller(const CronCaller &other) : _p(other._p ? other._p->clone() : NULL)
    {
    }

    CronCaller& operator=(const CronCaller &other)
    {
        if (_p)
            delete _p;
        _p = NULL;

        if (other._p)
            _p = other._p->clone();

        return *this;
    }

public:
    void operator()(int id_)
    {
        if (_p)
            (*_p)(id_);
    }

private:
    CallerBase *_p;
};


typedef Poco::Int32  cron_id_t;
#define INVALID_CRON_ID  (cron_id_t)-1
/**
@brief 计划任务管理器
*/
class NGOMM_API Cron : public Poco::Runnable
{
public:
    /**@brief 计划时间*/
    typedef struct{
        union{
            struct {
                /**@brief 分钟*/
                Poco::UInt64    minutes;
                /**@brief 小时*/
                Poco::UInt64    hours;
                /**@brief 月中的天*/
                Poco::UInt64    days;
                /**@brief 月*/
                Poco::UInt64    months;
                /**@brief 星期*/
                Poco::UInt64    weeks;
            }   part;

            Poco::UInt64        parts[5];
        }   sche;

        /**@brief 计划时间字符串*/
        std::string             str;
    } date_rec;

    /**@brief 计划任务标记*/
    typedef enum{
        /**计划任务重叠运行，及上一个计划任务还在运行时允许当前到期的计划任务也可以运行*/
        CO_OVERLAP  = 0x00000001,

        /**计划任务独立运行，单独启动一个线程执行计划任务，否则计划任务排队执行*/
        CO_ALONE    = 0x00000002,

        /**计划任务失效*/
        CO_INVALID  = 0x80000000
    } CO_FLAG;

    /**@brief 计划任务状态*/
    typedef enum{
        /**计划任务已经正在被销毁，或者已经不在了*/
        CO_DEAD     = 0x00,
        
        /**计划任务处于休眠中，等待计划时间到达时唤醒*/
        CO_SLEEP    = 0x01, 

        /**计划任务在运行队列中排队*/
        CO_RUNNABLE = 0x02, 

        /**计划任务正在运行*/
        CO_RUNNING  = 0x04
    } CO_STAT;
    
    /**@brief 计划任务使用计数器*/
    typedef struct Counter{
        Counter() : _counter(0), _event(false)
        {
            _event.set();
        }

        inline Poco::UInt16 operator++()
        {
            Poco::Mutex::ScopedLock lock(_mutex);
            if (_counter++ == 0)
                _event.reset();
            return _counter;
        }

        inline Poco::UInt16 operator++(int)
        {
            Poco::Mutex::ScopedLock lock(_mutex);
            Poco::UInt16 i = _counter;
            ++(*this);
            return i;
        }

        inline Poco::UInt16 operator--()
        {
            Poco::Mutex::ScopedLock lock(_mutex);
            if (--_counter == 0)
                _event.set();
            return _counter;
        }

        inline Poco::UInt16 operator--(int)
        {
            Poco::Mutex::ScopedLock lock(_mutex);
            Poco::UInt16 i = _counter;
            --(*this);
            return _counter;
        }

        inline Poco::UInt16 operator()()
        {
            return _counter;
        }

        inline void operator()(Poco::UInt16 c)
        {
            Poco::Mutex::ScopedLock lock(_mutex);
            _counter = c;
            if (_counter == 0)
                _event.set();
            else
                _event.reset();
        }

        inline void wait()
        {
            _event.wait();
        }

        Poco::Mutex     _mutex;
        Poco::Event     _event;
        Poco::UInt16    _counter;
    } Counter;

    typedef enum{
        SCHE_ID     = 0,
        SCHE_DATE   = 1, 
        SCHE_CALLER = 2, 
        SCHE_COUNTER= 3, 
        SCHE_FLAG   = 4,
        SCHE_THREAD = 5
    }   SCHE_TUPLE;

#ifdef POCO_OS_FAMILY_UNIX
    typedef pthread_t   thread_t;
#else
    typedef DWORD       thread_t;
#endif

    typedef Poco::Tuple<Poco::Int32, std::vector<date_rec>, CronCaller, Counter, Poco::UInt32, thread_t> _SCHE;
    typedef Poco::SharedPtr<_SCHE> SCHE;

public:
    /**
    @brief 构造函数
    */
    Cron();

    /**
    @brief 析构函数
    */
    ~Cron();

public:
    /**
    @brief 注册计划任务
    @param[in]  crontab     计划时间
    @param[in]  callback    计划到期时回调函数
    @param[in]  flags       标记，参考CO_FLAG枚举定义
    @return     >=0         计划任务id值<br/>
                ==INVALID_CRON_ID        解析错误<br/>
    @note 计划时间格式为：M h d m w(分 时 天 月 星期)，使用空格分割<br/>
    M: 分钟（0-59）<br/>
    h：小时（0-23）<br/>
    d：天（1-31）<br/>
    m: 月（1-12）<br/>
    w: 一星期内的天（0~6，0为星期天）。<br/>
    除了数字还有几个个特殊的符号就是"*"、"/"和"-"、","，*代表所有的取值范围内的数字，"/"代表每的意思,"＊/5"表示每5个单位，"-"代表从某个数字到某个数字,","分开几个离散的数字。
    */
    cron_id_t Register(const std::string &crontab, CronCaller &callback, Poco::UInt32 flags = 0);

    /**
    @brief 在一个已经存在的计划任务上增加一个计划时间
    @param[in]  id_         计划任务id值
    @param[in]  crontab     计划时间
    @return     >=0         计划任务id值，该值应该等于输入参数id_<br/>
                ==INVALID_CRON_ID        解析错误，或者指定的计划任务id值不存在<br/>
    @note   一个计划任务上可以包含多个计划时间，只要一个满足条件，计划任务就会派发。
    */
    cron_id_t Register(cron_id_t id_, const std::string &crontab);

    /**
    @brief 注销一个计划任务
    @param[in]  id_         计划任务id值
    @return     N/A
    @note   如果注销的计划任务正在执行，该方法会阻塞直到任务执行结束
    */
    void Unregister(cron_id_t id_);
    
    /**
    @brief 启动一个计划任务
    @param[in]  id_         计划任务id值
    @return     N/A
    @note
    */
    void Start(cron_id_t id_);

    /**
    @brief 输出一个计划任务日期的字符串
    @param[in]  id_         计划任务id值
    @return     N/A
    @note
    */
    std::string toStr(cron_id_t id_);

    /**
    @brief 获取一个计划任务的运行状态
    @param[in]  id_         计划任务id值
    @return     运行状态，参考CO_STAT枚举定义
    @note
    */
    Cron::CO_STAT getStatus(cron_id_t id_);

protected:

    void startTask(SCHE sche);

    /**
    @brief 自检任务，每分钟运行检测到期计划任务
    @param[in]
    @return     N/A
    @note
    */
    void run();

    /**
    @brief 公共计划任务线程函数
    @param[in]  sche    占位使用，弃用
    @return     N/A
    @note       因公共计划任务线程从_run_list列表中获取待处理的计划任务，故输入参数sche没有被使用，只是一个占位符。
    */
    void runPublic(int);

    /**
    @brief 私有计划任务线程函数，主要执行flags等于CO_ALONE的任务
    @param[in]  sche        计划任务定义信息
    @return     N/A
    @note
    */
    void runPrivate(SCHE sche);

    /**
    @brief 解析计划时间
    @param[in]  rec     计划时间
    @return     =0  成功<br/>
                =-1 解析失败
    @note
    */
    Poco::Int32 parse(date_rec &rec);

    /**
    @brief 解析每个段的计划时间
    @param[in]      index   计划时间段的下标
    @param[in]      pattern 格式
    @param[in,out]  set     根据格式设置结果
    @return     =0  成功<br/>
                =-1 解析失败
    @note
    */
    Poco::Int32 parse_part(Poco::UInt8 index, const std::string &pattern, Poco::UInt64 &set);

private:
    Poco::SharedPtr<Poco::Event> _stop_event;
    /**@brief 检测线程*/
    Poco::Thread    _check_thread;
    
    /**@brief 启动任务锁*/
    Poco::FastMutex _start_mutex;
    /**@brief 全局计划任务号锁*/
    Poco::FastMutex _id_mutex;
    /**@brief 全局计划任务号*/
    Poco::UInt16    _global_id;

    /**@brief 计划任务锁*/
    Poco::RWLock                        _sche_rwlock;
    /**@brief 计划任务map*/
    std::map<cron_id_t, SCHE>           _sche_map;

    /**@brief 待处理计划任务列表，主要是公共计划任务线程使用*/
    Poco::SharedPtr<Poco::Tuple<std::list<SCHE>, Poco::FastMutex> >  _run_list;

public:
    DECLARE_STATIC_LOG();

public:
    /**@brief 计划任务单例*/
    static Poco::SingletonHolder<Cron>    CronSingleton;
};

}}
#endif
