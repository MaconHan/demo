/**@file
* ��Ȩ����(C)2008, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�TelnetHandle.h<br>
* �ļ���ʶ��
* ����ժҪ��SSS 2.0 NDFͨ�ŷ���ͷ�ļ�<br>
* ����˵������<br>
* @version 1.0<br>
* @author <br>
* @since 2009��5��12��<br>
*/

#ifndef TELNET_HANDLE_H
#define TELNET_HANDLE_H

#include <vector>

#include "syscomm.h"

#include "connection.h"
#include "TerminalInfo.h"
#include "TerminalMessage.h"
#include "ProtocolNegotiator.h"
#include "TerminalIOWrapper.h"
#include "InputCharBuffer.h"
#include "TelnetConst.h"
#include "TerminalServer.h"

NS_VCN_OAM_BEGIN(ROUTER)

/*------------------------------------------------------------------------
*                            ��������                                     *
--------------------------------------------------------------------------*/


#define NDF_API
struct ConnIllegalException : public Poco::Exception{
    ConnIllegalException(const std::string &message) : Poco::Exception(message){
    }
};

/*-------------------------------------------------------------------------
*                            ����                                        *
-------------------------------------------------------------------------*/


/*------------------------------------------------------------------------
*                             ������                                      *
------------------------------------------------------------------------*/


class TelnetHandle;

using TelnetTermServer = TermServer<TelnetHandle>;

class TelnetHandle : public Connection<>
{
    friend class TermServer<TelnetHandle>;

    enum TelnetState
    {
        STATE_TELENT_KEEP_STATE = 0,
        STATE_TELNET_INIT,          /**< Telent ��ʼ״̬ */
        STATE_TELNET_LOGIN_USERNAME,/**< Telent ��¼�û�����״̬ */
        STATE_TELNET_LOGIN_PASSWORD,/**< Telent ��¼��������״̬ */
        STATE_TELNET_LOGINING,
        STATE_TELNET_WORKING,       /**< Telent ��¼����̬ */
        STATE_TELNET_EXECUTING,     /**< Telent ����ִ������*/
        STATE_TELNET_NEST_EXECUTING,/**< Telent ����ִ��Ƕ������*/
        STATE_TELNET_SCROLL_LINES   /**< ����������ʾ*/
    };

public:
    TelnetHandle(StreamSocket& socket, SocketReactor& reactor) : Connection<>::Connection(socket, reactor)
    {}

    TelnetHandle(StreamSocket& socket, SocketReactor& reactor, TermServer<TelnetHandle> *pTermServer);

    virtual ~TelnetHandle();

    /**
    * @brief �����ն���Ϣ
    */
    void updateTermInfo( const TerminalInfo& oldInfo, const TerminalInfo& newInfo );

    /**
    * @brief ��ȡ�ն���Ϣ
    */
    const TerminalInfo& getTermInfo() const;

    /**
    * @brief �����ն�����״̬��һ�������ڹر�
    */
    void setConnected(bool state, bool quit = true);

    /**
    * @brief Telnet AYT
    */
    void hereamI();

    /**
    * @brief Telnet NTB
    */
    void nvtBreak();

    /**
    * @brief Telnet дһ���ֽ�
    */
    void rawWrite(Poco::Int16 i);

    /**
    * @brief Telnet ��һ���ֽ�
    */
    Poco::Int16 rawRead();

    /**
    * @brief Telnet ��SOCKET IO,������
    */
    Poco::Int16 read();

    /**
    * @brief Telnet ��ȡһ��INT16��2���ֽڣ�
    */
    Poco::Int16 read16int();

    /**
    * @brief Telnet �����ն˳ߴ�
    */
    void setTerminalSize( Poco::Int16& width, Poco::Int16& height );

    /**
    * @brief Telnet ���� CR LF ��������Ͳ�������������������ΪENTER
    */
    Poco::Int16 processCRLFSeq( Poco::Int16& i );

    /**
    * @brief Telnet ��ȡ�ն˿��
    */
    Poco::Int16 getWidth();

    /**
    * @brief Telnet ��ȡ�ն˸߶�
    */
    Poco::Int16 getRows();

    /**
    * @brief Telnet ����Э��Ŀ��ƴ� �� ��Э�������ַ�
    */
    void handle(Poco::Int16& ch);

