/***@file
* 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
* 文件名称：ProtocolNegotiator.cpp<br>
* 文件标识：<br>
* 内容摘要：<br>
* 其它说明：<br>
* @version v1.0<br>
* @author
* @since 2009-06-04
*/
#include "Poco/NumberFormatter.h"

#include "TelnetHandle.h"
#include "ProtocolNegotiator.h"

NS_VCN_OAM_BEGIN(ROUTER)

ProtocolNegotiator::ProtocolNegotiator( TelnetHandle* handler ): _handler( handler )
{
    CREATE_LOG();
    init();
}

ProtocolNegotiator::~ProtocolNegotiator()
{
}

void ProtocolNegotiator::setHandle( TelnetHandle* handler )
{
    this->_handler = handler;
}

void ProtocolNegotiator::init()
{
    memset( _buffer, 0x0, sizeof( _buffer ) );
    doECHO = false;
    doSUPGA = false;
    doNAWS = false;
    doTTYPE = false;
    doLINEMODE = false;
    doNEWENV = false;
    waitDO_REPLY_SUPGA = false;
    waitDO_REPLY_ECHO = false;
    waitDO_REPLY_NAWS = false;
    waitDO_REPLY_TTYPE = false;
    waitDO_REPLY_LINEMODE = false;
    waitLM_MODE_ACK = false;
    waitLM_DO_REPLY_FORWARDMASK = false;
    waitDO_REPLY_NEWENV = false;
    waitNE_SEND_REPLY = false;
    waitWILL_REPLY_SUPGA = false;
    waitWILL_REPLY_ECHO = false;
    waitWILL_REPLY_NAWS = false;
    waitWILL_REPLY_TTYPE = false;
}

void ProtocolNegotiator::handleC( Poco::Int16& i )
{
    _buffer[0] = i;
    if (!parseTWO( _buffer )){
        _buffer[1] = this->_handler->rawRead();
        parse( _buffer );
    }
    _buffer[0] = 0;
    _buffer[1] = 0;
}

void ProtocolNegotiator::doCharacterModeInit()
{
    sendCommand( WILL, ECHO, true );
    sendCommand( DONT, ECHO, true ); // necessary for some clients
    sendCommand( DO, NAWS, true );
    sendCommand( WILL, SUPGA, true );
    sendCommand( DO, SUPGA, true );
    sendCommand( DO, TTYPE, true );
    sendCommand( DO, NEWENV, true ); // environment variables
}

void ProtocolNegotiator::doLineModeInit()
{
    sendCommand( DO, NAWS, true );
    sendCommand( WILL, SUPGA, true );
    sendCommand( DO, SUPGA, true );
    sendCommand( DO, TTYPE, true );
    sendCommand( DO, LINEMODE, true );
    sendCommand( DO, NEWENV, true );
}

void ProtocolNegotiator::negotiateLineMode()
{
    if ( isEnabled( LINEMODE ) )
    {
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SB );
        this->_handler->rawWrite( LINEMODE );
        this->_handler->rawWrite( LM_MODE );
        this->_handler->rawWrite( LM_EDIT | LM_TRAPSIG );
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SE );
        waitLM_MODE_ACK = true;

        // dont forwardmask
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SB );
        this->_handler->rawWrite( LINEMODE );
        this->_handler->rawWrite( DONT );
        this->_handler->rawWrite( LM_FORWARDMASK );
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SE );
        waitLM_DO_REPLY_FORWARDMASK = true;
    }
}

void ProtocolNegotiator::getTTYPE()
{
    if ( isEnabled( TTYPE ) )
    {
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SB );
        this->_handler->rawWrite( TTYPE );
        this->_handler->rawWrite( SEND );
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SE );
    }
}


bool ProtocolNegotiator::parseTWO( Poco::Int16 ( &buf )[2] )
{
    switch ( buf[0] )
    {
        case IAC:
            // doubled IAC to escape 255 is handled within the
            // read method.
            break;
        case AYT:
            this->_handler->hereamI();
            break;
        case AO:
        case IP:
        case EL:
        case EC:
        case NOP:
            break;
        case BRK:
            this->_handler->nvtBreak();
            break;
        default:
            return false;
    }
    return true;
}

bool ProtocolNegotiator::supported( const Poco::Int16 i )
{
    switch ( i )
    {
        case SUPGA:
        case ECHO:
        case NAWS:
        case TTYPE:
        case NEWENV:
            return true;
        case LINEMODE:
            return true;  //默认只支持行模式
        default:
            return false;
    }
}

