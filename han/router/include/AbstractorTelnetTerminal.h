/**@file
* ��Ȩ����(C)2001, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�AbstractorTelnetTerminal.h<br>
* ����ժҪ��<br>
* ����˵�����ο�����ʵ��
* @version 1.0
* @author
* @since  2009-06-04
*/

#ifndef ABSCTRACT_TELNET_TERMINAL_H
#define ABSCTRACT_TELNET_TERMINAL_H

/* ======================== ���� ============================ */
#include <string>

#include "Poco/Types.h"

#include "ITelnetTerminal.h"
/* ======================== �����ռ� ========================= */

NS_VCN_OAM_BEGIN(ROUTER)


class AbstractorTelnetTerminal: public ITelnetTerminal
{

public:

    AbstractorTelnetTerminal();

    virtual ~AbstractorTelnetTerminal();

    /**
    * @brief �����ַ�ת�����ڲ�����  �� EOT BACKSPACE DEL�ȣ��ɲ�����
    */
    virtual Poco::Int16 translateControlCharacter( const Poco::Int16& byteread );
    /**
    * @brief ESCAPE ��������ת�����ڲ�����
    */
    virtual Poco::Int16 translateEscapeSequence( Poco::Int16* buffer );
    /**
    * @brief ���ݹ��ܻ�ȡ��������
    */
    virtual void getEraseSequence( const Poco::Int16& eraseFunc, Poco::Int8* bufferOut, Poco::Int16& outLen  );
    /**
    * @brief ����ƶ���������
    */
    virtual void getCursorMoveSequence( const Poco::Int16& dir, const Poco::Int16& times, Poco::Int8* bufferOut );
    /**
    * @brief ��궨λ��������
    */
    virtual void getCursorPositioningSequence( Poco::Int16* pos, Poco::Int8* bufferOut );
    /**
    * @brief ����������� ----
    */
    virtual void getSpecialSequence( const Poco::Int16& sequence, Poco::Int8* bufferOut, Poco::Int16& outLen );
    /**
    * @brief �������ƣ�
    */
    virtual void getScrollMarginsSequence( const Poco::Int16& topmargin, const Poco::Int16& bottommargin, Poco::Int8* bufferOut );
    /**
    * @brief ��ʾ����
    */
    virtual void getGRSequence( const Poco::Int16& type, const Poco::Int16& param, Poco::Int8* bufferOut );
    /**
    * @brief ������ʾ���Ƹ�ʽ�����(���ӿ�������)
    */
    virtual void format( std::string& str );
    /**
    * @brief ������ʾ���Ƹ�ʽ�����(���ӿ�������)
    */
    virtual void formatBold( std::string& str );
    /**
    * @brief ��ʼ����������
    */
    virtual void getInitSequence( Poco::Int8* bufferOut );
    /**
    * @brief �ն��Ƿ�֧�ֿ��Ƹ�ʽ���������ɫ����ʽ��
    */
    virtual bool supportsSGR();
    /**
    * @brief �ն��Ƿ�֧�ֹ���
    */
    virtual bool supportsScrolling();
    /**
    * @brief ESCAPE ���г���   ��2��
    */
    virtual Poco::Int16 getAtomicSequenceLength();

protected:

private:

};

NS_VCN_OAM_END

#endif
