/**@file
* 版权所有(C)2001, 深圳市中兴通讯股份有限公司<br>
* 文件名称：AbstractorTelnetTerminal.h<br>
* 内容摘要：<br>
* 其它说明：参考现有实现
* @version 1.0
* @author
* @since  2009-06-04
*/

#ifndef ABSCTRACT_TELNET_TERMINAL_H
#define ABSCTRACT_TELNET_TERMINAL_H

/* ======================== 引用 ============================ */
#include <string>

#include "Poco/Types.h"

#include "ITelnetTerminal.h"
/* ======================== 命名空间 ========================= */

NS_VCN_OAM_BEGIN(ROUTER)


class AbstractorTelnetTerminal: public ITelnetTerminal
{

public:

    AbstractorTelnetTerminal();

    virtual ~AbstractorTelnetTerminal();

    /**
    * @brief 控制字符转换到内部定义  如 EOT BACKSPACE DEL等，可不处理
    */
    virtual Poco::Int16 translateControlCharacter( const Poco::Int16& byteread );
    /**
    * @brief ESCAPE 控制序列转换到内部定义
    */
    virtual Poco::Int16 translateEscapeSequence( Poco::Int16* buffer );
    /**
    * @brief 根据功能获取控制序列
    */
    virtual void getEraseSequence( const Poco::Int16& eraseFunc, Poco::Int8* bufferOut, Poco::Int16& outLen  );
    /**
    * @brief 鼠标移动控制序列
    */
    virtual void getCursorMoveSequence( const Poco::Int16& dir, const Poco::Int16& times, Poco::Int8* bufferOut );
    /**
    * @brief 鼠标定位控制序列
    */
    virtual void getCursorPositioningSequence( Poco::Int16* pos, Poco::Int8* bufferOut );
    /**
    * @brief 特殊控制序列 ----
    */
    virtual void getSpecialSequence( const Poco::Int16& sequence, Poco::Int8* bufferOut, Poco::Int16& outLen );
    /**
    * @brief 滚动控制？
    */
    virtual void getScrollMarginsSequence( const Poco::Int16& topmargin, const Poco::Int16& bottommargin, Poco::Int8* bufferOut );
    /**
    * @brief 显示控制
    */
    virtual void getGRSequence( const Poco::Int16& type, const Poco::Int16& param, Poco::Int8* bufferOut );
    /**
    * @brief 根据显示控制格式化输出(附加控制序列)
    */
    virtual void format( std::string& str );
    /**
    * @brief 根据显示控制格式化输出(附加控制序列)
    */
    virtual void formatBold( std::string& str );
    /**
    * @brief 初始化控制序列
    */
    virtual void getInitSequence( Poco::Int8* bufferOut );
    /**
    * @brief 终端是否支持控制格式化输出（颜色、格式）
    */
    virtual bool supportsSGR();
    /**
    * @brief 终端是否支持滚动
    */
    virtual bool supportsScrolling();
    /**
    * @brief ESCAPE 序列长度   （2）
    */
    virtual Poco::Int16 getAtomicSequenceLength();

protected:

private:

};

NS_VCN_OAM_END

#endif
