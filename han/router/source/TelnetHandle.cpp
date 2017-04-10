/**@file
* 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
* 文件名称：TelnetHandle.cpp<br>
* 文件标识：<br>
* 内容摘要：<br>
* 其它说明：<br>
* @version v1.0<br>
* @author
* @since 2009-06-04
*/
#include <iostream>
#include <functional>

#include "Poco/String.h"
#include "Poco/Format.h"

#include "TelnetHandle.h"
#include "TerminalServer.h"


NS_VCN_OAM_BEGIN(ROUTER)

/**
* @details
* 函数名称：TelnetHandle<br>
* 功能描述：构造<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
*/
TelnetHandle::TelnetHandle(StreamSocket &socket, SocketReactor &reactor, TermServer<TelnetHandle> *pTermServer)
        : Connection<>(socket, reactor, "TelnetHandle"),
        m_pTermServer(pTermServer),
        _protocolNegotiator(this),
        _terminalIOWrapper(this),
        _charBuff(80, 25, "")
{
    m_tSocket.setBlocking(true);// 修改为阻塞模式
    m_tInBuffer.setByteOrder(1);//网络字节序？

    try{
        m_term = std::shared_ptr<Telnet_TerminalInfo>(new Telnet_TerminalInfo(m_pTermServer->prop()));
        m_term->type       = SOCKET_TYPE_TELNET;
        m_term->socket_fd  = m_tSocket.impl()->sockfd();
        m_term->local_addr = m_tSocket.address().toString();
        m_term->peer_addr  = m_tSocket.peerAddress().toString();

        logDebug("start a telnet(%s) connection, peer address:%s.",
              m_term->prop.Type.c_str(),
              m_term->peer_addr.c_str());
        pTermServer->registryConn(this);

        _terminalIOWrapper.setDefaultTerminal();
        m_bState = OMS_TCP_WORKING;

        if (_lineMode) // @TODO 只支字符模式
            _protocolNegotiator.doLineModeInit();
        else
            _protocolNegotiator.doCharacterModeInit();

        _terminalIOWrapper.eraseScreen();
        _terminalIOWrapper.moveCursor(TelnetConst::UP, 100);

        changeState();
        changeState(STATE_TELNET_LOGIN_USERNAME);
    }
    catch(Poco::Exception &e){
        std::string error = e.displayText();
        try{
            socket.sendBytes(error.c_str(), error.length());
        }
        catch(...){
        }
        setConnected(false);
    }
    catch(...){
        setConnected(false);
    }
}

TelnetHandle::~TelnetHandle()
{
    try{
        if (m_token.size())
            m_pTermServer->dispatch(LogoutRequest(m_token));
        m_token.clear();
    }
    catch (...){
        logError(" >>>>> In ~TelnetHandle() catch unknown exception");
    }

    if (m_term){
        logDebug("close a telnet(%s) connection, peer address:%s.",
                 m_term->prop.Type.c_str(),
                 m_term->peer_addr.c_str());
    }
}

/**< 关闭链路处理 */
void TelnetHandle::closeConnection()
{
    if (m_term){
        logDebug("unregistry the telnet connection, peer address:%s.", m_term->peer_addr.c_str());
        m_pTermServer->unregistryConn(this);
        m_term = nullptr;
    }
}

void TelnetHandle::setConnected(bool state, bool quit)
{
    m_bConnected = state;
    if (!m_bConnected && quit){
        closeConnection();
    }
}

const TerminalInfo& TelnetHandle::getTermInfo() const
{
    return *m_term;
}

Poco::Int16 TelnetHandle::getWidth()
{
    return _charBuff.getWidth();
}

Poco::Int16 TelnetHandle::getRows()
{
    return _charBuff.getRows();
}

void TelnetHandle::hereamI()
{
    std::string iamHere;
    iamHere.append( "[" ).append( m_tSocket.address().host().toString() ).append( ":Yes" );
    this->sendBytes( ( LPBYTE )iamHere.c_str(), iamHere.size() );
}

void TelnetHandle::nvtBreak()
{
    // @TODO
    logDebug( "///Network virtual terminal break." );
}

void TelnetHandle::rawWrite(Poco::Int16 i)
{
    // @CRLF 在linux上一定要顺序出现，这个由应用保证。这里不处理
    Poco::UInt8 ch;
    if (!_isCRWithoutLF && i == TelnetConst::LF){
        ch = TelnetConst::CR;
        this->sendBytes((LPBYTE)&ch, sizeof(ch));
    }
    else if (_isCRWithoutLF && i != TelnetConst::CR){
        ch = TelnetConst::LF;
        this->sendBytes((LPBYTE)&ch, sizeof(ch));
    }

    this->sendBytes((LPBYTE)&i, 1);
    this->flush();

    _isCRWithoutLF = (i == TelnetConst::LF);
}

Poco::Int16 TelnetHandle::read16int()
{
    // 读取 UInt16
    Poco::UInt16 temp;
    m_tInBuffer >> temp;
    return temp;
}