    /**< �ر���·���� */
    virtual void closeConnection();

    void check_timeout(std::size_t seconds);

    void clear_line();

protected:
    /**< ��������*/
    void builtin(const std::string &cmd);

    /** read */
    virtual bool readData();

    /**< ��Ϣ�ַ� */
    virtual void dispatchMsg();

    /**< �������� */
    virtual void heartBeat( bool bActive = true );

    virtual void send2terminal(const MessageInfo &data);

    /**
    * @brief Telnet �ı䵱ǰTELNET״̬
    */
    virtual void changeState(TelnetState state = STATE_TELENT_KEEP_STATE);

    inline TelnetState state() const
    {
        return m_telnetState;
    }

    inline std::string getInputStr() const
    {
        return _charBuff.getTrimedCurStr();
    }

    inline std::string getInputNativeStr() const
    {
        return _charBuff.getCurrentStr();
    }

    inline std::size_t getInputSize() const
    {
        return _charBuff.getCurrentStrSize();
    }

    void setPrompt(const std::string &prompt);

    /**
    * @brief Telnet ������ƶ�
    */
    virtual void moveLeft();

    /**
    * @brief Telnet ������ƶ�
    */
    virtual void moveRight();

    /**
    * @brief Telnet ɾ�����ǰ���ߺ�����������ַ�
    */
    virtual void eraseFromCursor(Direction direct);

    /**
    * @brief Telnet ɾ�����ǰ�ĵ���
    */
    virtual void deleteBeforeWord();

    /**
    * @brief Telnet �ƶ���굽����ǰ���ߵ��ʺ�
    */
    void moveWord(Direction direct);

    /**
    * @brief Telnet ɾ���ַ�
    */
    virtual bool deleteCh( const Direction& direct );

    /**
    * @brief Telnet �����ַ�
    */
    virtual void appendCh(const std::string& tempCh);

    /**
    * @brief Telnet �����ַ���
    */
    void appendString(const std::string &str);

    /**
    * @brief Telnet �����ַ�
    */
    virtual void insertCh( const std::string& tempCh );

    /**
    * @brief Telnet ��ȡ��ʷ����
    */
    virtual void historyCmds( const Direction& direct );

protected:
    std::shared_ptr<Telnet_TerminalInfo>  m_term;
    TermServer<TelnetHandle>        *m_pTermServer;

    ProtocolNegotiator               _protocolNegotiator;/**< Telnet Э��Э�̴����� */
    TerminalIOWrapper<TelnetHandle>  _terminalIOWrapper;/**< Telnet �ն�Wrapper    */

    /**
    * @brief ����δ��ɵ�Э������
    */
    Poco::Int32     m_buffer_pos    = -1;
    bool            _noIAC;/**< Telnet �������п�ʼ��־ */
    bool            _lineMode       = false;/**< Telnet �Ƿ���ģʽ */
    TelnetState     m_telnetState   = STATE_TELNET_INIT;/**< Telnet �ն�״̬  */

    /**
    * ��ʽ�����
    */
    std::string     m_userName;/**< Telent �û��� */
    std::string     m_passWord;/**< Telent ���� */
    std::string     m_token;
    Poco::UInt16    m_loginTimes    = 0;/**< Telent ��¼ʧ�ܴ��� */
    Poco::Int16     m_heartbeat     = 0;
    InputCharBuffer _charBuff;/**< Telent �ն��ַ�Buffer */
    Poco::UInt16    _historyIndex   = 0;/**< Telent ��ʷ���� */
    bool            _isCRWithoutLF  = false;

    std::string     m_prompt = TelnetConst::TELNET_PROMPT;
    SessionPtr      m_ctx;
    std::string     m_additional;
    std::size_t     m_timeout_ticks = 0;
    std::size_t     m_output_size   = 0;

    std::string     m_multiple_output;
    std::string     m_echo;

    enum IntelliMode{
        INTELLI_NONE = 0,
        INTELLI_TAB = 1,        //tab��ʾ��
        INTELLI_QUESTION,       //?��ʾ��
        INTELLI_BLANK_QUESTION  //�ո�?��ʾ��
    };
    IntelliMode     m_intelli_mode = INTELLI_NONE;
};

NS_VCN_OAM_END

#endif
