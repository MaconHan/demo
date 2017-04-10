/**@file
* 版权所有(C)2001, 深圳市中兴通讯股份有限公司<br>
* 文件名称：ProtocolNegotiator.h<br>
* 内容摘要：<br>
* 其它说明：无
* @version 1.0
* @author
* @since 2009-06-04
*/

#ifndef PROTOCOL_NEGOTIATOR_H
#define PROTOCOL_NEGOTIATOR_H

/* ======================== 引用 ============================ */
#include <string>

#include "Poco/Types.h"
#include "Poco/Mutex.h"

#include "vcn_defs.h"
/* ======================== 命名空间 ========================= */

NS_VCN_OAM_BEGIN(ROUTER)

class TelnetHandle;

class ProtocolNegotiator
{

public:
    // 窗口大小设置
    static const Poco::UInt16 SMALLEST_BELIEVABLE_WIDTH = 20; /**<.*/
    static const Poco::UInt16 SMALLEST_BELIEVABLE_HEIGHT = 6;/**<.*/
    static const Poco::UInt16 DEFAULT_WIDTH = 80;/**<.*/
    static const Poco::UInt16 DEFAULT_HEIGHT = 25;/**<.*/
    // 协议常量定义
    static const Poco::Int16 IAC = 255;/**<当作命令来解释(Interpret as Command)(IAC).*/
    static const Poco::Int16 GA = 249;/**<Go Ahead .*/
    static const Poco::Int16 WILL = 251;/**<协商: 接受某选项.*/
    static const Poco::Int16 WONT = 252;/**<协商: 不接受某选项.*/
    static const Poco::Int16 DO = 253;/**<协商: 请开启某选项.*/
    static const Poco::Int16 DONT = 254;/**<协商: 请不开启某选项.*/
    static const Poco::Int16 SB = 250;/**<子选项开始标记.*/
    static const Poco::Int16 SE = 240;/**<子选项结束标记.*/
    static const Poco::Int16 NOP = 241;/**<No operation  暂不处理.*/
    static const Poco::Int16 DM = 242;/**<？？ 暂不处理.*/
    static const Poco::Int16 BRK = 243;/**<终端中断.*/
    static const Poco::Int16 IP = 244;/**<Interrupt Process 中断进程.*/
    static const Poco::Int16 AO = 245;/**< 放弃输出（Abort Output）.*/
    static const Poco::Int16 AYT = 246;/**< Are You There.*/
    static const Poco::Int16 EC = 247;/**<删除字符.*/
    static const Poco::Int16 EL = 248;/**<删除行.*/
    //
    static const Poco::Int16 ECHO = 1;/**<回显.*/
    static const Poco::Int16 SUPGA = 3;/**<.*/
    static const Poco::Int16 NAWS = 31;/**<.*/
    static const Poco::Int16 TTYPE = 24;/**<.*/
    static const Poco::Int16 IS = 0;/**<.*/
    static const Poco::Int16 SEND = 1;/**<.*/
    static const Poco::Int16 LOGOUT = 18;/**<.*/
    static const Poco::Int16 LINEMODE = 34;/**<.*/
    static const Poco::Int16 LM_MODE = 1;/**<.*/
    static const Poco::Int16 LM_EDIT = 1;/**<.*/
    static const Poco::Int16 LM_TRAPSIG = 2;/**<.*/
    static const Poco::Int16 LM_MODEACK = 4;/**<.*/
    static const Poco::Int16 LM_FORWARDMASK = 2;/**<.*/
    static const Poco::Int16 LM_SLC = 3;/**<.*/
    static const Poco::Int16 LM_SLC_NOSUPPORT = 0;/**<.*/
    static const Poco::Int16 LM_SLC_DEFAULT = 3;/**<.*/
    static const Poco::Int16 LM_SLC_VALUE = 2;/**<.*/
    static const Poco::Int16 LM_SLC_CANTCHANGE = 1;/**<.*/
    static const Poco::Int16 LM_SLC_LEVELBITS = 3;/**<.*/
    static const Poco::Int16 LM_SLC_ACK = 128;/**<.*/
    static const Poco::Int16 LM_SLC_FLUSHIN = 64;/**<.*/
    static const Poco::Int16 LM_SLC_FLUSHOUT = 32;/**<.*/
    static const Poco::Int16 LM_SLC_SYNCH = 1;/**<.*/
    static const Poco::Int16 LM_SLC_BRK = 2;/**<.*/
    static const Poco::Int16 LM_SLC_IP = 3;/**<.*/
    static const Poco::Int16 LM_SLC_AO = 4;/**<.*/
    static const Poco::Int16 LM_SLC_AYT = 5;/**<.*/
    static const Poco::Int16 LM_SLC_EOR = 6;/**<.*/
    static const Poco::Int16 LM_SLC_ABORT = 7;/**<.*/
    static const Poco::Int16 LM_SLC_EOF = 8;/**<.*/
    static const Poco::Int16 LM_SLC_SUSP = 9;/**<.*/
    static const Poco::Int16 NEWENV = 39;/**<.*/
    static const Poco::Int16 NE_INFO = 2;/**<.*/
    static const Poco::Int16 NE_VAR = 0;/**<.*/
    static const Poco::Int16 NE_VALUE = 1;/**<.*/
    static const Poco::Int16 NE_ESC = 2;/**<.*/
    static const Poco::Int16 NE_USERVAR = 3;/**<.*/
    static const Poco::Int16 NE_VAR_OK = 2;/**<.*/
    static const Poco::Int16 NE_VAR_DEFINED = 1;/**<.*/
    static const Poco::Int16 NE_VAR_DEFINED_EMPTY = 0;/**<.*/
    static const Poco::Int16 NE_VAR_UNDEFINED = -1;/**<.*/
    static const Poco::Int16 NE_IN_ERROR = -2;/**<.*/
    static const Poco::Int16 NE_IN_END = -3;/**<.*/
    static const Poco::Int16 NE_VAR_NAME_MAXLENGTH = 50;/**<.*/
    static const Poco::Int16 NE_VAR_VALUE_MAXLENGTH = 1000;/**<.*/

public:
    ProtocolNegotiator( TelnetHandle* handler = NULL );
    virtual ~ProtocolNegotiator();

