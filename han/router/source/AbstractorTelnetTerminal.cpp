/***@file
* ��Ȩ����(C)2008, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�AbstractorTelnetTerminal.cpp<br>
* �ļ���ʶ��<br>
* ����ժҪ��<br>
* ����˵����<br>
* @version v1.0<br>
* @author
* @since 2009-06-04
*/
#include <iostream>

#include "AbstractorTelnetTerminal.h"
#include "TelnetConst.h"


NS_VCN_OAM_BEGIN(ROUTER)

AbstractorTelnetTerminal::AbstractorTelnetTerminal()
{
}

AbstractorTelnetTerminal::~AbstractorTelnetTerminal()
{
}

/**
* @brief �����ַ�ת�����ڲ�����  �� EOT BACKSPACE DEL�ȣ��ɲ�����
*/
Poco::Int16 AbstractorTelnetTerminal::translateControlCharacter( const Poco::Int16& byteread )
{
    switch ( byteread )
    {
        case TelnetConst::DEL:
        case TelnetConst::BS:
            return TelnetConst::BACKSPACE;

        //case TelnetConst::HT:
        //    return TelnetConst::TABULATOR;

        case TelnetConst::ESC:
            return TelnetConst::ESCAPE;
        case TelnetConst::SGR:
            return TelnetConst::COLORINIT;
        case TelnetConst::EOT:
            return TelnetConst::LOGOUT;
        case TelnetConst::SC:
            return TelnetConst::STOPCMD;
        case TelnetConst::CB:
            return TelnetConst::CTRL_B;
        case TelnetConst::CN:
            return TelnetConst::CTRL_N;
        default:
            return byteread;
    }
}

/**
* @brief ESCAPE ��������ת�����ڲ�����  // buffer����Ϊ2
* @TODO �Ϸ��Լ�飿��
*/
Poco::Int16 AbstractorTelnetTerminal::translateEscapeSequence(Poco::Int16* buffer)
{
    if (*buffer == TelnetConst::LSB)
    {
        switch (*(++buffer))
        {
            case TelnetConst::A:
                return TelnetConst::UP;
            case TelnetConst::B:
                return TelnetConst::DOWN;
            case TelnetConst::C:
                return TelnetConst::RIGHT;
            case TelnetConst::D:
                return TelnetConst::LEFT;
            case TelnetConst::H:
                return TelnetConst::HOME;
            case TelnetConst::I:
                return TelnetConst::END;
            case TelnetConst::J:
                return TelnetConst::DELETE;
            default:
                break;
        }
    }

    return TelnetConst::UNRECOGNIZED;
}

/**
* @brief ���ݹ��ܻ�ȡ��������
* @TODO ָ�뷶Χ��� ����ĳ���Ϊ >=4
* @TODO ����ʹ���±����
* Poco::Int8 (&bufferOut)[4]
*/
void AbstractorTelnetTerminal::getEraseSequence( const Poco::Int16& eraseFunc, Poco::Int8* bufferOut, Poco::Int16& outLen )
{
    switch (eraseFunc)
    {
        case TelnetConst::EEOL:
            bufferOut[0] = TelnetConst::ESC;
            bufferOut[1] = TelnetConst::LSB;
            bufferOut[2] = TelnetConst::LE;
            outLen = 3;
            break;

        case TelnetConst::EBOL:
            bufferOut[0] = TelnetConst::ESC;
            bufferOut[1] = TelnetConst::LSB;
            bufferOut[2] = 49; // Ascii Code of 1
            bufferOut[3] = TelnetConst::LE;
            outLen = 4;
            break;

        case TelnetConst::EEL:
            bufferOut[0] = TelnetConst::ESC;
            bufferOut[1] = TelnetConst::LSB;
            bufferOut[2] = 50; // ASCII��---2
            bufferOut[3] = TelnetConst::LE;
            outLen = 4;
            break;

        case TelnetConst::EEOS:
            bufferOut[0] = TelnetConst::ESC;
            bufferOut[1] = TelnetConst::LSB;
            bufferOut[2] = TelnetConst::SE;
            outLen = 3;
            break;

        case TelnetConst::EBOS:
            bufferOut[0] = TelnetConst::ESC;
            bufferOut[1] = TelnetConst::LSB;
            bufferOut[2] = 49; // ASCII��---1
            bufferOut[3] = TelnetConst::SE;
            outLen = 4;
            break;

        case TelnetConst::EES:
            bufferOut[0] = TelnetConst::ESC;
            bufferOut[1] = TelnetConst::LSB;
            bufferOut[2] = 50; // ASCII��---2
            bufferOut[3] = TelnetConst::SE;
            outLen = 4;
            break;

        default:
            break;
    }
}

