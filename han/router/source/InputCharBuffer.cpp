/***@file
* 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
* 文件名称：UTF8CharBuff.cpp<br>
* 文件标识：<br>
* 内容摘要：<br>
* 其它说明：<br>
* @version v1.0<br>
* @author
* @since 2009-06-04
*/
#include "Poco/String.h"

#include "InputCharBuffer.h"


NS_VCN_OAM_BEGIN(ROUTER)

/**
 * @details
 * 函数名称：UTF8CharBuff<br>
 * 功能描述：构造函数<br>
 * 其它说明：<br>
 * @version v1.0
 */
InputCharBuffer::InputCharBuffer( const Poco::UInt16 width /* = 80 */,
                                  const Poco::UInt16 rows /* = 25 */,
                                  const std::string& prompt /* = */ ):
        _terminalRows( rows ),
        _terminalWidth( width ),
        _inputStr( "" ),
        _strAllANSILength( 0 ),
        _indexInBuff( 0 ),
        _lastHistoryStr( "" ),
        _vetHistoryCmds ( 0 ),
        _historyIndex ( 0 )
{
    CREATE_LOG();
    resetPrompt( _prompt );
}

/**
* @details
* 函数名称：~UTF8CharBuff<br>
* 功能描述：析构函数<br>
* 其它说明：<br>
* @version v1.0
*/
InputCharBuffer::~InputCharBuffer()
{

}

/**
* @details
* 函数名称：appendCh<br>
* 功能描述：附加一个UTF-8字符（或串）<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param val     附加一个UTF-8字符（或串）
* @retrun Poco::UInt16 该字符串的屏幕宽度
*/
Poco::UInt16 InputCharBuffer::appendCh( const std::string& val )
{
    Poco::UInt16 strLength = 0;

    try
    {
        if ( utf8::is_valid( val.begin(), val.end() ) )
        {
            strLength = TelnetConst::getUTF8StrANSILength( val );
        }
        else
        {
            strLength = val.length();
        }

    }
    catch ( ... )
    {
        strLength = val.length();
        logDebug( "/// appendCh exception." );
    }

    _inputStr.append( val );

    _indexInBuff += val.length();
    _strAllANSILength += strLength;
    _indexInAllANSI += strLength;
    chgTermSize( _terminalWidth, _terminalRows );
    return strLength;
}

/**
* @details
* 函数名称：resetPrompt<br>
* 功能描述：重置提示符，提示符尽量不要使用中文，否则在界面会存在偏移<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param val     提示符
* @param index     字符串长度
* @param inputValue     字符串
*/
void InputCharBuffer::resetPrompt( const std::string& val, const Poco::UInt16& index, const std::string& inputValue )
{
    Poco::UInt16 tempIndex = index > inputValue.length() ? inputValue.length() : index;
    _inputStr = inputValue;
    _prompt = val;
    _strAllANSILength = 0;

    try{
        std::string temp    = _prompt + _inputStr.substr( 0, tempIndex );
        _strAllANSILength   = TelnetConst::getUTF8StrANSILength( temp );
        _indexInAllANSI     = _strAllANSILength;
        _indexInBuff        = _prompt.length() + tempIndex;
    }
    catch (...){
        _strAllANSILength   = _prompt.length() + _inputStr.length();
        _indexInBuff        = _indexInAllANSI = _prompt.length() + tempIndex;
        logDebug( "/// appendCh exception." );
    }

    chgTermSize( _terminalWidth, _terminalRows );
}

/**
* @details
* 函数名称：chgTermSize<br>
* 功能描述：终端尺寸变化<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param width    终端宽度
* @param rows     终端高度
* @retrun Position 变化后的光标位置
*/
Position InputCharBuffer::chgTermSize( const Poco::UInt16& width, const Poco::UInt16& rows )
{
    Poco::UInt16 tmpWidth   = width ? width : 80;
    Poco::UInt16 tmprows    = rows ? rows : 80;
    _terminalWidth          = _terminalWidth ? _terminalWidth : 80;

    _cursor.cursorRow = _indexInAllANSI / tmpWidth + 1;
    _cursor.cursorPos = _indexInAllANSI % tmpWidth;

    _endCursor.cursorRow = _strAllANSILength / _terminalWidth + 1;
    _endCursor.cursorPos = _strAllANSILength % tmpWidth;

    _terminalWidth  = tmpWidth;
    _terminalRows   = tmprows;
    return _cursor;
}

/**
* @details
* 函数名称：erase<br>
* 功能描述：擦除字符串（不包括提示符）<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
*/
void InputCharBuffer::erase()
{
    _inputStr.erase();
    //resetPrompt(_prompt); 多余
}

