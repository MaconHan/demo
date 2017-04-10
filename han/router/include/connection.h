/**@file
* ��Ȩ����(C)2008, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�zteomm/NDF/connection.h<br>
* �ļ���ʶ��
* ����ժҪ��SSS 2.0 OMMϵͳ��·ģ�����ļ�<br>
* ����˵������<br>
* @version 1.0<br>
* @author <br>
* @since 2009��6��8��<br>
*/
#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>

#include "syscomm.h"

#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/SocketConnector.h"
#include "Poco/Net/SocketAcceptor.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Observer.h"
#include "Poco/SingletonHolder.h"

#include "Poco/Thread.h"
#include "Poco/Runnable.h"
#include "Poco/Event.h"

#include "vcn_defs.h"

using namespace std;
using namespace zteomm::pub;
using namespace Poco;
using namespace Poco::Net;


NS_VCN_OAM_BEGIN(ROUTER)

typedef enum
{
    EN_LINK_CLOSE = 0x0,
    EN_LINK_CONNECTING,
    EN_LINK_CONNECTED,
} ENLinkStatus;

struct flush_out_t{};
constexpr flush_out_t flush_out{};

/**
* ��·ģ����
*/
template <typename _ST = decltype(nullptr), typename _DT = decltype(nullptr)>
class Connection
{
public:
    Connection(StreamSocket &socket, SocketReactor &reactor, const string &strName = "Connection"):
        m_tSocket(socket),
        m_tReactor(reactor)
    {
        CREATE_NAME_LOG(strName.c_str());
        initSystem();
    }

    virtual ~Connection()
    {
        endSystem();
    }

    virtual void onReadable(ReadableNotification* pNf)
    {
        pNf->release();

        try{
            /* ������������ */
            if (!readData()){
                /* ��ȡ�����쳣 */
                logError("failed to read from socket(%s)", m_strPeerIP.c_str());
                disconnect();
                return;
            }

            /* ���� */
            if (m_bState != OMS_TCP_WORKING)
                shakeHand();
            /* ��Ϣ�ַ� */
            else
                dispatchMsg();
        }
        catch (Poco::Exception &e){
            logError("failed to read from socket(%s), %s", m_strPeerIP.c_str(), e.displayText().c_str());
            disconnect();
        }
    }

    virtual void onTimeout( TimeoutNotification* pNf )
    {
        pNf->release();

        if (m_bState != OMS_TCP_WORKING){
            m_iConnectTimes++;

            //15s���ֳ�ʱ����
            if (m_iConnectTimes >= (5 * m_iHeartCounts)){
                logFatal( ">>>>> In onTimeout,the connection shake hand failed." );
                m_iConnectTimes = 0;
                disconnect();
            }
            else{
                return;
            }
        }

        if (!m_bConnected){
            try{
                m_tReactor.removeEventHandler( m_tSocket, Observer<Connection, TimeoutNotification>( *this, &Connection::onTimeout ) );
            }
            catch ( Poco::Net::NetException& ex ){
                logError( "In endSystem,Poco::Net::NetException : %s", ex.what() );
            }
            catch ( Poco::Exception& ex ){
                logError( "In endSystem,Poco::Exception : %s", ex.what() );
            }
            catch (...){
                logError( "Unknown exception. close socket error." );
            }

            /* �رո���· */
            closeConnection();
            /* ɾ���÷������ */
            deleteConnect();
        }
        else{
            /* ��·�����ݣ������������ */
            if ( m_bLinkActive == OMS_LINK_ACTIVE )
                m_iHeartBeats = 0;

            m_iHeartBeats++;
            heartBeat();
            m_bLinkActive = OMS_LINK_IDLE;
        }
    }

    virtual void onError(ErrorNotification* pNf)
    {
        pNf->release();
        try{
            m_tReactor.removeEventHandler(m_tSocket, Observer<Connection, ErrorNotification>( *this, &Connection::onError));
            disconnect();
        }
        catch (Poco::Exception& ex){
            logError("In endSystem, Poco::Exception : %s", ex.displayText().c_str());
        }
        catch (...){
            logError("Unknown exception. close socket error.");
        }
    }