Poco::Int16 TelnetHandle::read()
{
    Poco::Int16 c;
    while ((c = rawRead()) == ProtocolNegotiator::IAC){
        /**
        * TELNET下发是一个字节一个字节下发，
        * 异步方式，在读取协议下个字符时可能出现错误，需要缓存之前读取的，
        * 保证下次处理时是完成的协议序列 类似其他链路上的半包情况。
        * 这种情况应比较少，只是一种保护，理论上不应该出现。
        */
        m_buffer_pos = m_tInBuffer.getReadPosition();
        if ((c = rawRead()) != ProtocolNegotiator::IAC){
            _protocolNegotiator.handleC(c);

            m_buffer_pos = -1;
        }
        else{
            break;
        }
    }

    m_buffer_pos = -1;
    return processCRLFSeq(c);
}

/**
* @details
* 函数名称：setTerminalGeometry<br>
* 功能描述：Telent 终端尺寸协商 注意字节序问题<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param width 宽度
* @param height 高度
*/
void TelnetHandle::setTerminalSize(Poco::Int16& width, Poco::Int16& height)
{
    Poco::UInt16 tempWidth  = width;
    Poco::UInt16 tempHeight = height;
    tempWidth   = swap<Poco::UInt16>( tempWidth );
    tempHeight  = swap<Poco::UInt16>( tempHeight );
    if (width < 0 || (width > 0 && width > tempHeight && tempHeight > 0)){
        width   = tempWidth;
        height  = tempHeight;
    }

    logDebug("change terminal size from (%hd,%hd) to (%hu,%hu)",
             _charBuff.getWidth(),
             _charBuff.getRows(),
             width,
             height);
    _charBuff.chgTermSize(width, height);
}

/**
* @details
* 函数名称：rawRead<br>
* 功能描述：读取一个UInt8<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return Poco::Int16 一个字节存储在Int16
*/
Poco::Int16 TelnetHandle::rawRead()
{
    Poco::UInt8 temp = 0;
    m_tInBuffer >> temp;
    return temp;
}

/*------------------------------------------------------------------------
*                          类TelnetHandle实现--保护部分           *
------------------------------------------------------------------------*/
bool TelnetHandle::readData()
{
    std::size_t n = m_tSocket.available();
    if (n > m_tInBuffer.availabeSize()){
        logDebug("ready to read %lu data, but buffer's(%d) availabe is %u", n, m_tInBuffer.size(), m_tInBuffer.availabeSize());
        if (n < m_tInBuffer.size()){
            m_tInBuffer.reset(); // 丢弃前面的所有数据，继续处理后续的数据
        }
        else{
            return false; //超过最大长度了
        }
    }

    n = m_tSocket.receiveBytes(m_tInBuffer.getBuffer(), n);
    if (n == 0)
        throw Poco::Net::NetException();

    m_tInBuffer.move(n);
    return true;
}

/**< 消息分发 */
void TelnetHandle::dispatchMsg()
{
    Poco::ScopedLock<Poco::FastMutex> lock(m_tMutex);

    //TODO处理消息
    Poco::Int16 ch = 0;
    while(m_tInBuffer.getLen() > 0){
        try{
            ch = this->_terminalIOWrapper.read();
        }
        catch(...){
            //缓存协议
            if (m_buffer_pos >= 0)
                m_tInBuffer.setReadPosition(m_buffer_pos);
            else
                m_tInBuffer.reset();
            break;
        }

        {
            handle(ch);
            (*this) << flush_out;
        }
    }
}

