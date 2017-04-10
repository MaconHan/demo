/**@file
* ��Ȩ����(C)2001, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�ITelnetTerminal.h<br>
* ����ժҪ��<br>
* ����˵�����ο�����ʵ��
* @version 1.0
* @author
* @since  2009-06-04
*/

#ifndef I_TELNET_TERMINAL_H
#define I_TELNET_TERMINAL_H

/* ======================== ���� ============================ */
#include <string>

#include "Poco/Types.h"

#include "vcn_defs.h"
/* ======================== �����ռ� ========================= */

NS_VCN_OAM_BEGIN(ROUTER)

class ITelnetTerminal
{

public:

    ITelnetTerminal() {}

    virtual ~ITelnetTerminal() {}

    /**
    * @brief �����ַ�ת�����ڲ�����  �� EOT BACKSPACE DEL�ȣ��ɲ�����
    */
    virtual Poco::Int16 translateControlCharacter( const Poco::Int16& byteread ) = 0;
    /**
    * @brief ESCAPE ��������ת�����ڲ�����
    * @TODO Poco::Int16 (&buffer)[2] ??
    */
    virtual Poco::Int16 translateEscapeSequence( Poco::Int16* buffer ) = 0;
    /**
    * @brief ���ݹ��ܻ�ȡ��������
    */
    virtual void getEraseSequence( const Poco::Int16& eraseFunc, Poco::Int8* bufferOut, Poco::Int16& outLen ) = 0;
    /**
    * @brief ����ƶ���������
    */
    virtual void getCursorMoveSequence( const Poco::Int16& dir, const Poco::Int16& times, Poco::Int8* bufferOut ) = 0;
    /**
    * @brief ��궨λ��������
    */
    virtual void getCursorPositioningSequence( Poco::Int16* pos, Poco::Int8* bufferOut ) = 0;
    /**
    * @brief ����������� ----
    */
    virtual void getSpecialSequence( const Poco::Int16& sequence, Poco::Int8* bufferOut, Poco::Int16& outLen  ) = 0;
    /**
    * @brief �������ƣ�
    */
    virtual void getScrollMarginsSequence( const Poco::Int16& topmargin, const Poco::Int16& bottommargin, Poco::Int8* bufferOut ) = 0;
    /**
    * @brief ��ʾ����
    */
    virtual void getGRSequence( const Poco::Int16& type, const Poco::Int16& param, Poco::Int8* bufferOut ) = 0;
    /**
    * @brief ������ʾ���Ƹ�ʽ�����(���ӿ�������)
    */
    virtual void format( std::string& str ) = 0;
    /**
    * @brief ������ʾ���Ƹ�ʽ�����(���ӿ�������)
    */
    virtual void formatBold( std::string& str ) = 0;
    /**
    * @brief ��ʼ����������
    */
    virtual void getInitSequence( Poco::Int8* bufferOut ) = 0;
    /**
    * @brief �ն��Ƿ�֧�ֿ��Ƹ�ʽ���������ɫ����ʽ��
    */
    virtual bool supportsSGR() = 0;
    /**
    * @brief �ն��Ƿ�֧�ֹ���
    */
    virtual bool supportsScrolling() = 0;
    /**
    * @brief ESCAPE ���г���   ��2��
    */
    virtual Poco::Int16 getAtomicSequenceLength() = 0;



protected:

private:

};

NS_VCN_OAM_END
#endif