    Connection<_ST, _DT>& operator<< (const char *output_str)
    {
        this->sendBytes(output_str, strlen(output_str));
        return *this;
    }

    Connection<_ST, _DT>& operator<< (const std::string &output_str)
    {
        this->sendBytes(output_str.c_str(), output_str.size());
        return *this;
    }

    Connection<_ST, _DT>& operator<< (const flush_out_t&)
    {
        flush();
        return *this;
    }

    virtual bool sendBytes(const void *lpBytes, std::size_t len, int flags = 0)
    {
        /* ��·���ڻ*/
        m_bLinkActive = OMS_LINK_ACTIVE;
        if (m_tOutBuffer.getLen() + len >= 4 * 1024)
            flush();
        m_tOutBuffer.putData((BYTE*)lpBytes, len);
        return true;
    }

    void flush()
    {
        if (!m_bConnected)
            return;

        std::size_t len = m_tOutBuffer.getLen();
        const void *data= m_tOutBuffer.getData();
        if (!len)
            return;

        try{
            len = m_tSocket.sendBytes(data, len);
            m_tOutBuffer.distract(len);
        }
        catch (Poco::Exception& ex){
            logError("failed to terminal[%s], %s", m_strPeerIP.c_str(), ex.displayText().c_str());
        }
        catch(...){
            logError("failed to terminal[%s], unknown reason", m_strPeerIP.c_str());
        }
    }

protected:
    /**< ���ֽ��� */
    virtual void shakeHand() {}

    /**< ��Ϣ�ַ� */
    virtual void dispatchMsg() {}

    /**< �ر���·���� */
    virtual void closeConnection() {}

    /**< ������Ԫ */
    virtual void setNEID() {}

    /**< ���õ�¼״̬ */
    virtual void setLoginState() {}

    /**< �������� */
    virtual void heartBeat( bool bActive = true ) {}

    /**< ��ӡ��Ϣͷ */
    virtual void printMsgHead( const _DT& msgHead, bool bRecv ) {}

    /**< ��������� */
    virtual void sendFormatResponse( const LPBYTE lpBytes, int iLen, int flags = 0 ) {}

    void initSystem()
    {
        try
        {
            m_tSocket.setBlocking(false);
            m_bWriteable = false;
            m_bConnected = true;

            m_iHeartCounts = int( 3 * 1000 / m_tReactor.getTimeout().milliseconds() );
            m_tReactor.addEventHandler( m_tSocket, Observer<Connection, ReadableNotification>( *this, &Connection::onReadable ) );
            m_tReactor.addEventHandler( m_tSocket, Observer<Connection, TimeoutNotification>( *this, &Connection::onTimeout ) );
            m_tReactor.addEventHandler( m_tSocket, Observer<Connection, ErrorNotification>( *this, &Connection::onError ) );

            try
            {
                m_strIP = m_tSocket.address().host().toString();
                m_strPeerIP = m_tSocket.peerAddress().host().toString();
            }
            catch( ... )
            {
                logError("[NDF::Connection] PEERIP get fail:%s %s ", m_strIP.c_str(), m_strPeerIP.c_str() );
            }
        }
        catch ( Poco::Net::NetException& ex )
        {
            logError( "In initSystem,Poco::Net::NetException : %s", ex.what() );
        }
        catch ( Poco::Exception& ex )
        {
            logError( "In initSystem,Poco::Exception : %s", ex.what() );
        }
        catch ( ... )
        {
            logError( "In initSystem,other excption" );
        }
    }

    void endSystem()
    {
        try
        {
            m_tReactor.removeEventHandler( m_tSocket, Observer<Connection, ReadableNotification>( *this, &Connection::onReadable ) );
            m_tReactor.removeEventHandler( m_tSocket, Observer<Connection, TimeoutNotification>( *this, &Connection::onTimeout ) );
            m_tReactor.removeEventHandler( m_tSocket, Observer<Connection, ErrorNotification>( *this, &Connection::onError ) );
            m_tSocket.close();
        }
        catch ( Poco::Net::NetException& ex )
        {
            logError( "In endSystem,Poco::Net::NetException : %s", ex.what() );
        }
        catch ( Poco::Exception& ex )
        {
            logError( "In endSystem,Poco::Exception : %s", ex.what() );
        }
        catch ( ... )
        {
            logError( "Unknown exception. close socket error." );
        }
    }