bool ProtocolNegotiator::isEnabled( const Poco::Int16 i )
{
    switch ( i )
    {
        case SUPGA:
            return doSUPGA;
        case ECHO:
            return doECHO;
        case NAWS:
            return doNAWS;
        case TTYPE:
            return doTTYPE;
        case LINEMODE:
            return doLINEMODE;
        case NEWENV:
            return doNEWENV;
        default:
            return false;
    }
}

bool ProtocolNegotiator::waitDOreply( const Poco::Int16 i )
{
    switch ( i )
    {
        case SUPGA:
            return waitDO_REPLY_SUPGA;
        case ECHO:
            return waitDO_REPLY_ECHO;
        case NAWS:
            return waitDO_REPLY_NAWS;
        case TTYPE:
            return waitDO_REPLY_TTYPE;
        case LINEMODE:
            return waitDO_REPLY_LINEMODE;
        case NEWENV:
            return waitDO_REPLY_NEWENV;
        default:
            return false;
    }
}

void ProtocolNegotiator::negotiateEnvironment()
{
    if ( isEnabled( NEWENV ) )
    {
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SB );
        this->_handler->rawWrite( NEWENV );
        this->_handler->rawWrite( SEND );
        this->_handler->rawWrite( NE_VAR );
        this->_handler->rawWrite( NE_USERVAR );
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SE );
        waitNE_SEND_REPLY = true;
    }
}


void ProtocolNegotiator::enable( const Poco::Int16 i )
{
    switch ( i )
    {
        case SUPGA:
            if ( doSUPGA )
            {
                doSUPGA = false;
            }
            else
            {
                doSUPGA = true;
            }
            break;
        case ECHO:
            if ( doECHO )
            {
                doECHO = false;
            }
            else
            {
                doECHO = true;
            }
            break;
        case NAWS:
            if ( doNAWS )
            {
                doNAWS = false;
            }
            else
            {
                doNAWS = true;
            }
            break;
        case TTYPE:
            if ( doTTYPE )
            {
                doTTYPE = false;
            }
            else
            {
                doTTYPE = true;
                getTTYPE();
            }
            break;

        case LINEMODE:
            if ( doLINEMODE )
            {
                doLINEMODE = false;
                // @TODO 行模式处理
            }
            else
            {
                doLINEMODE = true;
                negotiateLineMode();
            }
            break;
        case NEWENV:
            if ( doNEWENV )
            {
                doNEWENV = false;
            }
            else
            {
                doNEWENV = true;
                negotiateEnvironment();
            }
            break;
    }

}

void ProtocolNegotiator::setWait( const Poco::Int16 what, const Poco::Int16 option, bool wait )
{
    switch ( what )
    {
        case DO:
            switch ( option )
            {
                case SUPGA:
                    waitDO_REPLY_SUPGA = wait;
                    break;
                case ECHO:
                    waitDO_REPLY_ECHO = wait;
                    break;
                case NAWS:
                    waitDO_REPLY_NAWS = wait;
                    break;
                case TTYPE:
                    waitDO_REPLY_TTYPE = wait;
                    break;
                case LINEMODE:
                    waitDO_REPLY_LINEMODE = wait;
                    break;
                case NEWENV:
                    waitDO_REPLY_NEWENV = wait;
                    break;
            }
            break;
        case WILL:
            switch ( option )
            {
                case SUPGA:
                    waitWILL_REPLY_SUPGA = wait;
                    break;
                case ECHO:
                    waitWILL_REPLY_ECHO = wait;
                    break;
                case NAWS:
                    waitWILL_REPLY_NAWS = wait;
                    break;
                case TTYPE:
                    waitWILL_REPLY_TTYPE = wait;
                    break;
            }
            break;
    }
}

void ProtocolNegotiator::sendCommand( const Poco::Int16 what, const Poco::Int16 option, bool waitReply )
{
    this->_handler->rawWrite( IAC );
    this->_handler->rawWrite( what );
    this->_handler->rawWrite( option );
    //设置等待标志

    if ( ( what == DO ) && waitReply )
        setWait( DO, option, true );

    //设置等待标志
    if ( ( what == WILL ) && waitReply )
        setWait( WILL, option, true );
}

bool ProtocolNegotiator::waitWILLreply( const Poco::Int16 i )
{
    switch ( i )
    {
        case SUPGA:
            return waitWILL_REPLY_SUPGA;
        case ECHO:
            return waitWILL_REPLY_ECHO;
        case NAWS:
            return waitWILL_REPLY_NAWS;
        case TTYPE:
            return waitWILL_REPLY_TTYPE;
        default:
            return false;
    }
}