/**
* @details
* 函数名称：handle<br>
* 功能描述：Telnet 处理 read 转换后的内部控制字符 或 过滤的非控制字符<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param Poco::Int16 转换后的内部控制字符 或 过滤的非控制字符
*/
void TelnetHandle::handle(Poco::Int16& ch)
{
    if (!m_bConnected)
        return;

    TelnetState st = state();
    //logDebug("ch:%hu state:%hu", ch, st);
    //中断登录, Ctrl+D
    if (ch == TelnetConst::LOGOUT){
        clear_line();
        if (m_token.size())
            m_pTermServer->dispatch(LogoutRequest(m_token));
        m_token.clear();
        this->setConnected(false);
        return;
    }
    //中断当前执行的命令, Ctrl+C
    else if (ch == TelnetConst::STOPCMD){
        if (m_token.empty()){
            this->setConnected(false);
            return;
        }

        switch(st){
        case STATE_TELNET_SCROLL_LINES:
            clear_line();
        case STATE_TELNET_EXECUTING:
        case STATE_TELNET_NEST_EXECUTING:
            {
                if (m_ctx)
                    m_pTermServer->dispatch(BreakCommandRequest(m_ctx->sequence(), m_token));
                m_ctx.reset();

                changeState(STATE_TELNET_WORKING);
                return;
            }
        }
    }

    //执行状态禁止输入
    if (st == STATE_TELNET_EXECUTING){
        return;
    }
    //嵌套命令(危险命名)
    else if (st == STATE_TELNET_NEST_EXECUTING){
        switch(ch){
        case TelnetConst::UP:
        case TelnetConst::DOWN:
            break;
        }
    }
    //多行显示。多包显示
    else if (st == STATE_TELNET_SCROLL_LINES){
        Poco::UInt16 screen_columns = getWidth(), screen_rows = getRows();
        //空格显示满屏
        if (ch == ' '){

        }
        //回车显示一行
        else if (ch == TelnetConst::ENTER_KEY){
            screen_rows = 1;
        }
        //其它键, 则退出
        else{
            if (m_ctx)
                m_pTermServer->dispatch(BreakCommandRequest(m_ctx->sequence(), m_token));
            m_ctx.reset();

            clear_line();
            changeState(STATE_TELNET_WORKING);
            return;
        }

        bool fullscreen = false, isdisplay = false;
        if (!m_ctx || (fullscreen = TelnetConst::screen_full(screen_columns, screen_rows, m_multiple_output))){
            clear_line();

            std::string output = TelnetConst::screen_getlines(screen_columns, screen_rows, m_multiple_output);
            (*this) << output << flush_out;
            m_output_size += output.size();

            fullscreen  = TelnetConst::screen_full(screen_columns, getRows(), m_multiple_output);
            isdisplay   = true;
        }

        if (m_ctx && !fullscreen && m_intelli_mode == INTELLI_NONE){
            if (!isdisplay)
                changeState(STATE_TELNET_EXECUTING);
            m_pTermServer->dispatch(MultiPacketsRequest(m_ctx->sequence(), m_token));
        }
        else if (!m_ctx && m_multiple_output.empty()){
            changeState(STATE_TELNET_WORKING);
        }
        return;
    }

    switch (ch)
    {
        case TelnetConst::LEFT:
        {
            moveLeft();
            break;
        }
        case TelnetConst::RIGHT:
        {
            moveRight();
            break;
        }
        case TelnetConst::UP:
        {
            if (st == STATE_TELNET_WORKING)
                historyCmds(UP);
            break;
        }
        case TelnetConst::DOWN:
        {
            if (st == STATE_TELNET_WORKING)
                historyCmds(DOWN);
            break;
        }
        case TelnetConst::DELETE:
        {
            deleteCh(RIGHT);
            break;
        }
        case TelnetConst::BACKSPACE:
        {
            deleteCh(LEFT);
            break;
        }
        case TelnetConst::DEL_AC:
        {
            eraseFromCursor(RIGHT);
            break;
        }
        case TelnetConst::DEL_BC:
        {
            eraseFromCursor(LEFT);
            break;
        }
        case TelnetConst::DEL_BW:
        {
            deleteBeforeWord();
            break;
        }
        case TelnetConst::HOME:
        {
            while(_charBuff.getOneCh(LEFT).size()) moveLeft();
            break;
        }
        case TelnetConst::END:
        {
            while(_charBuff.getOneCh(RIGHT).size()) moveRight();
            break;
        }
        case TelnetConst::CTRL_B:
        {
            moveWord(LEFT);
            break;
        }
        case TelnetConst::CTRL_N:
        {
            moveWord(RIGHT);
            break;
        }
        case TelnetConst::ENTER_KEY:
        {
            switch (st)
            {
                case STATE_TELNET_LOGIN_USERNAME:
                {
                    m_userName = getInputStr();
                    if (m_userName.length() == 0)
                        changeState();//保持现状
                    else
                        changeState(STATE_TELNET_LOGIN_PASSWORD);
                    break;
                }
                case STATE_TELNET_LOGIN_PASSWORD:
                {
                    changeState(STATE_TELNET_LOGINING);

                    m_passWord = getInputStr();
                    m_ctx = m_term->new_session();
                    m_pTermServer->dispatch(LoginRequest(m_ctx->sequence(), m_userName, m_passWord, m_term->peer_addr, m_term->prop.Type));
                    break;
                }
                case STATE_TELNET_WORKING:
                {
                    std::string input = getInputStr();
                    if (input.length() <= 0){
                        changeState();//保持现状
                        break;
                    }

                    changeState(STATE_TELNET_EXECUTING);
                    if (input[0] != '!'){
                        m_ctx = m_term->new_session();
                        m_pTermServer->dispatch(CommandRequest(m_ctx->sequence(), m_token, input, m_additional));
                        _charBuff.pushBackHistory(input);
                    }
                    else{
                        builtin(input);
                    }
                    break;
                }
                case STATE_TELNET_NEST_EXECUTING:
                {
                    changeState(STATE_TELNET_EXECUTING);

                    std::string input = getInputStr();
                    m_pTermServer->dispatch(NestCommandResponse(m_ctx->sequence(), input, m_token, m_additional));
                    break;
                }
                default:
                {
                    (*this) << TelnetConst::CRLF;
                    changeState();
                    break;
                }
            }

            //每次回车后，清空字符串缓冲区
            _charBuff.erase();
            break;
        } // case ENTER_KEY;
        default:
        {
            switch (st)
            {
                case STATE_TELNET_LOGIN_PASSWORD:
                case STATE_TELNET_LOGIN_USERNAME:
                case STATE_TELNET_WORKING:
                case STATE_TELNET_NEST_EXECUTING:
                {
                    if (ch > TelnetConst::CTRL || getInputSize() == TelnetConst::MAX_CMD_LENGTH)
                        break;

                    std::string tempCh((const char*)&ch, 1);
                    while (1){
                        if (tempCh.size() == 4 || utf8::is_valid(tempCh.begin(), tempCh.end())
                            || m_tInBuffer.getLen() == 0 || *(m_tInBuffer.getData()) == TelnetConst::CR)
                        {
                            break;
                        }

                        ch = this->read();
                        tempCh.append((const char*)&ch, 1);
                    }

                    //执行智能提示信息
                    if (st == STATE_TELNET_WORKING && (tempCh == "\t" || tempCh == "?")){
                        std::string input = getInputNativeStr();
                        std::size_t n = input.size();
                        char c = (n > 1 ? input[n - 1] : 0);
                        std::string mode;
                        if (tempCh == "\t"){
                            mode = "DM_TAB";
                            m_intelli_mode = INTELLI_TAB;
                        }
                        else if (tempCh == "?" && c == ' '){
                            mode = "DM_BLANKQUESTION";
                            m_intelli_mode = INTELLI_BLANK_QUESTION;
                            input.erase(n - 1);
                        }
                        else if (tempCh == "?"){
                            mode = "DM_QUESTION";
                            m_intelli_mode = INTELLI_QUESTION;
                        }
                        else{
                            m_intelli_mode = INTELLI_NONE;
                        }

                        changeState(STATE_TELNET_EXECUTING);
                        m_ctx = m_term->new_session();
                        m_pTermServer->dispatch(CommandRequest(m_ctx->sequence(), m_token, input, "", mode));
                    }
                    else if (_charBuff.isAtEnd()){
                        appendCh(tempCh);
                    }
                    else{
                        insertCh(tempCh);
                    }
                    break;
                } // case INPUTSTATE;
                default:
                {
                    break;
                }
            }// SWITCH STATE
        }// SWITCH CH DEFAULT
    }// SWITCH CH
}