/**
* @details
* 函数名称：moveOneCh<br>
* 功能描述：移动一个UTF-8字符<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param direct    方向
* @retrun std::string::difference_type 删除字符的屏幕宽度
*/
std::string::difference_type InputCharBuffer::moveOneCh(Direction direct)
{
    std::string::difference_type distance = 0;
    if (direct == LEFT && _indexInBuff == _prompt.length())
        return distance;
    if (direct == RIGHT && _indexInBuff == _inputStr.length() + _prompt.length())
        return distance;

    try
    {
        if (!utf8::is_valid(_inputStr.begin(), _inputStr.end()))
        {
            distance = 1;
        }
        else
        {
            std::string::iterator start;
            std::string::iterator find;
            find = start = _inputStr.begin() + (_indexInBuff - _prompt.length());
            if (direct == LEFT)
            {
                utf8::prior(find, _inputStr.begin());
                distance = start - find;
            }
            else
            {
                utf8::next(find, _inputStr.end());
                distance = find - start;
            }
        }
    }
    catch ( ... )
    {
        distance = 1;
    }

    if (direct == LEFT)
    {
        _indexInAllANSI = _indexInAllANSI - ( distance > 1 ? 2 : distance );
        _indexInBuff = _indexInBuff - distance;
    }
    else
    {
        _indexInAllANSI = _indexInAllANSI + ( distance > 1 ? 2 : distance );
        _indexInBuff = _indexInBuff + distance;
    }

    chgTermSize(_terminalWidth, _terminalRows);
    return distance;
}


std::string InputCharBuffer::getOneCh(Direction direct)
{
    std::string ch;
    if (direct == LEFT && _indexInBuff == _prompt.length())
        return ch;
    if (direct == RIGHT && _indexInBuff == _inputStr.length() + _prompt.length())
        return ch;

    std::string::difference_type distance = 0;
    Poco::UInt16 offset = _indexInBuff - _prompt.length();
    std::string::iterator start;
    std::string::iterator find;
    find = start = _inputStr.begin() + (_indexInBuff - _prompt.length());
    if (direct == LEFT){
        utf8::prior(find, _inputStr.begin());
        distance = start - find;
        ch = _inputStr.substr(offset - distance, distance);
    }
    else{
        utf8::next(find, _inputStr.end());
        distance = find - start;
        ch = _inputStr.substr(offset, distance);
    }
    return ch;
}
/**
* @details
* 函数名称：getStrToEnd<br>
* 功能描述：从偏移量开始获取字符串<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param offLeft    偏移量
* @retrun std::string 从偏移量开始获取的字符串
*/
std::string InputCharBuffer::getStrToEnd( Poco::UInt16 offLeft )
{
    Poco::UInt16 start = ( _indexInBuff - offLeft ) < 0 ? 0 : _indexInBuff - offLeft;
    return _inputStr.substr( start - _prompt.length() );
}

/**
* @details
* 函数名称：insert<br>
* 功能描述：插入一个UTF-8字符串（可指定位置，不指定，则在当前光标位置插入--暂不支持）<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param val    UTF-8字符串
* @param index  索引位置 暂不支持
* @retrun Poco::UInt16 插入字符的屏幕显示宽度
*/
Poco::UInt16 InputCharBuffer::insert( const std::string& val, const Poco::UInt16& index /* = 0 */ )
{

    Poco::UInt16 strLength = 0;

    if ( index != 0 )
    {
        //...暂不支持指定位置插入
    }

    try
    {
        if ( utf8::is_valid( val.begin(), val.end() ) )
        {
            strLength = TelnetConst::getUTF8StrANSILength( val );
        }
        else
        {
            strLength = val.length();
        }
    }
    catch ( ... )
    {
        strLength = val.length();
        logDebug( "/// insertCh exception." );
    }

    _inputStr.insert( ( _indexInBuff - _prompt.length() ), val );

    _strAllANSILength += strLength;
    _indexInAllANSI += strLength;
    _indexInBuff += val.length();
    chgTermSize( _terminalWidth, _terminalRows );
    return strLength;
}

