/**@file
* ��Ȩ����(C)2001, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�TerminalIOWrapper.h<br>
* ����ժҪ��<br>
* ����˵�����ο�����ʵ��
* @version 1.0
* @author
* @since  2009-06-04
*/

#ifndef TERMINAL_IO_WRAPPER_H
#define TERMINAL_IO_WRAPPER_H

/* ======================== ���� ============================ */
#include <string>
#include <stdio.h>

#include "Poco/Types.h"
#include "Poco/Mutex.h"

#include "syscomm.h"
#include "IIOWrapper.h"
#include "AbstractorTelnetTerminal.h"
#include "ProtocolNegotiator.h"
#include "TelnetConst.h"
/* ======================== �����ռ� ========================= */

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

    virtual Poco::Int16 read() /**< ��ȡIO����,�����첽��ʽ����Ҫ������δ���*/
    {
        Poco::Int16 i = this->_handler->read();
        Poco::Int16 j = _telnetTerminal->translateControlCharacter(i);
        if (j == TelnetConst::LOGOUT){

        }
        else if (j == TelnetConst::ESCAPE){
            // ת��Ϊ��������
            j = handleEscapeSequence(j);
        }

        //printf("TerminalIOWrapper read:%hd -> %hd\n", i, j);
        // ���ش�������ַ�������һ�����Ʊ���
        return j;
    }

    /**
    * @deprecated
    * @TODO ��Ҫ��֤CR LF ������������������CR
    */
    virtual void write( const Poco::Int8& b )/**< дһ��byte*/
    {
        // @CRLF ��linux��һ��Ҫ˳����֣������Ӧ�ñ�֤�����ﲻ����
        this->_handler->sendBytes( ( LPBYTE )&b, 1 );
    }

    /**
    * @deprecated
    */
    virtual void write( const std::string& str )/**< д�ַ���*/
    {
        this->_handler->sendBytes( ( LPBYTE ) str.c_str(), 1 );
    }

    //virtual void setCursor( const Poco::Int16& row, const Poco::Int16& col ); /**<�������λ��*/

    virtual void moveCursor(const Poco::Int16& direction, const Poco::Int16& times)/**<�ƶ����λ��*/
    {
        Poco::Int8 *tempBuffer = (Poco::Int8*)alloca(sizeof(Poco::Int8) * times * 3); //�����㹻
        memset(tempBuffer, 0, sizeof(Poco::Int8) * times * 3);

        _telnetTerminal->getCursorMoveSequence(direction, times, tempBuffer);
        this->_handler->sendBytes((LPBYTE)tempBuffer, times * 3);
    }

    //virtual void homeCursor();/**<HOME*/
    virtual void storeCursor()/**<�������λ��*/
    {
        Poco::Int8 buf[20] = {0x00};
        Poco::Int16 outputLen = 0;
        _telnetTerminal->getSpecialSequence(TelnetConst::STORECURSOR, buf, outputLen);
        this->_handler->sendBytes((LPBYTE)buf, outputLen);
    }

    virtual void restoreCursor()/**<�ָ��ϴ����λ��*/
    {
        Poco::Int8 buf[20] = {0x00};
        Poco::Int16 outputLen = 0;
        _telnetTerminal->getSpecialSequence(TelnetConst::RESTORECURSOR, buf, outputLen);
        this->_handler->sendBytes((LPBYTE)buf, outputLen);
    }

    virtual void eraseToEndOfLine()/**<�ӵ�ǰ���ɾ����END of line*/
    {
        doErase(TelnetConst::EEOL);
    }

    virtual void eraseToBeginOfLine()/**<�ӵ�ǰ���ɾ����Begin of line*/
    {
        doErase(TelnetConst::EBOL);
    }

    virtual void eraseLine()/**<ɾ����ǰ��*/
    {
        doErase(TelnetConst::EEL);
    }

    virtual void eraseToEndOfScreen()/**<ɾ������Ļ����*/
    {
        doErase(TelnetConst::EEOS);
    }

    virtual void eraseToBeginOfScreen()/**<*/
    {
        doErase( TelnetConst::EBOS );
    }

    virtual void eraseScreen()/**<ɾ����ǰ��Ļ*/
    {
        doErase( TelnetConst::EES );
    }

    virtual void bell()/**< ������ʾ*/
    {
        write(TelnetConst::BEL);
    }

    virtual void close()/**< �ر�*/
    {
        write( ( Poco::Int8 )ProtocolNegotiator::IAC );
        write( ( Poco::Int8 )ProtocolNegotiator::DO );
        write( ( Poco::Int8 )ProtocolNegotiator::LOGOUT );
        this->_handler->closeConnection();
    }

    // Ŀǰֻʵ��Ĭ�ϵ��ն� ����VT100 ���������Ƿ�֧�� supportsSGR supportsScrolling
    // ֱ��ʹ��AbstractorTelnetTerminal
    // ���Ƕ��ڲ�ͬ�նˣ�����WINDOWS�ģ��Ƿ�Ҫת���������(UTF-8 --- GBK2312)����Ҫ����
    virtual void setTerminal( std::string& terminalname )/**< �ն����ͣ���Э�̵�ʱ��ȷ��*/
    {
        _telnetTerminal = new AbstractorTelnetTerminal();
    }

    virtual void setDefaultTerminal()/**< �ն����ͣ���Э�̵�ʱ��ȷ��*/
    {
        std::string termType = "default";
        setTerminal( termType );
    }

    virtual Poco::UInt16 getRows()/**< ��ȡ�ն���ʾ�ĸ߶ȣ���������֧���޸ģ�ʵ���ϲ�֧���޸�*/
    {
        return _handler->getRows();
    }

    virtual Poco::UInt16 getColumns()/**< ��ȡ�ն���ʾ�ģ���ȣ�������֧���޸ģ�ʵ���ϲ�֧���޸�*/
    {
        return _handler->getWidth();
    }

    virtual void setSignalling( bool b )/**< ���÷���ʾ����Ĭ��֧��*/
    {

    }

    virtual bool isSignalling()/**< �Ƿ���ʾ����Ĭ��֧��*/
    {
        return true;
    }

protected:

private:
    Poco::Mutex   _mutex ;

    ServiceHandler* _handler;

    //AbstractorTelnetTerminal _telnetTerminal; // @TODO Ĭ�ϣ�

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
