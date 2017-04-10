/**@file
* 版权所有(C)2001, 深圳市中兴通讯股份有限公司<br>
* 文件名称：TelnetConst.h<br>
* 内容摘要：<br>
* 其它说明：无
* @version 1.0
* @author
* @since 2009-06-04
*/

#ifndef TELNET_CONTROL_CONST_H
#define TELNET_CONTROL_CONST_H

/* ======================== 引用 ============================ */
#include <string>

#include "Poco/Types.h"
#include "Poco/Thread.h"

#include "vcn_defs.h"
/* ======================== 命名空间 ========================= */

NS_VCN_OAM_BEGIN(ROUTER)

class TelnetConst
{
public:
    // 内部定义
    // 位置 1001 - 1004
    static const Poco::UInt16 CTRL  = 1000;
    static const Poco::UInt16 UP    = 1001; /**< one up */
    static const Poco::UInt16 DOWN  = 1002; /**< one down */
    static const Poco::UInt16 RIGHT = 1003; /**< one left */
    static const Poco::UInt16 LEFT  = 1004; /**< one right */
    static const Poco::UInt16 HOME  = 1005; /**< one home */
    static const Poco::UInt16 END   = 1006; /**< one end */
    // HOME=1005, // Home cursor pos(0,0)

    // 功能 1051 ---
    static const Poco::UInt16 STORECURSOR   = 1051; /**< store cursor position + attributes*/
    static const Poco::UInt16 RESTORECURSOR = 1052; /**< restore cursor + attributes*/

    // 删除功能 1100 ---
    static const Poco::UInt16 EEOL  = 1100; /**< erase to end of line*/
    static const Poco::UInt16 EBOL  = 1101; /**< erase to beginning of line*/
    static const Poco::UInt16 EEL   = 1103; /**< erase entire line*/
    static const Poco::UInt16 EEOS  = 1104; /**< erase to end of screen*/
    static const Poco::UInt16 EBOS  = 1105; /**< erase to beginning of screen*/
    static const Poco::UInt16 EES   = 1106; /**< erase entire screen*/

    // Escape 序列 1200 ----
    static const Poco::UInt16 ESCAPE        = 1200; /**< Escape*/
    static const Poco::UInt16 BYTEMISSING   = 1201; /**< another byte needed*/
    static const Poco::UInt16 UNRECOGNIZED  = 1202; /**< escape match missed*/

    // 控制字符 13xx
    static const Poco::UInt16 ENTER_KEY = 1300; /**< LF is ENTER_KEY at the moment*/
    static const Poco::UInt16 TABULATOR = 1301; /**< Tabulator*/
    static const Poco::UInt16 BACKSPACE = 1303; /**< BACKSPACE*/
    static const Poco::UInt16 COLORINIT = 1304; /**< Color inited*/
    static const Poco::UInt16 HANDLED   = 1305;/**< .*/
    static const Poco::UInt16 LOGOUT    = 1306; /**< CTRL-D beim login*/
    static const Poco::UInt16 STOPCMD   = 1307; /**< CTRL-C beim login*/
    static const Poco::UInt16 DELETE    = 1308; /**< Delete*/
    static const Poco::UInt16 CTRL_B    = 1309; /**< */
    static const Poco::UInt16 CTRL_N    = 1310; /**< */
    //static const Poco::UInt16 LineUpdate = 475, CharacterUpdate = 476, ScreenpartUpdate = 477; /** <@notuse 更新类型常量 ??没有使用到.*/
    //static const Poco::UInt16 EditBuffer = 575, LineEditBuffer = 576; /** <@notuse 没有使用到*/

    //控制代码ctrl
    static const Poco::UInt8 DEL_AC = 11;   //删除光标后的所有输入字符
    static const Poco::UInt8 DEL_BC = 21;   //删除光标前的所有输入字符
    static const Poco::UInt8 DEL_BW = 23;   //删除光标前的单词