    void setHandle( TelnetHandle* handler );

    void init();

    void doCharacterModeInit();
    void doLineModeInit();
    void handleC( Poco::Int16& i );

protected:
    // 两个子字节的协议选项
    Poco::Int16 _buffer[2];

    /**
    * doECHO or not
    */
    bool doECHO;

    /**
    * doSUPGA or not
    */
    bool doSUPGA;

    /**
    * doNAWS or not
    */
    bool doNAWS;

    /**
    * doTTYPE or not
    */
    bool doTTYPE;

    /**
    * doLINEMODE or not
    */
    bool doLINEMODE;

    /**
    * doNEWENV or not
    */
    bool doNEWENV;

    /**
    * Are we waiting for a DO reply?
    */
    bool waitDO_REPLY_SUPGA;
    bool waitDO_REPLY_ECHO;
    bool waitDO_REPLY_NAWS;
    bool waitDO_REPLY_TTYPE;
    bool waitDO_REPLY_LINEMODE;
    bool waitLM_MODE_ACK;
    bool waitLM_DO_REPLY_FORWARDMASK;
    bool waitDO_REPLY_NEWENV;
    bool waitNE_SEND_REPLY;

    /**
    * Are we waiting for a WILL reply?
    */
    bool waitWILL_REPLY_SUPGA;
    bool waitWILL_REPLY_ECHO;
    bool waitWILL_REPLY_NAWS;
    bool waitWILL_REPLY_TTYPE;

private:

    bool parseTWO( Poco::Int16 ( &buf )[2] );
    void parse( Poco::Int16 ( &buf )[2] );
    void handleNAWS();
    void handleTTYPE();
    Poco::Int16 readNEVariableName( std::string& sbuf );
    Poco::Int16 readNEVariableValue( std::string& sbuf );
    void negotiateEnvironment();
    void skipToSE();
    bool readTriple( Poco::Int16 ( &triple )[3] );
    void readIACSETerminatedString(const Poco::UInt16 maxlength, std::string& sbuf);
    bool supported( const Poco::Int16 i );
    void sendCommand( const Poco::Int16 what, const Poco::Int16 option, bool waitReply );
    void enable( const Poco::Int16 i );
    bool isEnabled( const Poco::Int16 i );
    bool waitWILLreply( const Poco::Int16 i );
    bool waitDOreply( const Poco::Int16 i );
    void setWait( const Poco::Int16 what, const Poco::Int16 option, bool wait );
    void handleNEWENV();
    void handleLINEMODE();
    void handleLMMode();
    void handleLMSLC();
    void handleLMForwardMask( Poco::Int16& what );
    void readNEVariables();
    void handleNEIs();
    void handleNEInfo();
    void getTTYPE();
    void negotiateLineMode();

    Poco::Mutex   _mutex ;

    DECLARE_LOG();

    TelnetHandle* _handler;
};

NS_VCN_OAM_END
#endif
