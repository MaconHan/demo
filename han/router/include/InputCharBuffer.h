/**@file
* 版权所有(C)2001, 深圳市中兴通讯股份有限公司<br>
* 文件名称：UTF8CharBuff.h<br>
* 内容摘要：<br>
* 其它说明：无
* @version 1.0
* @author
* @since 2009-06-04
*/

#ifndef UTF8_CHAR_BUFF_H
#define UTF8_CHAR_BUFF_H

/* ======================== 引用 ============================ */
#include <string>

#include "Poco/Types.h"
#include "Poco/Mutex.h"

#include "utf8.h"
#include "syscomm.h"
#include "TelnetConst.h"

/* ======================== 命名空间 ========================= */
using namespace std;

NS_VCN_OAM_BEGIN(ROUTER)

/**
* @brief 鼠标位置信息类
*/

struct Position
{
    Position(): cursorPos(0), cursorRow(0)
    {
    }

    Poco::UInt16 cursorPos;/**< 光标 X */
    Poco::UInt16 cursorRow;/**< 光标 Y */
};

/**
* @brief 移动方向
*/
enum Direction
{
    LEFT,/**< 左 */
    RIGHT,/**< 右 */
    UP,/**< 上 */
    DOWN/**< 下 */
};


/**
* @brief UTF8字符串终端显示包装，如果非UTF8...
*/

class InputCharBuffer
{

public:
    /**
    * @brief 构造
    */
    InputCharBuffer( const Poco::UInt16 width = 80,
                     const Poco::UInt16 rows = 25,
                     const std::string& prompt = "" );
    /**
    * @brief 析构
    */
    virtual ~InputCharBuffer();
    /**
    * @brief 附加一个字符（串）
    */
    Poco::UInt16 appendCh( const std::string& val );

    /**
    * @brief 插入一个字符（串），不指定index ，表示在当前索引位置插入
    */
    Poco::UInt16 insert( const std::string& val, const Poco::UInt16& index = 0 );

    /**
    * @brief 适应终端变化
    */
    Position chgTermSize( const Poco::UInt16& width, const Poco::UInt16& rows );

    /**
    * @brief 改变终端提示符
    */
    void resetPrompt( const std::string& val, const Poco::UInt16& index = 0, const std::string& inputValue = "" );

    /**
    * @brief 获取当前鼠标位置
    */
    Position getCursorPos();

    /**
    * @brief 获取当前鼠标位置
    */
    Position getEndCursor();

    inline std::size_t getCurrentStrSize() const
    {
        return _inputStr.size();
    }

    /**
    * @brief 获取当前的字符串，不带提示符
    */
    const std::string& getCurrentStr() const;

    /**
    * @brief 获取当前的TRIM字符串，不带提示符
    */
    std::string getTrimedCurStr() const;

    /**
    * @brief 移动一个UTF-8字符
    */
    std::string::difference_type moveOneCh(Direction direct);

    /**
    * @brief 删除一个UTF-8字符
    */
    std::string::difference_type deleteOneUTF8Ch(Direction direct);

    std::string getOneCh(Direction direct);

    /**
    * @brief 获取对应终端宽度
    */
    Poco::UInt16 getWidth();

    /**
    * @brief 获取对应终端高度
    */
    Poco::UInt16 getRows();

    /**
    * @brief 获取偏移量的字符，用于移动等场景
    */
    std::string getStrToEnd( Poco::UInt16 offLeft = 0 );

    /**
    * @brief 擦除当前字符串
    */
    void erase();

    /**
    * @brief 判断当前光标是否在末尾
    */
    bool isAtEnd();
    /**
    * @brief 获取终端显示长度（终端一个中文占两个字符）
    */
    const std::string& getCurrentPrompt() const;
    /**
    * @brief 获取最后一次输入
    */
    bool getHistory( const Direction& direct, std::string& history );
    /**
    * @brief 设置当期历史命令是否更新
    */
    void updateLastHistory( const std::string& history );
    /**
    * @brief 清除历史命令
    */
    void clearHistory();
    /**
    * @brief 清除历史命令
    */
    void pushBackHistory( const std::string& history );

protected:

private:
    DECLARE_LOG();
    Poco::Mutex _Mutex;
    std::string _prompt;/**<提示符*/
    std::string _inputStr;/**<用户输入*/
    std::string _lastHistoryStr;
    //std::string _tempAppendStr;/**<缓存输入字符*/
    //std::string _tempInsertStr;/**<缓存输入字符*/
    Poco::UInt16 _terminalWidth;/**<终端宽度*/
    Poco::UInt16 _terminalRows;/**< 终端高度*/
    Poco::UInt16 _strAllANSILength;/**<字符串的ANSI长度，在终端中显示时中文是两个字符，包括prompt*/
    Position _cursor;/**<光标位置*/
    Poco::UInt16 _indexInBuff;/**<字节位置，包括prompt*/
    Poco::UInt16 _indexInAllANSI;/**<光标在_strAllANSILength中的索引*/
    Position _endCursor;/**<光标位置*/
    std::vector<std::string> _vetHistoryCmds;
    Poco::UInt16 _historyIndex;
};

NS_VCN_OAM_END
#endif