    // 协议定义
    static const Poco::UInt8 CB     = 2;   /**ctrl+b 移动到单词前*/
    static const Poco::UInt8 CN     = 14;  /**ctrl+n 移动到单词后*/
    static const Poco::UInt8 BEL    = 7;/** <bell 网络虚拟终端.*/
    static const Poco::UInt8 BS     = 8;/** <backspace .*/
    static const Poco::UInt8 DEL    = 127;/** <del .*/
    static const Poco::UInt8 CR     = 13;/** <回车 .*/
    static const Poco::UInt8 LF     = 10;/** <换行 .*/
    static const Poco::UInt8 EOT    = 4;/** < CTRL+D.终端断开*/
    static const Poco::UInt8 SC     = 3;/** < CTRL+C.停止上条命令应答的接收*/
    static const Poco::UInt8 HT     = 9;/**< TAB*/
    static const Poco::UInt8 FF     = 12;/**< 换页符*/
    static const Poco::UInt8 SGR    = 1;/**< CTRL+A ?? @notuse,暂不需要支持*/
    static const Poco::UInt8 CAN    = 24;/**< Cancel*/
    static const Poco::UInt8 ESC    = 27;/**< ESC*/
    static const Poco::UInt8 LSB    = 91;/**< [*/
    static const Poco::UInt8 SEMICOLON = 59;/**< ;*/
    static const Poco::UInt8 A      = 65;/**< A (UP)*/
    static const Poco::UInt8 B      = 66;/**< B (DOWN)*/
    static const Poco::UInt8 C      = 67;/**< C (RIGHT)*/
    static const Poco::UInt8 D      = 68;/**< D (LEFT)*/
    static const Poco::UInt8 E      = 69; // 回车(CR)和换行(LF)
    static const Poco::UInt8 H      = 49; // Home
    static const Poco::UInt8 I      = 52; // END
    static const Poco::UInt8 J      = 51; // DELETE
    static const Poco::UInt8 f      = 102;/**< 换页符*/
    static const Poco::UInt8 r      = 114;/**< 换页符*/
    static const Poco::UInt8 LE     = 75; // K...line erase actions related
    static const Poco::UInt8 SE     = 74; // J...screen erase actions related
    static const std::string CRLF;  /**< .*/


    //格式化控制，暂时不需要支持。
    static const Poco::UInt16 FCOLOR        = 10001;
    static const Poco::UInt16 BCOLOR        = 10002;
    static const Poco::UInt16 STYLE         = 10003;
    static const Poco::UInt16 RESET         = 10004;
    static const Poco::UInt16 BOLD          = 1;
    static const Poco::UInt16 BOLD_OFF      = 22;
    static const Poco::UInt16 ITALIC        = 3;
    static const Poco::UInt16 ITALIC_OFF    = 23;
    static const Poco::UInt16 BLINK         = 5;
    static const Poco::UInt16 BLINK_OFF     = 25;
    static const Poco::UInt16 UNDERLINED    = 4;
    static const Poco::UInt16 UNDERLINED_OFF= 24;
    static const Poco::UInt16 DEVICERESET   = 10005;
    static const Poco::UInt16 LINEWRAP      = 10006;
    static const Poco::UInt16 NOLINEWRAP    = 10007;
    static const Poco::UInt16 BLACK         = 30; /**< .*/
    static const Poco::UInt16 RED           = 31;/**< .*/
    static const Poco::UInt16 GREEN         = 32;/**< .*/
    static const Poco::UInt16 YELLOW        = 33;/**< .*/
    static const Poco::UInt16 BLUE          = 34;/**< .*/
    static const Poco::UInt16 MAGENTA       = 35;/**< .*/
    static const Poco::UInt16 CYAN          = 36;/**< .*/
    static const Poco::UInt16 WHITE         = 37;/**< .*/

    //内部定义
    static const std::string TELNET_ROOT;
    static const std::string TELNET_SUFFIX;
    static const std::string TELNET_PROMPT;
    static const std::string INIT_MESSAGE;
    static const std::string LOGIN_PROMPT;
    static const std::string PASSWORD_PROMPT;
    static const std::string PASSWORD_ASTERISK;
    static const std::string NEST_PREFIX;

    static const Poco::UIntPtr MML_ERROR_CODE_TELNET_NEINFO     = 0x08D00213;  /**< 网元信息提示，利用错误码 国际化*/
    static const Poco::UIntPtr MML_ERROR_CODE_TELNET_SELECT_NE  = 0x08D00214;  /**< 网元选择提示，利用错误码 国际化*/

    static const Poco::Int16 MAX_LOGIN_TIMES    = 3;
    static const Poco::Int16 MAX_HISTORY_CMDS   = 100;
    static const Poco::Int16 MAX_CMD_LENGTH     = 15000;

    static const std::string MORE;
public:
    static Poco::UInt16 getUTF8StrANSILength(const std::string& str);

    static std::size_t screen_getline(std::size_t max_columns, const char* str, std::size_t size, char *output);

    static std::size_t screen_getline_num(std::size_t max_columns, const std::string &str);

    static bool screen_full(std::size_t max_columns, std::size_t max_rows, const std::string &str);

    static std::string screen_getlines(std::size_t max_columns, std::size_t max_rows, std::string &str);
};

NS_VCN_OAM_END

#endif
