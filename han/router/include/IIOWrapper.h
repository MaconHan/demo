/**@file
* 版权所有(C)2001, 深圳市中兴通讯股份有限公司<br>
* 文件名称：ITelnetTerminal.h<br>
* 内容摘要：<br>
* 其它说明：参考现有实现
* @version 1.0
* @author
* @since  2009-06-04
*/

#ifndef I_IO_WRAPPER_H
#define I_IO_WRAPPER_H

/* ======================== 引用 ============================ */
#include <string>

#include "Poco/Types.h"

#include "vcn_defs.h"
/* ======================== 命名空间 ========================= */

NS_VCN_OAM_BEGIN(ROUTER)


class IIOWrapper
{

public:

    IIOWrapper() {}

    virtual ~IIOWrapper() {}

    virtual Poco::Int16 read() = 0; /**< 读取IO数据,采用异步方式，需要考虑如何处理*/
    virtual void write( const Poco::Int8& b ) = 0;/**< 写一个byte*/
    virtual void write( const std::string& str ) = 0;/**< 写字符串*/
    //virtual void setCursor( const Poco::Int16& row, const Poco::Int16& col ) = 0; /**<设置鼠标位置*/

    virtual void setCursor( const Poco::Int16& row, const Poco::Int16& col ) {} /**<设置鼠标位置*/

    virtual void moveCursor( const Poco::Int16& direction, const Poco::Int16& times ) = 0;/**<移动鼠标位置*/
    //virtual void homeCursor() = 0;/**<HOME*/
    virtual void homeCursor() {}/**<HOME*/

    virtual void storeCursor() = 0; /**<保存鼠标位置*/
    virtual void restoreCursor() = 0;/**<恢复上次鼠标位置*/
    virtual void eraseToEndOfLine() = 0;/**<从当前鼠标删除至END of line*/
    virtual void eraseToBeginOfLine() = 0;/**<从当前鼠标删除至Begin of line*/
    virtual void eraseLine() = 0;/**<删除当前行*/
    virtual void eraseToEndOfScreen() = 0;/**<删除至屏幕结束*/
    virtual void eraseToBeginOfScreen() = 0;/**<*/
    virtual void eraseScreen() = 0;/**<删除当前屏幕*/
    virtual void bell() = 0;/**< 声音提示*/
    //virtual void flush() = 0; /**< 强制输出*/
    virtual void close() = 0;/**< 关闭*/
    virtual void setTerminal( std::string& terminalname ) = 0;/**< 终端类型，在协商的时候确定*/
    virtual void setDefaultTerminal() = 0;/**< 终端类型，在协商的时候确定*/
    virtual Poco::UInt16 getRows() = 0;/**< 获取终端显示的高度（行数），支持修改？实际上不支持修改*/
    virtual Poco::UInt16 getColumns() = 0;/**< 获取终端显示的（宽度）列数，支持修改？实际上不支持修改*/
    virtual void setSignalling( bool b ) = 0;/**< 设置发提示音，默认支持*/
    virtual bool isSignalling() = 0;/**< 是否发提示音，默认支持*/
    //virtual void setAutoflushing( bool b );
    //virtual bool isAutoflushing();


    //以下部分功能，暂不实现，非纯虚函数
    virtual void setForegroundColor( const Poco::Int16& color ) {}/**<设置字体颜色*/

    virtual void setBackgroundColor( const Poco::Int16& color ) {}/**<设置背景色*/

    virtual void setBold( bool b ) {}/**<粗体*/

    virtual void forceBold( bool b ) {}/**<粗体*/

    virtual void setItalic( bool b ) {}/**<斜体*/

    virtual void setUnderlined( bool b ) {}/**<下划线*/

    virtual void setBlink( bool b ) {} /**<鼠标闪烁*/

    virtual void resetAttributes()  {} /**< 恢复设置*/

    virtual void resetTerminal() {}/**< 重置终端*/

    virtual void setLinewrapping( bool b ) {}/**< 换行*/

    virtual bool isLineWrapping()
    {
        return false;
    }/**< 换行*/

    virtual bool defineScrollRegion( const Poco::Int16& topmargin, const Poco::Int16& bottommargin )
    {
        return false;
    }


protected:

private:

};

NS_VCN_OAM_END

#endif

