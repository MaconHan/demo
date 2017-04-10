/**
@file
* ��Ȩ����(C)2001, ����������ͨѶ�ɷ����޹�˾<br/>
* �ļ����ƣ�Cron.h<br/>
* ����ժҪ���ƻ�����<br/>
* ����˵������<br/>
@version 1.0
@author han.yamin
@since  2013-5
@note   �ƻ�ʱ���ʽΪ��M h d m w(�� ʱ �� �� ����)��ʹ�ÿո�ָ�<br/>
        M: ���ӣ�0-59��<br/>
        h��Сʱ��0-23��<br/>
        d���죨1-31��<br/>
        m: �£�1-12��<br/>
        w: һ�����ڵ��죨0~6��0Ϊ�����죩��<br/>
        �������ֻ��м���������ķ��ž���"*"��"/"��"-"��","��*�������е�ȡֵ��Χ�ڵ����֣�"/"����ÿ����˼,"��/5"��ʾÿ5����λ��"-"�����ĳ�����ֵ�ĳ������,","�ֿ�������ɢ�����֡�
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
@brief �ƻ����������
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
@brief �ƻ����������
*/
class NGOMM_API Cron : public Poco::Runnable
{
public:
    /**@brief �ƻ�ʱ��*/
    typedef struct{
        union{
            struct {
                /**@brief ����*/
                Poco::UInt64    minutes;
                /**@brief Сʱ*/
                Poco::UInt64    hours;
                /**@brief ���е���*/
                Poco::UInt64    days;
                /**@brief ��*/
                Poco::UInt64    months;
                /**@brief ����*/
                Poco::UInt64    weeks;
            }   part;

            Poco::UInt64        parts[5];
        }   sche;

        /**@brief �ƻ�ʱ���ַ���*/
        std::string             str;
    } date_rec;

    /**@brief �ƻ�������*/
    typedef enum{
        /**�ƻ������ص����У�����һ���ƻ�����������ʱ����ǰ���ڵļƻ�����Ҳ��������*/
        CO_OVERLAP  = 0x00000001,

        /**�ƻ�����������У���������һ���߳�ִ�мƻ����񣬷���ƻ������Ŷ�ִ��*/
        CO_ALONE    = 0x00000002,

        /**�ƻ�����ʧЧ*/
        CO_INVALID  = 0x80000000
    } CO_FLAG;

    /**@brief �ƻ�����״̬*/
    typedef enum{
        /**�ƻ������Ѿ����ڱ����٣������Ѿ�������*/
        CO_DEAD     = 0x00,
        
        /**�ƻ������������У��ȴ��ƻ�ʱ�䵽��ʱ����*/
        CO_SLEEP    = 0x01, 

        /**�ƻ����������ж������Ŷ�*/
        CO_RUNNABLE = 0x02, 

        /**�ƻ�������������*/
        CO_RUNNING  = 0x04
    } CO_STAT;
    
    /**@brief �ƻ�����ʹ�ü�����*/
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
    @brief ���캯��
    */
    Cron();

    /**
    @brief ��������
    */
    ~Cron();

public:
    /**
    @brief ע��ƻ�����
    @param[in]  crontab     �ƻ�ʱ��
    @param[in]  callback    �ƻ�����ʱ�ص�����
    @param[in]  flags       ��ǣ��ο�CO_FLAGö�ٶ���
    @return     >=0         �ƻ�����idֵ<br/>
                ==INVALID_CRON_ID        ��������<br/>
    @note �ƻ�ʱ���ʽΪ��M h d m w(�� ʱ �� �� ����)��ʹ�ÿո�ָ�<br/>
    M: ���ӣ�0-59��<br/>
    h��Сʱ��0-23��<br/>
    d���죨1-31��<br/>
    m: �£�1-12��<br/>
    w: һ�����ڵ��죨0~6��0Ϊ�����죩��<br/>
    �������ֻ��м���������ķ��ž���"*"��"/"��"-"��","��*�������е�ȡֵ��Χ�ڵ����֣�"/"����ÿ����˼,"��/5"��ʾÿ5����λ��"-"�����ĳ�����ֵ�ĳ������,","�ֿ�������ɢ�����֡�
    */
    cron_id_t Register(const std::string &crontab, CronCaller &callback, Poco::UInt32 flags = 0);

