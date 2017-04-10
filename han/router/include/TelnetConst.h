/**@file
* ��Ȩ����(C)2001, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�TelnetConst.h<br>
* ����ժҪ��<br>
* ����˵������
* @version 1.0
* @author
* @since 2009-06-04
*/

#ifndef TELNET_CONTROL_CONST_H
#define TELNET_CONTROL_CONST_H

/* ======================== ���� ============================ */
#include <string>

#include "Poco/Types.h"
#include "Poco/Thread.h"

#include "vcn_defs.h"
/* ======================== �����ռ� ========================= */

NS_VCN_OAM_BEGIN(ROUTER)

class TelnetConst
{
public:
    // �ڲ�����
    // λ�� 1001 - 1004
    static const Poco::UInt16 CTRL  = 1000;
    static const Poco::UInt16 UP    = 1001; /**< one up */
    static const Poco::UInt16 DOWN  = 1002; /**< one down */
    static const Poco::UInt16 RIGHT = 1003; /**< one left */
    static const Poco::UInt16 LEFT  = 1004; /**< one right */
    static const Poco::UInt16 HOME  = 1005; /**< one home */
    static const Poco::UInt16 END   = 1006; /**< one end */
    // HOME=1005, // Home cursor pos(0,0)

    // ���� 1051 ---
    static const Poco::UInt16 STORECURSOR   = 1051; /**< store cursor position + attributes*/
    static const Poco::UInt16 RESTORECURSOR = 1052; /**< restore cursor + attributes*/

    // ɾ������ 1100 ---
    static const Poco::UInt16 EEOL  = 1100; /**< erase to end of line*/
    static const Poco::UInt16 EBOL  = 1101; /**< erase to beginning of line*/
    static const Poco::UInt16 EEL   = 1103; /**< erase entire line*/
    static const Poco::UInt16 EEOS  = 1104; /**< erase to end of screen*/
    static const Poco::UInt16 EBOS  = 1105; /**< erase to beginning of screen*/
    static const Poco::UInt16 EES   = 1106; /**< erase entire screen*/

    // Escape ���� 1200 ----
    static const Poco::UInt16 ESCAPE        = 1200; /**< Escape*/
    static const Poco::UInt16 BYTEMISSING   = 1201; /**< another byte needed*/
    static const Poco::UInt16 UNRECOGNIZED  = 1202; /**< escape match missed*/

    // �����ַ� 13xx
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
    //static const Poco::UInt16 LineUpdate = 475, CharacterUpdate = 476, ScreenpartUpdate = 477; /** <@notuse �������ͳ��� ??û��ʹ�õ�.*/
    //static const Poco::UInt16 EditBuffer = 575, LineEditBuffer = 576; /** <@notuse û��ʹ�õ�*/

    //���ƴ���ctrl
    static const Poco::UInt8 DEL_AC = 11;   //ɾ����������������ַ�
    static const Poco::UInt8 DEL_BC = 21;   //ɾ�����ǰ�����������ַ�
    static const Poco::UInt8 DEL_BW = 23;   //ɾ�����ǰ�ĵ���

    // Э�鶨��
    static const Poco::UInt8 CB     = 2;   /**ctrl+b �ƶ�������ǰ*/
    static const Poco::UInt8 CN     = 14;  /**ctrl+n �ƶ������ʺ�*/
    static const Poco::UInt8 BEL    = 7;/** <bell ���������ն�.*/
    static const Poco::UInt8 BS     = 8;/** <backspace .*/
    static const Poco::UInt8 DEL    = 127;/** <del .*/
    static const Poco::UInt8 CR     = 13;/** <�س� .*/
    static const Poco::UInt8 LF     = 10;/** <���� .*/
    static const Poco::UInt8 EOT    = 4;/** < CTRL+D.�ն˶Ͽ�*/
    static const Poco::UInt8 SC     = 3;/** < CTRL+C.ֹͣ��������Ӧ��Ľ���*/
    static const Poco::UInt8 HT     = 9;/**< TAB*/
    static const Poco::UInt8 FF     = 12;/**< ��ҳ��*/
    static const Poco::UInt8 SGR    = 1;/**< CTRL+A ?? @notuse,�ݲ���Ҫ֧��*/
    static const Poco::UInt8 CAN    = 24;/**< Cancel*/
    static const Poco::UInt8 ESC    = 27;/**< ESC*/
    static const Poco::UInt8 LSB    = 91;/**< [*/
    static const Poco::UInt8 SEMICOLON = 59;/**< ;*/
    static const Poco::UInt8 A      = 65;/**< A (UP)*/
    static const Poco::UInt8 B      = 66;/**< B (DOWN)*/
    static const Poco::UInt8 C      = 67;/**< C (RIGHT)*/
    static const Poco::UInt8 D      = 68;/**< D (LEFT)*/
    static const Poco::UInt8 E      = 69; // �س�(CR)�ͻ���(LF)
    static const Poco::UInt8 H      = 49; // Home
    static const Poco::UInt8 I      = 52; // END
    static const Poco::UInt8 J      = 51; // DELETE
    static const Poco::UInt8 f      = 102;/**< ��ҳ��*/
    static const Poco::UInt8 r      = 114;/**< ��ҳ��*/
    static const Poco::UInt8 LE     = 75; // K...line erase actions related
    static const Poco::UInt8 SE     = 74; // J...screen erase actions related
    static const std::string CRLF;  /**< .*/


    //��ʽ�����ƣ���ʱ����Ҫ֧�֡�
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

    //�ڲ�����
    static const std::string TELNET_ROOT;
    static const std::string TELNET_SUFFIX;
    static const std::string TELNET_PROMPT;
    static const std::string INIT_MESSAGE;
    static const std::string LOGIN_PROMPT;
    static const std::string PASSWORD_PROMPT;
    static const std::string PASSWORD_ASTERISK;
    static const std::string NEST_PREFIX;

    static const Poco::UIntPtr MML_ERROR_CODE_TELNET_NEINFO     = 0x08D00213;  /**< ��Ԫ��Ϣ��ʾ�����ô����� ���ʻ�*/
    static const Poco::UIntPtr MML_ERROR_CODE_TELNET_SELECT_NE  = 0x08D00214;  /**< ��Ԫѡ����ʾ�����ô����� ���ʻ�*/

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
