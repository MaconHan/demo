/***@file
* ��Ȩ����(C)2008, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�UTF8CharBuff.cpp<br>
* �ļ���ʶ��<br>
* ����ժҪ��<br>
* ����˵����<br>
* @version v1.0<br>
* @author
* @since 2009-06-04
*/
#include "Poco/String.h"

#include "InputCharBuffer.h"


NS_VCN_OAM_BEGIN(ROUTER)

/**
 * @details
 * �������ƣ�UTF8CharBuff<br>
 * �������������캯��<br>
 * ����˵����<br>
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
* �������ƣ�~UTF8CharBuff<br>
* ������������������<br>
* ����˵����<br>
* @version v1.0
*/
InputCharBuffer::~InputCharBuffer()
{

}

/**
* @details
* �������ƣ�appendCh<br>
* ��������������һ��UTF-8�ַ����򴮣�<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @param val     ����һ��UTF-8�ַ����򴮣�
* @retrun Poco::UInt16 ���ַ�������Ļ���
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
* �������ƣ�resetPrompt<br>
* ����������������ʾ������ʾ��������Ҫʹ�����ģ������ڽ�������ƫ��<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @param val     ��ʾ��
* @param index     �ַ�������
* @param inputValue     �ַ���
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
* �������ƣ�chgTermSize<br>
* �����������ն˳ߴ�仯<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @param width    �ն˿��
* @param rows     �ն˸߶�
* @retrun Position �仯��Ĺ��λ��
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
* �������ƣ�erase<br>
* ���������������ַ�������������ʾ����<br>
* ����˵����<br>
* @version v1.0
* @see N/A
*/
void InputCharBuffer::erase()
{
    _inputStr.erase();
    //resetPrompt(_prompt); ����
}

/**
* @details
* �������ƣ�moveOneCh<br>
* �����������ƶ�һ��UTF-8�ַ�<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @param direct    ����
* @retrun std::string::difference_type ɾ���ַ�����Ļ���
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
* �������ƣ�getStrToEnd<br>
* ������������ƫ������ʼ��ȡ�ַ���<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @param offLeft    ƫ����
* @retrun std::string ��ƫ������ʼ��ȡ���ַ���
*/
std::string InputCharBuffer::getStrToEnd( Poco::UInt16 offLeft )
{
    Poco::UInt16 start = ( _indexInBuff - offLeft ) < 0 ? 0 : _indexInBuff - offLeft;
    return _inputStr.substr( start - _prompt.length() );
}

/**
* @details
* �������ƣ�insert<br>
* ��������������һ��UTF-8�ַ�������ָ��λ�ã���ָ�������ڵ�ǰ���λ�ò���--�ݲ�֧�֣�<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @param val    UTF-8�ַ���
* @param index  ����λ�� �ݲ�֧��
* @retrun Poco::UInt16 �����ַ�����Ļ��ʾ���
*/
Poco::UInt16 InputCharBuffer::insert( const std::string& val, const Poco::UInt16& index /* = 0 */ )
{

    Poco::UInt16 strLength = 0;

    if ( index != 0 )
    {
        //...�ݲ�֧��ָ��λ�ò���
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
* �������ƣ�deleteOneUTF8Ch<br>
* �����������ڵ�ǰλ��ɾ��һ��UTF-8�ַ�<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @param direct    �������ң�
* @return std::string::difference_type  ɾ�����ַ�����Ļ����
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
* �������ƣ�getWidth<br>
* ������������ȡ��ǰ��Ļ���<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @return Poco::UInt16  ��ǰ��Ļ���
*/
Poco::UInt16 InputCharBuffer::getWidth()
{
    return _terminalWidth;
}

/**
* @details
* �������ƣ�getRows<br>
* ������������ȡ��ǰ��Ļ�߶�<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @return Poco::UInt16  ��ǰ��Ļ�߶�
*/
Poco::UInt16 InputCharBuffer::getRows()
{
    return _terminalRows;
}

/**
* @details
* �������ƣ�isAtEnd<br>
* �����������ж��Ƿ��ڵ�ǰ�ַ�����β��<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @return bool �Ƿ�����Ļ�ַ���β��
*/
bool InputCharBuffer::isAtEnd()
{
    return _indexInBuff == ( _inputStr.length() + _prompt.length() );
}

/**
* @details
* �������ƣ�getCursorPos<br>
* ������������ȡ��ǰ���λ��<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @return Position ��ǰ���λ��
*/
Position InputCharBuffer::getCursorPos()
{
    return _cursor;
}

/**
* @details
* �������ƣ�getEndCursor<br>
* ������������ȡ��ǰ���λ��<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @return Position ��ǰ���λ��
*/
Position InputCharBuffer::getEndCursor()
{
    return _endCursor;
}

/**
* @details
* �������ƣ�getCurrentStr<br>
* ������������ȡ��ǰ�ַ���<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @return const std::string& ��ǰ�ַ���
*/
const std::string& InputCharBuffer::getCurrentStr() const
{
    return _inputStr;
}

/**
* @details
* �������ƣ�getCurrentPrompt<br>
* ������������ȡ��ǰ�ַ���<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @return const std::string& ��ǰ��ʾ��
*/
const std::string& InputCharBuffer::getCurrentPrompt() const
{
    return _prompt;
}

/**
* @details
* �������ƣ�getCurrentStr<br>
* ������������ȡTrim��ĵ�ǰ�ַ���<br>
* ����˵����<br>
* @version v1.0
* @see N/A
* @return std::string Trim��ĵ�ǰ�ַ���
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
* @brief �����ǰ��ʷ�����Ƿ�
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

    if ( _vetHistoryCmds.size() >= TelnetConst::MAX_HISTORY_CMDS )//��ʷ�����
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