/**
* @brief ����ƶ���������
* @TODO ָ�뷶Χ���  ĿǰӦʹ�ò��� �����bufferOut����Ϊ times * 3
* @TODO ����ʹ���±����
*/
void AbstractorTelnetTerminal::getCursorMoveSequence( const Poco::Int16& direction, const Poco::Int16& times, Poco::Int8* bufferOut )
{
    for (int i = 0; i < times * 3; i += 3){
        bufferOut[i]    = TelnetConst::ESC;
        bufferOut[i+1]  = TelnetConst::LSB;
        switch (direction)
        {
            case TelnetConst::UP:
                bufferOut[i+2] = TelnetConst::A;
                break;
            case TelnetConst::DOWN:
                bufferOut[i+2] = TelnetConst::B;
                break;
            case TelnetConst::RIGHT:
                bufferOut[i+2] = TelnetConst::C;
                break;
            case TelnetConst::LEFT:
                bufferOut[i+2] = TelnetConst::D;
                break;
            default:
                break;
        }
    }
}


/**
* @brief ��궨λ��������
* @TODO �ݲ�֧��
*/
void AbstractorTelnetTerminal::getCursorPositioningSequence( Poco::Int16* pos, Poco::Int8* bufferOut )
{

}


/**
* @brief ����������� ----
* @TODO ����ʹ���±����
*/
void AbstractorTelnetTerminal::getSpecialSequence( const Poco::Int16& sequence, Poco::Int8* bufferOut, Poco::Int16& outLen )
{
    switch ( sequence )
    {
        case TelnetConst::STORECURSOR:
            bufferOut[0]  = TelnetConst::ESC;
            bufferOut[1]  = 55; // Ascii Code of 7
            outLen = 2;
            break;

        case TelnetConst::RESTORECURSOR:
            bufferOut[0]  = TelnetConst::ESC;
            bufferOut[1]  = 56; // Ascii Code of 8
            outLen = 2;
            break;

        case TelnetConst::DEVICERESET:
            bufferOut[0]  = TelnetConst::ESC;
            bufferOut[1]  = 99; // Ascii Code of c
            outLen = 2;
            break;

        case TelnetConst::LINEWRAP:
            bufferOut[0]  = TelnetConst::ESC;
            bufferOut[1]  = TelnetConst::LSB;
            bufferOut[2]  = 55; // Ascii code of 7
            bufferOut[3]  = 104; // Ascii code of h
            outLen = 4;
            break;

        case TelnetConst::NOLINEWRAP:
            bufferOut[0]  = TelnetConst::ESC;
            bufferOut[1]  = TelnetConst::LSB;
            bufferOut[2]  = 55; // Ascii code of 7
            bufferOut[3]  = 108; // Ascii code of l
            outLen = 4;
            break;
    }
}


/**
* @brief �������ƣ�
* @TODO �ݲ���Ҫ֧��
*/
void AbstractorTelnetTerminal::getScrollMarginsSequence( const Poco::Int16& topmargin, const Poco::Int16& bottommargin, Poco::Int8* bufferOut )
{

}


/**
* @brief ��ʾ����
* @TODO �ݲ���Ҫ֧��
*/
void AbstractorTelnetTerminal::getGRSequence( const Poco::Int16& type, const Poco::Int16& param, Poco::Int8* bufferOut )
{

}


/**
* @brief ������ʾ���Ƹ�ʽ�����(���ӿ�������)
* @TODO �ݲ���Ҫ֧��
*/
void AbstractorTelnetTerminal::format( std::string& str )
{
    // donothing
}


/**
* @brief ������ʾ���Ƹ�ʽ�����(���ӿ�������)
* @TODO �ݲ���Ҫ֧��
*/
void AbstractorTelnetTerminal::formatBold( std::string& str )
{
    // donothing
}


/**
* @brief ��ʼ����������
*/
void AbstractorTelnetTerminal::getInitSequence( Poco::Int8* bufferOut )
{
    bufferOut[0] = 0;
}


/**
* @brief �ն��Ƿ�֧�ֿ��Ƹ�ʽ���������ɫ����ʽ��
*/
bool AbstractorTelnetTerminal::supportsSGR()
{
    return false;
}


/**
* @brief �ն��Ƿ�֧�ֹ���
*/
bool AbstractorTelnetTerminal::supportsScrolling()
{
    return false;
}


/**
* @brief ESCAPE ���г���   ��2��
*/
Poco::Int16 AbstractorTelnetTerminal::getAtomicSequenceLength()
{
    return ( Poco::Int16 )2;
}

NS_VCN_OAM_END