void TelnetHandle::builtin(const std::string &cmd)
{
    std::string cmd_ = Poco::toLower(cmd);
    std::string tip;
    tip.reserve(256);
    if (cmd_ == "!info"){
        tip.append(Poco::format("type=%s%s",    m_term->prop.Type, TelnetConst::CRLF));
        tip.append(Poco::format("client=%s%s",  m_term->peer_addr, TelnetConst::CRLF));
        tip.append(Poco::format("server=%s%s",  m_term->local_addr, TelnetConst::CRLF));
        tip.append(Poco::format("client.max=%hu%s",  m_term->prop.Max_Client, TelnetConst::CRLF));
        tip.append(Poco::format("timeout.login=%hu%s",  m_term->prop.Login_Timeout, TelnetConst::CRLF));
        tip.append(Poco::format("timeout.input.idle=%hu%s",  m_term->prop.InputIdle_Timeout, TelnetConst::CRLF));
    }
    else if (cmd_ == "!exit"){
        m_pTermServer->dispatch(LogoutRequest(m_token));
        m_token.clear();
        this->setConnected(false);
        return;
    }
    else if (cmd_ == "!shortcut"){
        tip.append(string("CTRL + d:    Exit the connection") + TelnetConst::CRLF);
        tip.append(string("CTRL + c:    Stop the currrent command execution") + TelnetConst::CRLF);
        tip.append(string("CTRL + u:    Delete all characters before the cursor") + TelnetConst::CRLF);
        tip.append(string("CTRL + k:    Delete all characters after the cursor") + TelnetConst::CRLF);
        tip.append(string("CTRL + w:    Delete the word before the cursor") + TelnetConst::CRLF);
        tip.append(string("CTRL + b:    Move the cursor to the front of the word") + TelnetConst::CRLF);
        tip.append(string("CTRL + n:    Move the cursor to the front of the next word") + TelnetConst::CRLF);
        tip.append(string("HOME:        Move the cursor to the front") + TelnetConst::CRLF);
        tip.append(string("END:         Move the cursor to the end") + TelnetConst::CRLF);
    }
    else{
        tip.append(std::string("!info       display connection information") + TelnetConst::CRLF);
        tip.append(std::string("!exit       exit the connection. The shortcut key \"CTRL + d\"") + TelnetConst::CRLF);
        tip.append(std::string("!shortcut   display the shortcut keys of the terminal") + TelnetConst::CRLF);
    }

    (*this) << tip << flush_out;
    changeState(STATE_TELNET_WORKING);
}