    virtual bool readData() = 0;

    bool writeData()
    {
        return sendBytes( NULL, 0 );
    }

    void sendTestConnect()
    {
        m_iHeartBeats = 0;
        m_tSocket.setBlocking(false);
        heartBeat();
    }

    void disconnect()
    {
        try
        {
            m_tReactor.removeEventHandler( m_tSocket, Observer<Connection, ReadableNotification>( *this, &Connection::onReadable ) );
            /* ������·״̬Ϊfalse */
            m_bConnected = false;
        }
        catch ( Poco::Net::NetException& ex )
        {
            logError( "In endSystem,Poco::Net::NetException : %s", ex.what() );
        }
        catch ( Poco::Exception& ex )
        {
            logError( "In endSystem,Poco::Exception : %s", ex.what() );
        }
        catch ( ... )
        {
            logError( "Unknown exception. close socket error." );
        }
    }

    void deleteConnect()
    {
        delete this;
    }

    inline WORD32 IP2Long( const string& strIP )
    {
        int a[4];
        string IP = strIP;
        string strTemp;
        size_t pos;
        size_t i = 3;

        do
        {
            pos = IP.find( "." );

            if ( pos != string::npos )
            {
                strTemp = IP.substr( 0, pos );
                a[i] = atoi( strTemp.c_str() );
                i--;
                IP.erase( 0, pos + 1 );
            }
            else
            {
                strTemp = IP;
                a[i] = atoi( strTemp.c_str() );
                break;
            }
        }
        while ( 1 );

        WORD32 dwResult = ( a[3] << 24 ) + ( a[2] << 16 ) + ( a[1] << 8 ) + a[0];

        return dwResult;
    }

    inline string LongToIP( const WORD32& nValue )
    {
        char strTemp[20];
        sprintf( strTemp, "%d.%d.%d.%d",
                 ( nValue&0xff000000 ) >> 24,
                 ( nValue&0x00ff0000 ) >> 16,
                 ( nValue&0x0000ff00 ) >> 8,
                 ( nValue&0x000000ff ) );
        return string( strTemp );
    }

    inline bool isLittleEndian()
    {
        static int iInit = 0;
        static bool bLittleEndian = false;

        if ( iInit == 0 )
        {
            char szAB[2] = {0x0A, 0x0B};
            WORD16 wAB;

            memcpy( &wAB, szAB, sizeof( wAB ) );

            if ( wAB == 0x0B0A )
            {
                bLittleEndian = true;
            }

            iInit = 1;
        }

        return bLittleEndian;
    }

protected:
    enum
    {
        OMS_TCP_INIT = 0x0,
        OMS_TCP_WAIT_CONN_REQ = 0x01,
        OMS_TCP_WAIT_RECEPT_ACK,
        OMS_TCP_WORKING,
    };
    enum
    {
        OMS_CONN_READ_MSGHEAD = 0x01,
        OMS_CONN_READ_MSGBODY,
    };
    enum
    {
        OMS_LINK_IDLE = 0x00,
        OMS_LINK_ACTIVE = 0x01,
    };

protected:
    StreamSocket        m_tSocket;
    SocketReactor       &m_tReactor;
    Poco::FastMutex     m_tMutex;

    /**< ��·״̬���� */
    bool                m_bConnected;
    bool                m_bWriteable;
    BYTE                m_bState        = OMS_TCP_INIT;
    BYTE                m_bReadState    = OMS_CONN_READ_MSGHEAD;
    BYTE                m_bLinkActive   = 0;
    int                 m_iHeartCounts  = 0;
    int                 m_iConnectTimes = 0 ;
    int                 m_iHeartBeats   = 0;

    string             m_strPeerIP;
    string             m_strIP;

    /**< ������ */
    CEvBuffer          m_tInBuffer;
    CEvBuffer          m_tOutBuffer;

    DECLARE_LOG();
};

NS_VCN_OAM_END

#endif
