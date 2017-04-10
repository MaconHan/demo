/**@file
* ��Ȩ����(C)2001, ����������ͨѶ�ɷ����޹�˾<br>
* �ļ����ƣ�ITelnetTerminal.h<br>
* ����ժҪ��<br>
* ����˵�����ο�����ʵ��
* @version 1.0
* @author
* @since  2009-06-04
*/

#ifndef I_IO_WRAPPER_H
#define I_IO_WRAPPER_H

/* ======================== ���� ============================ */
#include <string>

#include "Poco/Types.h"

#include "vcn_defs.h"
/* ======================== �����ռ� ========================= */

NS_VCN_OAM_BEGIN(ROUTER)


class IIOWrapper
{

public:

    IIOWrapper() {}

    virtual ~IIOWrapper() {}

    virtual Poco::Int16 read() = 0; /**< ��ȡIO����,�����첽��ʽ����Ҫ������δ���*/
    virtual void write( const Poco::Int8& b ) = 0;/**< дһ��byte*/
    virtual void write( const std::string& str ) = 0;/**< д�ַ���*/
    //virtual void setCursor( const Poco::Int16& row, const Poco::Int16& col ) = 0; /**<�������λ��*/

    virtual void setCursor( const Poco::Int16& row, const Poco::Int16& col ) {} /**<�������λ��*/

    virtual void moveCursor( const Poco::Int16& direction, const Poco::Int16& times ) = 0;/**<�ƶ����λ��*/
    //virtual void homeCursor() = 0;/**<HOME*/
    virtual void homeCursor() {}/**<HOME*/

    virtual void storeCursor() = 0; /**<�������λ��*/
    virtual void restoreCursor() = 0;/**<�ָ��ϴ����λ��*/
    virtual void eraseToEndOfLine() = 0;/**<�ӵ�ǰ���ɾ����END of line*/
    virtual void eraseToBeginOfLine() = 0;/**<�ӵ�ǰ���ɾ����Begin of line*/
    virtual void eraseLine() = 0;/**<ɾ����ǰ��*/
    virtual void eraseToEndOfScreen() = 0;/**<ɾ������Ļ����*/
    virtual void eraseToBeginOfScreen() = 0;/**<*/
    virtual void eraseScreen() = 0;/**<ɾ����ǰ��Ļ*/
    virtual void bell() = 0;/**< ������ʾ*/
    //virtual void flush() = 0; /**< ǿ�����*/
    virtual void close() = 0;/**< �ر�*/
    virtual void setTerminal( std::string& terminalname ) = 0;/**< �ն����ͣ���Э�̵�ʱ��ȷ��*/
    virtual void setDefaultTerminal() = 0;/**< �ն����ͣ���Э�̵�ʱ��ȷ��*/
    virtual Poco::UInt16 getRows() = 0;/**< ��ȡ�ն���ʾ�ĸ߶ȣ���������֧���޸ģ�ʵ���ϲ�֧���޸�*/
    virtual Poco::UInt16 getColumns() = 0;/**< ��ȡ�ն���ʾ�ģ���ȣ�������֧���޸ģ�ʵ���ϲ�֧���޸�*/
    virtual void setSignalling( bool b ) = 0;/**< ���÷���ʾ����Ĭ��֧��*/
    virtual bool isSignalling() = 0;/**< �Ƿ���ʾ����Ĭ��֧��*/
    //virtual void setAutoflushing( bool b );
    //virtual bool isAutoflushing();


    //���²��ֹ��ܣ��ݲ�ʵ�֣��Ǵ��麯��
    virtual void setForegroundColor( const Poco::Int16& color ) {}/**<����������ɫ*/

    virtual void setBackgroundColor( const Poco::Int16& color ) {}/**<���ñ���ɫ*/

    virtual void setBold( bool b ) {}/**<����*/

    virtual void forceBold( bool b ) {}/**<����*/

    virtual void setItalic( bool b ) {}/**<б��*/

    virtual void setUnderlined( bool b ) {}/**<�»���*/

    virtual void setBlink( bool b ) {} /**<�����˸*/

    virtual void resetAttributes()  {} /**< �ָ�����*/

    virtual void resetTerminal() {}/**< �����ն�*/

    virtual void setLinewrapping( bool b ) {}/**< ����*/

    virtual bool isLineWrapping()
    {
        return false;
    }/**< ����*/

    virtual bool defineScrollRegion( const Poco::Int16& topmargin, const Poco::Int16& bottommargin )
    {
        return false;
    }


protected:

private:

};

NS_VCN_OAM_END

#endif