void TelnetHandle::send2terminal(const MessageInfo &data)
{
    Poco::ScopedLock<Poco::FastMutex> lock(m_tMutex);

    if (data.type() == EV_ROUTER_TELNET_LOGIN_ACK){
        if (state() != STATE_TELNET_LOGINING){
            logError("invalid login response");
            return;
        }
        const LoginResponse &result = dynamic_cast<const LoginResponse&>(data);
        if (result.error_code() == 0){
            m_token = result.token();
            if (m_token.empty()){
                (*this) << "invalid token" << flush_out;
                if (++m_loginTimes == TelnetConst::MAX_LOGIN_TIMES){
                    this->setConnected(false);
                    return;
                }
                changeState(STATE_TELNET_LOGIN_USERNAME);
				return;
            }

            m_loginTimes = 0;
            changeState(STATE_TELNET_WORKING);
        }
        else{
            if (++m_loginTimes == TelnetConst::MAX_LOGIN_TIMES){
                (*this) << "\nPlease make sure you have the correct logon rights." << flush_out;
                this->setConnected(false);
                return;
            }

            (*this) << result.error_descript() << ", please try again." << flush_out;
            changeState(STATE_TELNET_LOGIN_USERNAME);
        }

        m_ctx.reset();
    }
    else if (data.type() == EV_ROUTER_TELNET_COMMAND_ACK){
        const CommandResponse &result = dynamic_cast<const CommandResponse&>(data);
        //当前的同步应答结束，判断顺序号是否一致
        if (!result.asyncmsg() && (!m_ctx || m_ctx->sequence() != result.sequence())){
            std::string err = Poco::format("invalid sequence(%u) in command", result.sequence());
            logError(err.c_str());
            return;
        }
        if (result.error_code() != 0){
            std::string err = Poco::format("%s(%d)", result.error_descript(), result.error_code());
            (*this) << err << TelnetConst::CRLF;
        }

        const std::string &output = result.result();
        //普通命令
        if (likely(m_intelli_mode == INTELLI_NONE)){
            (*this) << output << flush_out;
            if (m_ctx && m_ctx->sequence() == result.sequence())
                m_output_size += output.size();

            m_additional = result.additional();
            //当前的同步应答结束，或者异步应答
            if (result.lastpack() || result.asyncmsg()){
                //判断应答顺序号与当前请求顺序是否一致，如果一致就结束当前的请求
                if (m_ctx && m_ctx->sequence() == result.sequence()){
                    m_ctx.reset();
                    changeState(STATE_TELNET_WORKING);
                }
            }
            //多包请求
            else if (!result.lastpack() && !result.asyncmsg() && result.multipackets()){
                m_pTermServer->dispatch(MultiPacketsRequest(m_ctx->sequence(), m_token));
            }
        }
        //智能提示
        else{
            if (m_intelli_mode == INTELLI_TAB){
                m_echo = output;
                if (result.lastpack()){
                    m_echo = !m_echo.empty() ? m_echo : getInputNativeStr();
                    m_ctx.reset();
                    changeState(STATE_TELNET_WORKING);
                }
                else{
                    (*this) << output << flush_out;
                    m_output_size += output.size();
                }
            }
            else if (m_intelli_mode == INTELLI_QUESTION || m_intelli_mode == INTELLI_BLANK_QUESTION){
                m_multiple_output += output;
                Poco::UInt16 screen_columns = getWidth(), screen_rows = getRows();
                if (state() == STATE_TELNET_EXECUTING && (result.lastpack() || TelnetConst::screen_full(screen_columns, screen_rows, m_multiple_output))){
                    clear_line();

                    std::string output = TelnetConst::screen_getlines(screen_columns, screen_rows, m_multiple_output);
                    (*this) << output << flush_out;
                    m_output_size += output.size();
                    changeState(STATE_TELNET_SCROLL_LINES);
                }

                if (result.lastpack()){
                    m_ctx.reset();
                    m_echo = (result.error_code() != 0 ? m_echo : getInputNativeStr());

                    if (m_multiple_output.empty())
                        changeState(STATE_TELNET_WORKING);
                }
            }
        }
    }
    else if (data.type() == EV_ROUTER_TELNET_MULTIPACKETS_ACK){
        const MultiPacketsResponse &result = dynamic_cast<const MultiPacketsResponse&>(data);
        if (!m_ctx || m_ctx->sequence() != result.sequence()){
            std::string err = Poco::format("invalid sequence(%u) in multiple packets", result.sequence());
            logError(err.c_str());
            return;
        }
        if (result.error_code() != 0){
            std::string err = Poco::format("%s(%d)", result.error_descript(), result.error_code());
            (*this) << err << TelnetConst::CRLF;
        }

        const std::string &output = result.result();
        m_multiple_output += output;

        Poco::UInt16 screen_columns = getWidth(), screen_rows = getRows();
        bool fullscreen = TelnetConst::screen_full(screen_columns, screen_rows, m_multiple_output);
        if (state() == STATE_TELNET_EXECUTING){
            //满屏显示
            if (result.lastpack() || fullscreen){
                clear_line();

                std::string output = TelnetConst::screen_getlines(screen_columns, screen_rows, m_multiple_output);
                (*this) << output << flush_out;

                m_output_size += output.size();
                changeState(STATE_TELNET_SCROLL_LINES);

                fullscreen = TelnetConst::screen_full(screen_columns, screen_rows, m_multiple_output);
            }
        }

        if (result.lastpack()){
            m_ctx.reset();
            if (m_multiple_output.empty())
                changeState(STATE_TELNET_WORKING);
        }
        else if (!fullscreen){
            m_pTermServer->dispatch(MultiPacketsRequest(m_ctx->sequence(), m_token));
        }
    }
    else if (data.type() == EV_ROUTER_TELNET_NEST_REQ){
        const NestCommandRequest &nest_request = dynamic_cast<const NestCommandRequest&>(data);
        if (!m_ctx || m_ctx->sequence() != nest_request.sequence()){
            logError("current state is not executing command, or sequence disagree. command's sequence is %u", nest_request.sequence());
            return;
        }

        m_additional = nest_request.additional();
        std::string command = nest_request.command();
        Poco::trimInPlace(command);
        if (strncmp(command.c_str(), TelnetConst::NEST_PREFIX.c_str(), TelnetConst::NEST_PREFIX.size()) != 0)
            command.insert(0, TelnetConst::NEST_PREFIX);

        setPrompt(command);
        changeState(STATE_TELNET_NEST_EXECUTING);
    }
    else if (data.type() == EV_ROUTER_TELNET_BREAKCOMMAND_REQ){
        const BreakCommandRequest &break_request = dynamic_cast<const BreakCommandRequest&>(data);
        if (!m_ctx || m_ctx->sequence() != break_request.sequence() || m_token != break_request.token()){
            logError("current state is not executing command, or sequence not disagree. command's sequence is %u", break_request.sequence());
            return;
        }

        m_ctx.reset();
        changeState(STATE_TELNET_WORKING);
    }
    else if (data.type() == EV_ROUTER_TELNET_LOGOUT_REQ){
        const LogoutRequest &logout = dynamic_cast<const LogoutRequest&>(data);
        if (m_token != logout.token())
            return;

        m_token.clear();
        this->setConnected(false);
    }
    else if (data.type() == EV_ROUTER_TELNET_HEARTBEAT_REQ){
        const HeartbeatRequest &heartbeat = dynamic_cast<const HeartbeatRequest&>(data);
        if (m_token != heartbeat.token()){
            logError("token not disagree. heartbeat's token is %s", heartbeat.token().c_str());
            return;
        }

        m_heartbeat = 3;
    }
    else if (data.type() == EV_ROUTER_TELNET_CHANGE_PROMPT){
        const PromptRequest &prompt = dynamic_cast<const PromptRequest&>(data);
        std::string new_prompt = prompt.prompt();
        logDebug("change prompt from \"%s\" to \"%s\"", m_prompt.c_str(), new_prompt.c_str());
        m_prompt = (new_prompt.size() ? new_prompt : TelnetConst::TELNET_PROMPT);
        if (STATE_TELNET_WORKING == state()){
            changeState();
        }
    }
}