void ProtocolNegotiator::skipToSE()
{
    while (this->_handler->rawRead() != SE);
}

void ProtocolNegotiator::handleNAWS()
{
    Poco::Int16 width = this->_handler->read16int();
    if ( width == 255 ){
        width = this->_handler->read16int(); // handle doubled 255 value;
    }

    Poco::Int16 height = this->_handler->read16int();
    if ( height == 255 ){
        height = this->_handler->read16int(); // handle doubled 255 value;
    }

    skipToSE();
    this->_handler->setTerminalSize(width, height);
}

Poco::Int16 ProtocolNegotiator::readNEVariableValue( std::string& sbuf )
{
    // check conditions for first character after VALUE
    // @TODO 类型转 处理
    int i = this->_handler->rawRead();
    if (i == -1){
        return NE_IN_ERROR;
    }
    else if (i == IAC){
        i = this->_handler->rawRead();
        if ( i == IAC )
            // Double IAC
            return NE_VAR_DEFINED_EMPTY;
        else if ( i == SE )
            return NE_IN_END;
        else
            // according to rule IAC has to be duplicated
            return NE_IN_ERROR;
    }
    else if (i == NE_VAR || i == NE_USERVAR){
        return NE_VAR_DEFINED_EMPTY;
    }
    else if (i == NE_ESC){
        // escaped value
        i = this->_handler->rawRead();
        if ( i == NE_ESC || i == NE_VAR || i == NE_USERVAR || i == NE_VALUE ){
            sbuf.append(Poco::NumberFormatter::format(i));
        }
        else{
            return NE_IN_ERROR;
        }
    }
    else{
        // character
        sbuf.append(Poco::NumberFormatter::format(i));
    }

    // loop until end of value (IAC SE or TYPE)
    do{
        i = this->_handler->rawRead();
        if ( i == -1 ){
            return NE_IN_ERROR;
        }
        else if ( i == IAC ){
            i = this->_handler->rawRead();
            if ( i == IAC ){
                // duplicated IAC
                sbuf.append(Poco::NumberFormatter::format(i));
            }
            else if ( i == SE ){
                return NE_IN_END;
            }
            else{
                // Error should have been duplicated
                return NE_IN_ERROR;
            }
        }
        else if ( i == NE_ESC ){
            i = this->_handler->rawRead();
            if ( i == NE_ESC || i == NE_VAR || i == NE_USERVAR || i == NE_VALUE )
                sbuf.append(Poco::NumberFormatter::format(i));
            else
                return NE_IN_ERROR;
        }
        else if ( i == NE_VAR || i == NE_USERVAR ){
            return NE_VAR_OK;
        }
        else{
            // check maximum length to prevent overflow
            if ( sbuf.length() > NE_VAR_VALUE_MAXLENGTH )
                // TODO: LOG Overflow
                return NE_IN_ERROR;
            else
                sbuf.append(Poco::NumberFormatter::format(i));
        }
    }
    while ( true );
}

Poco::Int16 ProtocolNegotiator::readNEVariableName( std::string& sbuf )
{
    // @TODO 类型转 处理
    int i = -1;
    do{
        i = this->_handler->rawRead();
        if ( i == -1 ){
            return NE_IN_ERROR;
        }
        else if ( i == IAC ){
            i = this->_handler->rawRead();
            if ( i == IAC )
                // duplicated IAC
                sbuf.append(Poco::NumberFormatter::format(i));
            else if ( i == SE )
                return NE_IN_END;
            else
                // Error should have been duplicated
                return NE_IN_ERROR;
        }
        else if ( i == NE_ESC ){
            i = this->_handler->rawRead();
            if ( i == NE_ESC || i == NE_VAR || i == NE_USERVAR || i == NE_VALUE )
                sbuf.append(Poco::NumberFormatter::format(i));
            else
                return NE_IN_ERROR;
        }
        else if ( i == NE_VAR || i == NE_USERVAR )
        {
            return NE_VAR_UNDEFINED;
        }
        else if ( i == NE_VALUE )
        {
            return NE_VAR_DEFINED;
        }
        else
        {
            // check maximum length to prevent overflow
            if ( sbuf.length() >= NE_VAR_NAME_MAXLENGTH )
                // TODO: Log Overflow
                return NE_IN_ERROR;
            else
                sbuf.append(Poco::NumberFormatter::format(i));
        }
    }while ( true );
}

