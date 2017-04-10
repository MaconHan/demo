/**@file
* 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
* 文件名称：TelnetHandle.h<br>
* 文件标识：
* 内容摘要：SSS 2.0 NDF通信服务头文件<br>
* 其它说明：无<br>
* @version 1.0<br>
* @author <br>
* @since 2009年5月12日<br>
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
*                            数据类型                                     *
--------------------------------------------------------------------------*/


#define NDF_API
struct ConnIllegalException : public Poco::Exception{
    ConnIllegalException(const std::string &message) : Poco::Exception(message){
    }
};

/*-------------------------------------------------------------------------
*                            常量                                        *
-------------------------------------------------------------------------*/


/*------------------------------------------------------------------------
*                             类声明                                      *
------------------------------------------------------------------------*/


class TelnetHandle;

using TelnetTermServer = TermServer<TelnetHandle>;

class TelnetHandle : public Connection<>
{
    friend class TermServer<TelnetHandle>;

    enum TelnetState
    {
        STATE_TELENT_KEEP_STATE = 0,
        STATE_TELNET_INIT,          /**< Telent 初始状态 */
        STATE_TELNET_LOGIN_USERNAME,/**< Telent 登录用户输入状态 */
        STATE_TELNET_LOGIN_PASSWORD,/**< Telent 登录密码输入状态 */
        STATE_TELNET_LOGINING,
        STATE_TELNET_WORKING,       /**< Telent 登录后工作态 */
        STATE_TELNET_EXECUTING,     /**< Telent 正在执行命令*/
        STATE_TELNET_NEST_EXECUTING,/**< Telent 正在执行嵌套命令*/
        STATE_TELNET_SCROLL_LINES   /**< 交互分屏显示*/
    };

public:
    TelnetHandle(StreamSocket& socket, SocketReactor& reactor) : Connection<>::Connection(socket, reactor)
    {}

    TelnetHandle(StreamSocket& socket, SocketReactor& reactor, TermServer<TelnetHandle> *pTermServer);

    virtual ~TelnetHandle();

    /**
    * @brief 更新终端信息
    */
    void updateTermInfo( const TerminalInfo& oldInfo, const TerminalInfo& newInfo );

    /**
    * @brief 获取终端信息
    */
    const TerminalInfo& getTermInfo() const;

    /**
    * @brief 设置终端连接状态，一般是用于关闭
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
    * @brief Telnet 写一个字节
    */
    void rawWrite(Poco::Int16 i);

    /**
    * @brief Telnet 读一个字节
    */
    Poco::Int16 rawRead();

    /**
    * @brief Telnet 读SOCKET IO,并处理
    */
    Poco::Int16 read();

    /**
    * @brief Telnet 读取一个INT16（2个字节）
    */
    Poco::Int16 read16int();

    /**
    * @brief Telnet 设置终端尺寸
    */
    void setTerminalSize( Poco::Int16& width, Poco::Int16& height );

    /**
    * @brief Telnet 处理 CR LF 连续输入和不连续输入的情况，都视为ENTER
    */
    Poco::Int16 processCRLFSeq( Poco::Int16& i );

    /**
    * @brief Telnet 获取终端宽度
    */
    Poco::Int16 getWidth();

    /**
    * @brief Telnet 获取终端高度
    */
    Poco::Int16 getRows();

    /**
    * @brief Telnet 处理协议的控制串 或 非协议输入字符
    */
    void handle(Poco::Int16& ch);

    /**< 关闭链路处理 */
    virtual void closeConnection();

    void check_timeout(std::size_t seconds);

    void clear_line();

protected:
    /**< 内置命令*/
    void builtin(const std::string &cmd);

    /** read */
    virtual bool readData();

    /**< 消息分发 */
    virtual void dispatchMsg();

    /**< 心跳处理 */
    virtual void heartBeat( bool bActive = true );

    virtual void send2terminal(const MessageInfo &data);

    /**
    * @brief Telnet 改变当前TELNET状态
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
    * @brief Telnet 光标左移动
    */
    virtual void moveLeft();

    /**
    * @brief Telnet 光标右移动
    */
    virtual void moveRight();

    /**
    * @brief Telnet 删除光标前或者后的所有输入字符
    */
    virtual void eraseFromCursor(Direction direct);

    /**
    * @brief Telnet 删除光标前的单词
    */
    virtual void deleteBeforeWord();

    /**
    * @brief Telnet 移动光标到单词前或者单词后
    */
    void moveWord(Direction direct);

    /**
    * @brief Telnet 删除字符
    */
    virtual bool deleteCh( const Direction& direct );

    /**
    * @brief Telnet 附加字符
    */
    virtual void appendCh(const std::string& tempCh);

    /**
    * @brief Telnet 附加字符串
    */
    void appendString(const std::string &str);

    /**
    * @brief Telnet 插入字符
    */
    virtual void insertCh( const std::string& tempCh );

    /**
    * @brief Telnet 获取历史命令
    */
    virtual void historyCmds( const Direction& direct );

protected:
    std::shared_ptr<Telnet_TerminalInfo>  m_term;
    TermServer<TelnetHandle>        *m_pTermServer;

    ProtocolNegotiator               _protocolNegotiator;/**< Telnet 协议协商处理类 */
    TerminalIOWrapper<TelnetHandle>  _terminalIOWrapper;/**< Telnet 终端Wrapper    */

    /**
    * @brief 缓存未完成的协议数据
    */
    Poco::Int32     m_buffer_pos    = -1;
    bool            _noIAC;/**< Telnet 控制序列开始标志 */
    bool            _lineMode       = false;/**< Telnet 是否行模式 */
    TelnetState     m_telnetState   = STATE_TELNET_INIT;/**< Telnet 终端状态  */

    /**
    * 格式化输出
    */
    std::string     m_userName;/**< Telent 用户名 */
    std::string     m_passWord;/**< Telent 密码 */
    std::string     m_token;
    Poco::UInt16    m_loginTimes    = 0;/**< Telent 登录失败次数 */
    Poco::Int16     m_heartbeat     = 0;
    InputCharBuffer _charBuff;/**< Telent 终端字符Buffer */
    Poco::UInt16    _historyIndex   = 0;/**< Telent 历史命令 */
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
        INTELLI_TAB = 1,        //tab提示符
        INTELLI_QUESTION,       //?提示符
        INTELLI_BLANK_QUESTION  //空格?提示符
    };
    IntelliMode     m_intelli_mode = INTELLI_NONE;
};

NS_VCN_OAM_END

#endif