/**
* @details
* 函数名称：deleteOneUTF8Ch<br>
* 功能描述：在当前位置删除一个UTF-8字符<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param direct    方向（左右）
* @return std::string::difference_type  删除的字符的屏幕长度
*/
std::string::difference_type InputCharBuffer::deleteOneUTF8Ch(Direction direct)
{
    Poco::UInt16 offset = _indexInBuff - _prompt.length();
    std::string::difference_type distance = moveOneCh(direct);
    if (distance == 0)
        return distance;

    try
    {
        if (direct == LEFT)
        {
            std::string::iterator first = _inputStr.begin() + offset - distance;
            std::string::iterator last = _inputStr.begin() + offset;
            _inputStr.erase(first, last);
        }
        else
        {
            std::string::iterator first = _inputStr.begin() + offset;
            std::string::iterator last = _inputStr.begin() + offset + distance;
            _inputStr.erase(first, last);
            _indexInBuff = _indexInBuff - distance;
            _indexInAllANSI = _indexInAllANSI - ( distance > 1 ? 2 : distance );
        }

        _strAllANSILength = _strAllANSILength - ( distance > 1 ? 2 : distance );
        chgTermSize(_terminalWidth, _terminalRows);
    }
    catch ( ... )
    {
        logDebug( "/// deleteOneUTF8Ch error." );
    }

    return distance;
}

/**
* @details
* 函数名称：getWidth<br>
* 功能描述：获取当前屏幕宽度<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return Poco::UInt16  当前屏幕宽度
*/
Poco::UInt16 InputCharBuffer::getWidth()
{
    return _terminalWidth;
}

/**
* @details
* 函数名称：getRows<br>
* 功能描述：获取当前屏幕高度<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return Poco::UInt16  当前屏幕高度
*/
Poco::UInt16 InputCharBuffer::getRows()
{
    return _terminalRows;
}

/**
* @details
* 函数名称：isAtEnd<br>
* 功能描述：判断是否在当前字符串的尾部<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return bool 是否在屏幕字符串尾部
*/
bool InputCharBuffer::isAtEnd()
{
    return _indexInBuff == ( _inputStr.length() + _prompt.length() );
}

/**
* @details
* 函数名称：getCursorPos<br>
* 功能描述：获取当前光标位置<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return Position 当前光标位置
*/
Position InputCharBuffer::getCursorPos()
{
    return _cursor;
}

/**
* @details
* 函数名称：getEndCursor<br>
* 功能描述：获取当前光标位置<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return Position 当前光标位置
*/
Position InputCharBuffer::getEndCursor()
{
    return _endCursor;
}

/**
* @details
* 函数名称：getCurrentStr<br>
* 功能描述：获取当前字符串<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return const std::string& 当前字符串
*/
const std::string& InputCharBuffer::getCurrentStr() const
{
    return _inputStr;
}

/**
* @details
* 函数名称：getCurrentPrompt<br>
* 功能描述：获取当前字符串<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return const std::string& 当前提示符
*/
const std::string& InputCharBuffer::getCurrentPrompt() const
{
    return _prompt;
}

/**
* @details
* 函数名称：getCurrentStr<br>
* 功能描述：获取Trim后的当前字符串<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @return std::string Trim后的当前字符串
*/
std::string InputCharBuffer::getTrimedCurStr() const
{
    return Poco::trim(_inputStr);
}

bool InputCharBuffer::getHistory( const Direction& direct, std::string& history )
{
    if ( direct == UP )
    {
        if ( _historyIndex <= 0 )
        {
            return false;
        }
        else
        {
            _historyIndex--;
        }
    }
    else if ( direct == DOWN )
    {
        if ( _historyIndex >= _vetHistoryCmds.size() )
        {
            return false;
        }
        else
        {
            _historyIndex++;
        }
    }
    else
    {
        return false;
    }

    history = _historyIndex == _vetHistoryCmds.size() ? _lastHistoryStr : _vetHistoryCmds[_historyIndex];

    return true;
}

/**
* @brief 变更当前历史命令是否
*/
void InputCharBuffer::updateLastHistory( const std::string& history )
{
    _lastHistoryStr = history;
    _historyIndex = _vetHistoryCmds.size();
}

void InputCharBuffer::clearHistory()
{
    _historyIndex = 0;
    _vetHistoryCmds.clear();
    _lastHistoryStr = "";
}

/**
*
*/
void InputCharBuffer::pushBackHistory( const std::string& history )
{
    std::string tempStr = history;

    if ( _vetHistoryCmds.size() != 0 && tempStr == _vetHistoryCmds[_vetHistoryCmds.size() - 1] )
    {
        _historyIndex = _vetHistoryCmds.size();
        return;
    }

    if ( _vetHistoryCmds.size() >= TelnetConst::MAX_HISTORY_CMDS )//历史命令处理
    {
        _vetHistoryCmds.erase( _vetHistoryCmds.begin() );
        _vetHistoryCmds.push_back( tempStr );
        _historyIndex = _vetHistoryCmds.size();
    }
    else
    {
        _vetHistoryCmds.push_back( tempStr );
        _historyIndex = _vetHistoryCmds.size();
    }

    _lastHistoryStr = "";
}

NS_VCN_OAM_END
