/**@file
* ��Ȩ����(C)2001, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�UTF8CharBuff.h<br>
* ����ժҪ��<br>
* ����˵������
* @version 1.0
* @author
* @since 2009-06-04
*/

#ifndef UTF8_CHAR_BUFF_H
#define UTF8_CHAR_BUFF_H

/* ======================== ���� ============================ */
#include <string>

#include "Poco/Types.h"
#include "Poco/Mutex.h"

#include "utf8.h"
#include "syscomm.h"
#include "TelnetConst.h"

/* ======================== �����ռ� ========================= */
using namespace std;

NS_VCN_OAM_BEGIN(ROUTER)

/**
* @brief ���λ����Ϣ��
*/

struct Position
{
    Position(): cursorPos(0), cursorRow(0)
    {
    }

    Poco::UInt16 cursorPos;/**< ��� X */
    Poco::UInt16 cursorRow;/**< ��� Y */
};

/**
* @brief �ƶ�����
*/
enum Direction
{
    LEFT,/**< �� */
    RIGHT,/**< �� */
    UP,/**< �� */
    DOWN/**< �� */
};


/**
* @brief UTF8�ַ����ն���ʾ��װ�������UTF8...
*/

class InputCharBuffer
{

public:
    /**
    * @brief ����
    */
    InputCharBuffer( const Poco::UInt16 width = 80,
                     const Poco::UInt16 rows = 25,
                     const std::string& prompt = "" );
    /**
    * @brief ����
    */
    virtual ~InputCharBuffer();
    /**
    * @brief ����һ���ַ�������
    */
    Poco::UInt16 appendCh( const std::string& val );

    /**
    * @brief ����һ���ַ�����������ָ��index ����ʾ�ڵ�ǰ����λ�ò���
    */
    Poco::UInt16 insert( const std::string& val, const Poco::UInt16& index = 0 );

    /**
    * @brief ��Ӧ�ն˱仯
    */
    Position chgTermSize( const Poco::UInt16& width, const Poco::UInt16& rows );

    /**
    * @brief �ı��ն���ʾ��
    */
    void resetPrompt( const std::string& val, const Poco::UInt16& index = 0, const std::string& inputValue = "" );

    /**
    * @brief ��ȡ��ǰ���λ��
    */
    Position getCursorPos();

    /**
    * @brief ��ȡ��ǰ���λ��
    */
    Position getEndCursor();

    inline std::size_t getCurrentStrSize() const
    {
        return _inputStr.size();
    }

    /**
    * @brief ��ȡ��ǰ���ַ�����������ʾ��
    */
    const std::string& getCurrentStr() const;

    /**
    * @brief ��ȡ��ǰ��TRIM�ַ�����������ʾ��
    */
    std::string getTrimedCurStr() const;

    /**
    * @brief �ƶ�һ��UTF-8�ַ�
    */
    std::string::difference_type moveOneCh(Direction direct);

    /**
    * @brief ɾ��һ��UTF-8�ַ�
    */
    std::string::difference_type deleteOneUTF8Ch(Direction direct);

    std::string getOneCh(Direction direct);

    /**
    * @brief ��ȡ��Ӧ�ն˿��
    */
    Poco::UInt16 getWidth();

    /**
    * @brief ��ȡ��Ӧ�ն˸߶�
    */
    Poco::UInt16 getRows();

    /**
    * @brief ��ȡƫ�������ַ��������ƶ��ȳ���
    */
    std::string getStrToEnd( Poco::UInt16 offLeft = 0 );

    /**
    * @brief ������ǰ�ַ���
    */
    void erase();

    /**
    * @brief �жϵ�ǰ����Ƿ���ĩβ
    */
    bool isAtEnd();
    /**
    * @brief ��ȡ�ն���ʾ���ȣ��ն�һ������ռ�����ַ���
    */
    const std::string& getCurrentPrompt() const;
    /**
    * @brief ��ȡ���һ������
    */
    bool getHistory( const Direction& direct, std::string& history );
    /**
    * @brief ���õ�����ʷ�����Ƿ����
    */
    void updateLastHistory( const std::string& history );
    /**
    * @brief �����ʷ����
    */
    void clearHistory();
    /**
    * @brief �����ʷ����
    */
    void pushBackHistory( const std::string& history );

protected:

private:
    DECLARE_LOG();
    Poco::Mutex _Mutex;
    std::string _prompt;/**<��ʾ��*/
    std::string _inputStr;/**<�û�����*/
    std::string _lastHistoryStr;
    //std::string _tempAppendStr;/**<���������ַ�*/
    //std::string _tempInsertStr;/**<���������ַ�*/
    Poco::UInt16 _terminalWidth;/**<�ն˿��*/
    Poco::UInt16 _terminalRows;/**< �ն˸߶�*/
    Poco::UInt16 _strAllANSILength;/**<�ַ�����ANSI���ȣ����ն�����ʾʱ�����������ַ�������prompt*/
    Position _cursor;/**<���λ��*/
    Poco::UInt16 _indexInBuff;/**<�ֽ�λ�ã�����prompt*/
    Poco::UInt16 _indexInAllANSI;/**<�����_strAllANSILength�е�����*/
    Position _endCursor;/**<���λ��*/
    std::vector<std::string> _vetHistoryCmds;
    Poco::UInt16 _historyIndex;
};

NS_VCN_OAM_END
#endif