    /**
    @brief ��һ���Ѿ����ڵļƻ�����������һ���ƻ�ʱ��
    @param[in]  id_         �ƻ�����idֵ
    @param[in]  crontab     �ƻ�ʱ��
    @return     >=0         �ƻ�����idֵ����ֵӦ�õ����������id_<br/>
                ==INVALID_CRON_ID        �������󣬻���ָ���ļƻ�����idֵ������<br/>
    @note   һ���ƻ������Ͽ��԰�������ƻ�ʱ�䣬ֻҪһ�������������ƻ�����ͻ��ɷ���
    */
    cron_id_t Register(cron_id_t id_, const std::string &crontab);

    /**
    @brief ע��һ���ƻ�����
    @param[in]  id_         �ƻ�����idֵ
    @return     N/A
    @note   ���ע���ļƻ���������ִ�У��÷���������ֱ������ִ�н���
    */
    void Unregister(cron_id_t id_);
    
    /**
    @brief ����һ���ƻ�����
    @param[in]  id_         �ƻ�����idֵ
    @return     N/A
    @note
    */
    void Start(cron_id_t id_);

    /**
    @brief ���һ���ƻ��������ڵ��ַ���
    @param[in]  id_         �ƻ�����idֵ
    @return     N/A
    @note
    */
    std::string toStr(cron_id_t id_);

    /**
    @brief ��ȡһ���ƻ����������״̬
    @param[in]  id_         �ƻ�����idֵ
    @return     ����״̬���ο�CO_STATö�ٶ���
    @note
    */
    Cron::CO_STAT getStatus(cron_id_t id_);

protected:

    void startTask(SCHE sche);

    /**
    @brief �Լ�����ÿ�������м�⵽�ڼƻ�����
    @param[in]
    @return     N/A
    @note
    */
    void run();

    /**
    @brief �����ƻ������̺߳���
    @param[in]  sche    ռλʹ�ã�����
    @return     N/A
    @note       �򹫹��ƻ������̴߳�_run_list�б��л�ȡ������ļƻ����񣬹��������scheû�б�ʹ�ã�ֻ��һ��ռλ����
    */
    void runPublic(int);

    /**
    @brief ˽�мƻ������̺߳�������Ҫִ��flags����CO_ALONE������
    @param[in]  sche        �ƻ���������Ϣ
    @return     N/A
    @note
    */
    void runPrivate(SCHE sche);

    /**
    @brief �����ƻ�ʱ��
    @param[in]  rec     �ƻ�ʱ��
    @return     =0  �ɹ�<br/>
                =-1 ����ʧ��
    @note
    */
    Poco::Int32 parse(date_rec &rec);

    /**
    @brief ����ÿ���εļƻ�ʱ��
    @param[in]      index   �ƻ�ʱ��ε��±�
    @param[in]      pattern ��ʽ
    @param[in,out]  set     ���ݸ�ʽ���ý��
    @return     =0  �ɹ�<br/>
                =-1 ����ʧ��
    @note
    */
    Poco::Int32 parse_part(Poco::UInt8 index, const std::string &pattern, Poco::UInt64 &set);

private:
    Poco::SharedPtr<Poco::Event> _stop_event;
    /**@brief ����߳�*/
    Poco::Thread    _check_thread;
    
    /**@brief ����������*/
    Poco::FastMutex _start_mutex;
    /**@brief ȫ�ּƻ��������*/
    Poco::FastMutex _id_mutex;
    /**@brief ȫ�ּƻ������*/
    Poco::UInt16    _global_id;

    /**@brief �ƻ�������*/
    Poco::RWLock                        _sche_rwlock;
    /**@brief �ƻ�����map*/
    std::map<cron_id_t, SCHE>           _sche_map;

    /**@brief ������ƻ������б���Ҫ�ǹ����ƻ������߳�ʹ��*/
    Poco::SharedPtr<Poco::Tuple<std::list<SCHE>, Poco::FastMutex> >  _run_list;

public:
    DECLARE_STATIC_LOG();

public:
    /**@brief �ƻ�������*/
    static Poco::SingletonHolder<Cron>    CronSingleton;
};

}}
#endif