void ProtocolNegotiator::readNEVariables()
{
    std::string sbuf;
    int i = this->_handler->rawRead();
    if ( i == IAC )
    {
        // invalid or empty response
        skipToSE();
        return;
    }

    bool cont = true;
    if ( i == NE_VAR || i == NE_USERVAR ){
        do{
            switch ( readNEVariableName( sbuf ) )
            {
                case NE_IN_ERROR:
                    return;
                case NE_IN_END:
                    return;
                case NE_VAR_DEFINED:
                {
                    std::string str = sbuf;
                    sbuf.erase();
                    switch ( readNEVariableValue( sbuf ) )
                    {
                        case NE_IN_ERROR:
                            logDebug( "readNEVariables()::NE_IN_ERROR" );
                            return;

                        case NE_IN_END:
                            logDebug( "readNEVariables()::NE_IN_END" );
                            return;

                        case NE_VAR_DEFINED_EMPTY:
                            logDebug( "readNEVariables()::NE_VAR_DEFINED_EMPTY" );
                            break;

                        case NE_VAR_OK:
                            // add variable
                            logDebug( "readNEVariables()::NE_VAR_OK:VAR=%s, VAL=%s.", str.c_str(), sbuf.c_str() );
                            // @TODO 记录环境变量
                            sbuf.erase();
                            break;
                    }
                    break;
                }
                case NE_VAR_UNDEFINED:
                    logDebug( "readNEVariables()::NE_VAR_UNDEFINED" );
                    break;
            }
        }while ( cont );
    }
}

void ProtocolNegotiator::handleNEIs()
{
    if ( isEnabled( NEWENV ) )
    {
        readNEVariables();
    }
}

void ProtocolNegotiator::handleNEInfo()
{
    if ( isEnabled( NEWENV ) )
    {
        readNEVariables();
    }
}

void ProtocolNegotiator::handleNEWENV()
{
    int c = this->_handler->rawRead();
    switch ( c )
    {
        case IS:
            handleNEIs();
            break;
        case NE_INFO:
            handleNEInfo();
            break;
        default:
            // skip to (including) SE
            skipToSE();
    }
}

void ProtocolNegotiator::handleLMForwardMask( Poco::Int16& what )
{
    switch ( what )
    {
        case WONT:
            if ( waitLM_DO_REPLY_FORWARDMASK )
            {
                waitLM_DO_REPLY_FORWARDMASK = false;
            }
            break;
    }

    skipToSE();
}

bool ProtocolNegotiator::readTriple( Poco::Int16 ( & triple )[3] )
{
    triple[0] = this->_handler->rawRead();
    triple[1] = this->_handler->rawRead();
    if ( ( triple[0] == IAC ) && ( triple[1] == SE ) )
    {
        return false;
    }
    else
    {
        triple[2] = this->_handler->rawRead();
        return true;
    }
}

void ProtocolNegotiator::handleLMSLC()
{
    Poco::Int16 triple[3];
    memset( triple, 0x0, sizeof( triple ) );
    if ( !readTriple( triple ) )
        return;

    // SLC will be initiated by the client
    // case 1. client requests set
    // LINEMODE SLC 0 SLC_DEFAULT 0
    if ( ( triple[0] == 0 ) && ( triple[1] == LM_SLC_DEFAULT ) && ( triple[2] == 0 ) )
    {
        skipToSE();
        // reply with SLC xxx SLC_DEFAULT 0
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SB );
        this->_handler->rawWrite( LINEMODE );
        this->_handler->rawWrite( LM_SLC );
        // triples defaults for all

        for ( int i = 1; i < 12; i++ )
        {
            this->_handler->rawWrite( i );
            this->_handler->rawWrite( LM_SLC_DEFAULT );
            this->_handler->rawWrite( 0 );
        }

        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SE );
    }
    else
    {
        // case 2: just acknowledge anything we get from the client
        this->_handler->rawWrite( IAC );
        this->_handler->rawWrite( SB );
        this->_handler->rawWrite( LINEMODE );
        this->_handler->rawWrite( LM_SLC );
        this->_handler->rawWrite( triple[0] );
        this->_handler->rawWrite( triple[1] | LM_SLC_ACK );
        this->_handler->rawWrite( triple[2] );

        while ( readTriple( triple ) )
        {
            this->_handler->rawWrite( triple[0] );
            this->_handler->rawWrite( triple[1] | LM_SLC_ACK );
            this->_handler->rawWrite( triple[2] );
        }

        this->_handler->rawWrite( IAC );

        this->_handler->rawWrite( SE );
    }
}