void TelnetHandle::check_timeout(std::size_t seconds)
{
    if (!m_bConnected)
        return;
    m_timeout_ticks += seconds;

    std::string message;
    TelnetState st = state();
    if (st == STATE_TELNET_WORKING){
        if (!m_term->prop.InputIdle_Timeout || m_timeout_ticks <= m_term->prop.InputIdle_Timeout){
            if (m_term->prop.Heartbeat > 0 && m_token.size() && m_timeout_ticks % m_term->prop.Heartbeat < seconds)
                m_pTermServer->dispatch(HeartbeatRequest(m_token));
            return;
        }
        message = "Timeout waiting for input";
    }
    else if (st == STATE_TELNET_LOGIN_USERNAME || st == STATE_TELNET_LOGIN_PASSWORD){
        if (m_timeout_ticks <= m_term->prop.LoginIdle_Timeout)
            return;
        message = "Login timeout";
    }
    else if (st == STATE_TELNET_LOGINING){
        if (m_timeout_ticks <= m_term->prop.Login_Timeout)
            return;
        message = "Login timeout";
    }
    else{
        return;
    }

    logWarn("peer address:%s, %s", m_term->peer_addr.c_str(), message.c_str());
    Poco::ScopedLock<Poco::FastMutex> lock(m_tMutex);
    (*this) << TelnetConst::CRLF << message << TelnetConst::CRLF << flush_out;
    if (m_token.size())
        m_pTermServer->dispatch(LogoutRequest(m_token));
    m_token.clear();

    this->setConnected(false, false);
}

void TelnetHandle::changeState(TelnetState state)
{
    TelnetState old_state = m_telnetState;
    state = (state == STATE_TELENT_KEEP_STATE) ? m_telnetState : state;
    m_telnetState = state;
    switch (state)
    {
        case STATE_TELNET_INIT:
        {
            (*this) << TelnetConst::INIT_MESSAGE;//发送提示信息
            break;
        }
        case STATE_TELNET_LOGIN_USERNAME:
        {
            m_token.clear();

            (*this) << TelnetConst::CRLF;
            setPrompt(TelnetConst::LOGIN_PROMPT);
            _charBuff.clearHistory();

            m_timeout_ticks = 0;
            break;
        }
        case STATE_TELNET_LOGIN_PASSWORD:
        {
            (*this) << TelnetConst::CRLF;
            setPrompt(TelnetConst::PASSWORD_PROMPT);

            m_timeout_ticks = 0;
            break;
        }
        case STATE_TELNET_LOGINING:
        {
            (*this) << TelnetConst::CRLF;

            m_timeout_ticks = 0;
            break;
        }
        case STATE_TELNET_WORKING:
        {
            if (old_state == STATE_TELNET_WORKING || m_output_size)
                (*this) << TelnetConst::CRLF;
            setPrompt(m_prompt);

            //添加回显信息
            appendString(m_echo);

            m_echo.clear();
            m_multiple_output.clear();
            m_intelli_mode  = INTELLI_NONE;
            m_output_size   = 0;
            m_timeout_ticks = 0;
            break;
        }
        case STATE_TELNET_EXECUTING:
        {
            if (likely(old_state != STATE_TELNET_SCROLL_LINES))
                (*this) << TelnetConst::CRLF;
            break;
        }
        case STATE_TELNET_NEST_EXECUTING:
        {
            break;
        }
        case STATE_TELNET_SCROLL_LINES:
        {
            break;
        }
        default:
        {
            (*this) << TelnetConst::CRLF;
            setPrompt(m_prompt);

            m_timeout_ticks = 0;
            break;
        }
    }

    (*this) << flush_out;
}

