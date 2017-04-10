/***@file
* 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
* 文件名称：AbstractorTelnetTerminal.cpp<br>
* 文件标识：<br>
* 内容摘要：<br>
* 其它说明：<br>
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
* @brief 控制字符转换到内部定义  如 EOT BACKSPACE DEL等，可不处理
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
* @brief ESCAPE 控制序列转换到内部定义  // buffer长度为2
* @TODO 合法性检查？？
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
* @brief 根据功能获取控制序列
* @TODO 指针范围检查 传入的长度为 >=4
* @TODO 可以使用下标访问
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
            bufferOut[2] = 50; // ASCII码---2
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
            bufferOut[2] = 49; // ASCII码---1
            bufferOut[3] = TelnetConst::SE;
            outLen = 4;
            break;

        case TelnetConst::EES:
            bufferOut[0] = TelnetConst::ESC;
            bufferOut[1] = TelnetConst::LSB;
            bufferOut[2] = 50; // ASCII码---2
            bufferOut[3] = TelnetConst::SE;
            outLen = 4;
            break;

        default:
            break;
    }
}

/**
* @brief 鼠标移动控制序列
* @TODO 指针范围检查  目前应使用不上 传入的bufferOut长度为 times * 3
* @TODO 可以使用下标访问
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
* @brief 鼠标定位控制序列
* @TODO 暂不支持
*/
void AbstractorTelnetTerminal::getCursorPositioningSequence( Poco::Int16* pos, Poco::Int8* bufferOut )
{

}


/**
* @brief 特殊控制序列 ----
* @TODO 可以使用下标访问
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
* @brief 滚动控制？
* @TODO 暂不需要支持
*/
void AbstractorTelnetTerminal::getScrollMarginsSequence( const Poco::Int16& topmargin, const Poco::Int16& bottommargin, Poco::Int8* bufferOut )
{

}


/**
* @brief 显示控制
* @TODO 暂不需要支持
*/
void AbstractorTelnetTerminal::getGRSequence( const Poco::Int16& type, const Poco::Int16& param, Poco::Int8* bufferOut )
{

}


/**
* @brief 根据显示控制格式化输出(附加控制序列)
* @TODO 暂不需要支持
*/
void AbstractorTelnetTerminal::format( std::string& str )
{
    // donothing
}


/**
* @brief 根据显示控制格式化输出(附加控制序列)
* @TODO 暂不需要支持
*/
void AbstractorTelnetTerminal::formatBold( std::string& str )
{
    // donothing
}


/**
* @brief 初始化控制序列
*/
void AbstractorTelnetTerminal::getInitSequence( Poco::Int8* bufferOut )
{
    bufferOut[0] = 0;
}


/**
* @brief 终端是否支持控制格式化输出（颜色、格式）
*/
bool AbstractorTelnetTerminal::supportsSGR()
{
    return false;
}


/**
* @brief 终端是否支持滚动
*/
bool AbstractorTelnetTerminal::supportsScrolling()
{
    return false;
}


/**
* @brief ESCAPE 序列长度   （2）
*/
Poco::Int16 AbstractorTelnetTerminal::getAtomicSequenceLength()
{
    return ( Poco::Int16 )2;
}

NS_VCN_OAM_END
