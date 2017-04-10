/**@file
* 版权所有(C)2001, 深圳市中兴通讯股份有限公司<br>
* 文件名称：TerminalIOWrapper.h<br>
* 内容摘要：<br>
* 其它说明：参考现有实现
* @version 1.0
* @author
* @since  2009-06-04
*/

#ifndef TERMINAL_IO_WRAPPER_H
#define TERMINAL_IO_WRAPPER_H

/* ======================== 引用 ============================ */
#include <string>
#include <stdio.h>

#include "Poco/Types.h"
#include "Poco/Mutex.h"

#include "syscomm.h"
#include "IIOWrapper.h"
#include "AbstractorTelnetTerminal.h"
#include "ProtocolNegotiator.h"
#include "TelnetConst.h"
/* ======================== 命名空间 ========================= */

using namespace zteomm::pub;

NS_VCN_OAM_BEGIN(ROUTER)

template <typename ServiceHandler>
class TerminalIOWrapper: public IIOWrapper
{

public:

    TerminalIOWrapper( ServiceHandler* handler = NULL ): IIOWrapper(),
            _handler( handler )
    {

    }

    virtual ~TerminalIOWrapper() {}

    void setHandle( ServiceHandler* handler )
    {
        this->_handler = handler;
    }

    virtual Poco::Int16 read() /**< 读取IO数据,采用异步方式，需要考虑如何处理*/
    {
        Poco::Int16 i = this->_handler->read();
        Poco::Int16 j = _telnetTerminal->translateControlCharacter(i);
        if (j == TelnetConst::LOGOUT){

        }
        else if (j == TelnetConst::ESCAPE){
            // 转换为控制序列
            j = handleEscapeSequence(j);
        }

        //printf("TerminalIOWrapper read:%hd -> %hd\n", i, j);
        // 返回处理过的字符或者是一个控制变量
        return j;
    }

    /**
    * @deprecated
    * @TODO 需要保证CR LF 连续输出，不单独输出CR
    */
    virtual void write( const Poco::Int8& b )/**< 写一个byte*/
    {
        // @CRLF 在linux上一定要顺序出现，这个由应用保证。这里不处理
        this->_handler->sendBytes( ( LPBYTE )&b, 1 );
    }

    /**
    * @deprecated
    */
    virtual void write( const std::string& str )/**< 写字符串*/
    {
        this->_handler->sendBytes( ( LPBYTE ) str.c_str(), 1 );
    }

    //virtual void setCursor( const Poco::Int16& row, const Poco::Int16& col ); /**<设置鼠标位置*/

    virtual void moveCursor(const Poco::Int16& direction, const Poco::Int16& times)/**<移动鼠标位置*/
    {
        Poco::Int8 *tempBuffer = (Poco::Int8*)alloca(sizeof(Poco::Int8) * times * 3); //长度足够
        memset(tempBuffer, 0, sizeof(Poco::Int8) * times * 3);

        _telnetTerminal->getCursorMoveSequence(direction, times, tempBuffer);
        this->_handler->sendBytes((LPBYTE)tempBuffer, times * 3);
    }

    //virtual void homeCursor();/**<HOME*/
    virtual void storeCursor()/**<保存鼠标位置*/
    {
        Poco::Int8 buf[20] = {0x00};
        Poco::Int16 outputLen = 0;
        _telnetTerminal->getSpecialSequence(TelnetConst::STORECURSOR, buf, outputLen);
        this->_handler->sendBytes((LPBYTE)buf, outputLen);
    }

    virtual void restoreCursor()/**<恢复上次鼠标位置*/
    {
        Poco::Int8 buf[20] = {0x00};
        Poco::Int16 outputLen = 0;
        _telnetTerminal->getSpecialSequence(TelnetConst::RESTORECURSOR, buf, outputLen);
        this->_handler->sendBytes((LPBYTE)buf, outputLen);
    }

    virtual void eraseToEndOfLine()/**<从当前鼠标删除至END of line*/
    {
        doErase(TelnetConst::EEOL);
    }

    virtual void eraseToBeginOfLine()/**<从当前鼠标删除至Begin of line*/
    {
        doErase(TelnetConst::EBOL);
    }

    virtual void eraseLine()/**<删除当前行*/
    {
        doErase(TelnetConst::EEL);
    }

    virtual void eraseToEndOfScreen()/**<删除至屏幕结束*/
    {
        doErase(TelnetConst::EEOS);
    }

    virtual void eraseToBeginOfScreen()/**<*/
    {
        doErase( TelnetConst::EBOS );
    }

    virtual void eraseScreen()/**<删除当前屏幕*/
    {
        doErase( TelnetConst::EES );
    }

    virtual void bell()/**< 声音提示*/
    {
        write(TelnetConst::BEL);
    }

    virtual void close()/**< 关闭*/
    {
        write( ( Poco::Int8 )ProtocolNegotiator::IAC );
        write( ( Poco::Int8 )ProtocolNegotiator::DO );
        write( ( Poco::Int8 )ProtocolNegotiator::LOGOUT );
        this->_handler->closeConnection();
    }

    // 目前只实现默认的终端 例如VT100 区别在于是否支持 supportsSGR supportsScrolling
    // 直接使用AbstractorTelnetTerminal
    // 但是对于不同终端，例如WINDOWS的，是否要转换输出编码(UTF-8 --- GBK2312)，需要处理
    virtual void setTerminal( std::string& terminalname )/**< 终端类型，在协商的时候确定*/
    {
        _telnetTerminal = new AbstractorTelnetTerminal();
    }

    virtual void setDefaultTerminal()/**< 终端类型，在协商的时候确定*/
    {
        std::string termType = "default";
        setTerminal( termType );
    }

    virtual Poco::UInt16 getRows()/**< 获取终端显示的高度（行数），支持修改？实际上不支持修改*/
    {
        return _handler->getRows();
    }

    virtual Poco::UInt16 getColumns()/**< 获取终端显示的（宽度）列数，支持修改？实际上不支持修改*/
    {
        return _handler->getWidth();
    }

    virtual void setSignalling( bool b )/**< 设置发提示音，默认支持*/
    {

    }

    virtual bool isSignalling()/**< 是否发提示音，默认支持*/
    {
        return true;
    }

protected:

private:
    Poco::Mutex   _mutex ;

    ServiceHandler* _handler;

    //AbstractorTelnetTerminal _telnetTerminal; // @TODO 默认？

    ITelnetTerminal* _telnetTerminal;

    Poco::Int16 handleEscapeSequence(Poco::Int16& i)
    {
        if (i == TelnetConst::ESCAPE)
        {
            const std::size_t bs = 2;
            Poco::Int16 bytebuf[bs];
            for (Poco::Int16 i = 0; i < bs; i++)
                bytebuf[i] = _handler->read();

            auto j = _telnetTerminal->translateEscapeSequence(bytebuf);
            switch(j){
                case TelnetConst::HOME:
                case TelnetConst::END:
                case TelnetConst::DELETE:
                    try{
                        auto k = _handler->read();
                        //k == '~'
                    }
                    catch(...){
                    }
                    break;
            }
            return j;
        }

        if ( i == TelnetConst::BYTEMISSING )
        {
            // @TODO ??
        }

        return TelnetConst::HANDLED;
    }

    void doErase( const Poco::Int16& eraseFunc )
    {
        Poco::Int8 buf[8] = {0x00};
        Poco::Int16 len = 0;
        _telnetTerminal->getEraseSequence(eraseFunc, buf, len);
        this->_handler->sendBytes((LPBYTE)buf, len);
    }
};

NS_VCN_OAM_END

#endif