void ProtocolNegotiator::handleLMMode()
{
    if ( waitLM_MODE_ACK )
    {
        Poco::Int16 mask = this->_handler->rawRead();

        if ( mask != ( LM_EDIT | LM_TRAPSIG | LM_MODEACK ) )
        {
            logDebug( "Client violates linemode ack sent: %d ", mask );
        }

        waitLM_MODE_ACK = false;
    }

    skipToSE();
}

void ProtocolNegotiator::handleLINEMODE()
{
    Poco::Int16 c = this->_handler->rawRead();

    switch ( c )
    {
        case LM_MODE:
            handleLMMode();
            break;
        case LM_SLC:
            handleLMSLC();
            break;
        case WONT:
        case WILL:
            handleLMForwardMask( c );
            break;
        default:
            // skip to (including) SE
            skipToSE();
    }
}

void ProtocolNegotiator::readIACSETerminatedString(const Poco::UInt16 maxlength, std::string& sbuf)
{
    bool cont = true;
    sbuf.erase();
    do{
        Poco::Int16 i;
        i = this->_handler->rawRead();
        switch (i){
            case IAC:
                i = this->_handler->rawRead();
                if (i == SE)
                    cont = false;
                break;
            case -1:
                sbuf.append("default");
                return;
            default:
                ;
        }

        if (cont){
            //std::string sChar =  Util::toStr<Poco::Int16>( i );
            if (i == TelnetConst::LF || i == TelnetConst::CR || sbuf.length() == maxlength)
                cont = false;
            else
                sbuf.append((const char*)&i, 1);
        }
    }while(cont);
}

void ProtocolNegotiator::handleTTYPE()
{
    std::string tmpstr = "";
    // 下一个字符为：IS根据协议，读取丢弃
    this->_handler->rawRead();
    readIACSETerminatedString(40, tmpstr);
    logDebug("terminal type: %s", tmpstr.c_str());
}

void ProtocolNegotiator::parse( Poco::Int16 ( &buf )[2] )
{
    switch ( buf[0] )
    {
        /**
        *   协商选项
        */
        case WILL:
            if ( supported( buf[1] ) && isEnabled( buf[1] ) )
            {
                ;// do nothing
            }
            else
            {
                if ( waitDOreply( buf[1] ) && supported( buf[1] ) )
                {
                    enable( buf[1] );
                    setWait( DO, buf[1], false );
                }
                else
                {
                    if ( supported( buf[1] ) )
                    {
                        sendCommand( DO, buf[1], false );
                        enable( buf[1] );
                    }
                    else
                    {
                        sendCommand( DONT, buf[1], false );
                    }
                }
            }
            break;
        case WONT:
            if ( waitDOreply( buf[1] ) && supported( buf[1] ) )
            {
                setWait( DO, buf[1], false );
            }
            else
            {
                if ( supported( buf[1] ) && isEnabled( buf[1] ) )
                {
                    enable( buf[1] );
                }
            }
            break;
        case DO:
            if ( supported( buf[1] ) && isEnabled( buf[1] ) )
            {
                ; // do nothing
            }
            else
            {
                if ( waitWILLreply( buf[1] ) && supported( buf[1] ) )
                {
                    enable( buf[1] );
                    setWait( WILL, buf[1], false );
                }
                else
                {
                    if ( supported( buf[1] ) )
                    {
                        sendCommand( WILL, buf[1], false );
                        enable( buf[1] );
                    }
                    else
                    {
                        sendCommand( WONT, buf[1], false );
                    }
                }
            }
            break;
        case DONT:
            if ( waitWILLreply( buf[1] ) && supported( buf[1] ) )
            {
                setWait( WILL, buf[1], false );
            }
            else
            {
                if ( supported( buf[1] ) && isEnabled( buf[1] ) )
                {
                    enable( buf[1] );
                }
            }
            break;
        case DM:
            break;
        case SB: // 子协商选项
            if ((supported(buf[1])) && (isEnabled(buf[1]))){
                switch ( buf[1] )
                {
                    case NAWS:
                        handleNAWS();
                        break;
                    case TTYPE:
                        handleTTYPE();
                        break;
                    case LINEMODE:
                        handleLINEMODE();
                        break;
                    case NEWENV:
                        handleNEWENV();
                        break;
                    default:
                        ;// do nothing;
                }
            }
            else
            {
                // do nothing
            }
            break;
        default:
            ;// do nothing;
    }
}


NS_VCN_OAM_END