/**
* @details
* 函数名称：appendCh<br>
* 功能描述：Telnet 附加字符<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param tempCh 过滤的非控制字符
*/
void TelnetHandle::appendCh(const std::string& tempCh)
{
    Position lastPos = _charBuff.getCursorPos();
    Poco::UInt16 asteriskLength = _charBuff.appendCh(tempCh);
    if (asteriskLength == 0)
        return;

    _terminalIOWrapper.storeCursor();
    if (state() == STATE_TELNET_LOGIN_PASSWORD)
        (*this) << std::string(asteriskLength, TelnetConst::PASSWORD_ASTERISK[0]);
    else
        (*this) << tempCh;

    if (_charBuff.getCursorPos().cursorRow == lastPos.cursorRow + 1){
        (*this) << TelnetConst::CRLF;

        _terminalIOWrapper.restoreCursor();
        _terminalIOWrapper.moveCursor(TelnetConst::DOWN, 1);
        _terminalIOWrapper.moveCursor(TelnetConst::LEFT, _charBuff.getWidth() - _charBuff.getCursorPos().cursorPos);
    }
    else{
        _terminalIOWrapper.restoreCursor();
        _terminalIOWrapper.moveCursor(TelnetConst::RIGHT, asteriskLength);
    }

    if (state() == STATE_TELNET_WORKING){
        _charBuff.updateLastHistory(getInputNativeStr());
    }
}

void TelnetHandle::appendString(const std::string &str)
{
    _charBuff.appendCh(str);
    (*this) << str << flush_out;
}

/**
* @details
* 函数名称：insertCh<br>
* 功能描述：Telnet 在当前光标处插入字符<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param tempCh 过滤的非控制字符
*/
void TelnetHandle::insertCh( const std::string& tempCh )
{
    Position lastPos        = _charBuff.getCursorPos();
    Position lastEndPos     = _charBuff.getEndCursor();
    Poco::UInt16 sizeInsert =  _charBuff.insert(tempCh);
    if (sizeInsert == 0)
        return;

    _terminalIOWrapper.eraseToEndOfScreen();
    _terminalIOWrapper.storeCursor();
    std::string outPut = _charBuff.getStrToEnd(tempCh.length());

    if (state() == STATE_TELNET_LOGIN_PASSWORD)
    {
        Poco::UInt16 strANSILength = TelnetConst::getUTF8StrANSILength(outPut);
        outPut = std::string(strANSILength, TelnetConst::PASSWORD_ASTERISK[0]);
    }

    (*this) << outPut;
    if ( _charBuff.getCursorPos().cursorRow == lastPos.cursorRow + 1 )
    {
        _terminalIOWrapper.restoreCursor();
        _terminalIOWrapper.moveCursor( TelnetConst::DOWN, 1 );
        _terminalIOWrapper.moveCursor( TelnetConst::LEFT,
                                       _charBuff.getWidth() - _charBuff.getCursorPos().cursorPos );
    }
    else
    {
        _terminalIOWrapper.restoreCursor();

        if ( _charBuff.getEndCursor().cursorRow == lastEndPos.cursorRow + 1 )
        {
            if ( _charBuff.getEndCursor().cursorPos == 0 )
            {
                _terminalIOWrapper.storeCursor();
                (*this) << TelnetConst::CRLF;
                _terminalIOWrapper.restoreCursor();
            }
        }

        _terminalIOWrapper.moveCursor( TelnetConst::RIGHT, sizeInsert > 1 ? 2 : sizeInsert );
    }

    if (state() == STATE_TELNET_WORKING)
    {
        _charBuff.updateLastHistory(getInputNativeStr());
    }
}

/**
* @details
* 函数名称：moveLeft<br>
* 功能描述：Telnet 在当前光标处左移<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
*/
void TelnetHandle::moveLeft()
{
    try{
        Position lastPos = _charBuff.getCursorPos();
        std::string::difference_type distance = _charBuff.moveOneCh(LEFT);
        if (distance == 0){
            _terminalIOWrapper.bell();
            return;
        }

        if (_charBuff.getCursorPos().cursorRow == lastPos.cursorRow - 1)
        {
            _terminalIOWrapper.moveCursor( TelnetConst::UP, 1 );
            _terminalIOWrapper.moveCursor( TelnetConst::RIGHT, _charBuff.getCursorPos().cursorPos );
        }
        else
        {
            _terminalIOWrapper.moveCursor( TelnetConst::LEFT, distance > 1 ? 2 : distance );
        }
    }
    catch ( ... ){
        _terminalIOWrapper.bell();
    }
}

/**
* @details
* 函数名称：moveRight<br>
* 功能描述：Telnet 在当前光标处右移<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
*/
void TelnetHandle::moveRight()
{
    try{
        Position lastPos = _charBuff.getCursorPos();
        std::string::difference_type distance = _charBuff.moveOneCh(RIGHT);
        if (distance == 0){
            _terminalIOWrapper.bell();
            return;
        }

        if (_charBuff.getCursorPos().cursorRow == lastPos.cursorRow + 1){
            _terminalIOWrapper.moveCursor( TelnetConst::DOWN, 1 );
            _terminalIOWrapper.moveCursor( TelnetConst::LEFT, _charBuff.getWidth() - _charBuff.getCursorPos().cursorPos );
        }
        else{
            _terminalIOWrapper.moveCursor( TelnetConst::RIGHT, distance > 1 ? 2 : distance );
        }
    }
    catch (...){
        _terminalIOWrapper.bell();
    }
}

void TelnetHandle::eraseFromCursor(Direction direct)
{
    while(deleteCh(direct));
}

void TelnetHandle::deleteBeforeWord()
{
    std::string ch;
    while(1){
        ch = _charBuff.getOneCh(LEFT);
        if (ch.empty() || !(ch == " " || ch == "\t"))
            break;
        deleteCh(LEFT);
    }
    while(1){
        ch = _charBuff.getOneCh(LEFT);
        if (ch.empty() || ch == " " || ch == "\t")
            break;
        deleteCh(LEFT);
    }
}

void TelnetHandle::moveWord(Direction direct)
{
    std::function<void(TelnetHandle&)> move_func = (direct == LEFT ? &TelnetHandle::moveLeft : &TelnetHandle::moveRight);
    std::string ch;

    //去除前后的空格
    while(1){
        ch = _charBuff.getOneCh(direct);
        if (ch.empty() || !(ch == " " || ch == "\t"))
            break;
        move_func(*this);
    }

    //移动到有空格的地方
    while(1){
        ch = _charBuff.getOneCh(direct);
        if (ch.empty() || ch == " " || ch == "\t")
            break;
        move_func(*this);
    }

    //对于右移继续移动没有空格的地方
    while(direct == RIGHT){
        ch = _charBuff.getOneCh(direct);
        if (ch.empty() || !(ch == " " || ch == "\t"))
            break;
        move_func(*this);
    }
}

/**
* @details
* 函数名称：deleteCh<br>
* 功能描述：Telnet 在当前光标处右删除字符<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
*/
bool TelnetHandle::deleteCh(const Direction& direct)
{
    try
    {
        Position lastPos = _charBuff.getCursorPos();
        std::string::difference_type distance = _charBuff.deleteOneUTF8Ch(direct);
        if (distance == 0)
            return false;

        std::string strToEnd = _charBuff.getStrToEnd();
        if(direct == LEFT){
            if (_charBuff.getCursorPos().cursorRow == lastPos.cursorRow - 1){
                _terminalIOWrapper.moveCursor(TelnetConst::UP, 1);
                _terminalIOWrapper.moveCursor(TelnetConst::RIGHT, _charBuff.getCursorPos().cursorPos);
            }
            else{
                _terminalIOWrapper.moveCursor(TelnetConst::LEFT, distance > 1 ? 2 : distance);
            }
        }
        else{
        }

        _terminalIOWrapper.eraseToEndOfScreen();
        if (strToEnd.empty())
            return true;

        if (state() == STATE_TELNET_LOGIN_PASSWORD){
            Poco::UInt16 strANSILength = TelnetConst::getUTF8StrANSILength(strToEnd);
            strToEnd = std::string(strANSILength, '*');
        }

        _terminalIOWrapper.storeCursor();
        this->sendBytes((LPBYTE)strToEnd.c_str(), strToEnd.length());
        _terminalIOWrapper.restoreCursor();

        if (state() == STATE_TELNET_WORKING)
        {
            _charBuff.updateLastHistory(getInputNativeStr());
        }
    }
    catch ( ... )
    {
        _terminalIOWrapper.bell();
        return false;
    }

    return true;
}

/**< 心跳处理 */
void TelnetHandle::heartBeat(bool bActive)
{
    /* 不需要心跳 */
    m_iHeartBeats = 0;
}

/**
* @details
* 函数名称：stripCRSeq<br>
* 功能描述：Telnet 处理CR LF联系和不连续的情况<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param i 上一个字符
* @return Poco::Int16 ENTER_KEY 内部控制码 或 过滤LF后的下一个字符
*/
Poco::Int16 TelnetHandle::processCRLFSeq(Poco::Int16 &i)
{
    if (i != TelnetConst::CR)
        return i;

    try{
        rawRead();
    }
    //有些终端连续发CR + LF,但是有些终端只下发CR，捕捉该异常，直接丢弃
    catch (...) {
    }
    return TelnetConst::ENTER_KEY;
}

/**
* @details
* 函数名称：historyCmds<br>
* 功能描述：Telnet 历史命令显示<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param direct 上翻、下翻
*/
void TelnetHandle::historyCmds(const Direction& direct)
{
    std::string historyMML;
    if (!_charBuff.getHistory(direct, historyMML)){
        _terminalIOWrapper.bell();
        return;
    }

    Position lastPos = _charBuff.getCursorPos();
    while (lastPos.cursorRow > 1){
        _terminalIOWrapper.moveCursor( TelnetConst::UP, 1 );
        lastPos.cursorRow--;
    }

    _charBuff.resetPrompt(_charBuff.getCurrentPrompt());
    if ( lastPos.cursorPos > _charBuff.getCursorPos().cursorPos )
    {
        _terminalIOWrapper.moveCursor( TelnetConst::LEFT, lastPos.cursorPos - _charBuff.getCursorPos().cursorPos );
    }
    else
    {
        _terminalIOWrapper.moveCursor( TelnetConst::RIGHT, _charBuff.getCursorPos().cursorPos - lastPos.cursorPos );
    }

    _terminalIOWrapper.eraseToEndOfScreen();
    appendString(historyMML);

    if (_charBuff.getCursorPos().cursorPos == 0)
        (*this) << TelnetConst::CRLF;
}

void TelnetHandle::setPrompt(const std::string &prompt)
{
    _charBuff.resetPrompt(prompt);
    (*this) << prompt << flush_out;
}

void TelnetHandle::clear_line()
{
    _terminalIOWrapper.eraseLine();
    (*this) << "\r" << flush_out;
}

NS_VCN_OAM_END
