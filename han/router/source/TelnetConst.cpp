/***@file
* 版权所有(C)2008, 深圳市中兴通讯股份有限公司<br>
* 文件名称：MMLConst.cpp<br>
* 文件标识：<br>
* 内容摘要：<br>
* 其它说明：<br>
* @version v1.0<br>
* @author
* @since 2009-06-04
*/
#include <string>
#include <sstream>
#include <utility>
#include <functional>
#include <wchar.h>
#include <string.h>

#include "Poco/Format.h"

#include "TelnetConst.h"
#include "utf8.h"

NS_VCN_OAM_BEGIN(ROUTER)

std::string Message_Tip();

const std::string TelnetConst::CRLF = "\r\n";
const std::string TelnetConst::TELNET_ROOT      = "zte";
const std::string TelnetConst::TELNET_SUFFIX    = ":>";
const std::string TelnetConst::TELNET_PROMPT    = TelnetConst::TELNET_ROOT + TelnetConst::TELNET_SUFFIX;
const std::string TelnetConst::INIT_MESSAGE     = Message_Tip();
const std::string TelnetConst::LOGIN_PROMPT     = "login username:";
const std::string TelnetConst::PASSWORD_PROMPT  = "password:";
const std::string TelnetConst::PASSWORD_ASTERISK= "***********************";
const std::string TelnetConst::MORE             = "---More---";
const std::string TelnetConst::NEST_PREFIX      = "> ";

std::string Message_Tip()
{
    #ifdef VERSION
    #define M_VERSION "" VERSION
    #else
    #define M_VERSION ""
    #endif
    
    #ifdef PAASVERSION
    #define PAAS_VERSION "" PAASVERSION
    #else
    #define PAAS_VERSION ""
    #endif

    #define BUILD_DATE  __TIME__ " " __DATE__
    
    std::stringstream stream;
    stream  << "OAM Router(OAM System Terminal), " << BUILD_DATE << ", gcc " << __VERSION__ << TelnetConst::CRLF
            << "Version: " << M_VERSION << ", PaaS" << PAAS_VERSION << TelnetConst::CRLF
            << "Type \"!help\" for more information" << TelnetConst::CRLF;
    return stream.str();
}

/**
* @details
* 函数名称：getUTF8StrANSILength<br>
* 功能描述：获取UTF-8在屏幕上显示的长度(一个中文占两个)，如果不是UTF-8的，直接返回str.length()<br>
* 其它说明：<br>
* @version v1.0
* @see N/A
* @param str    UTF-8字符串
* @retrun Poco::UInt16 字符宽度
*/
Poco::UInt16 TelnetConst::getUTF8StrANSILength(const std::string& str)
{
    Poco::UInt16 ansi_len = 0;
    try{
        std::string::difference_type distance;
        std::string::const_iterator start= str.begin();
        std::string::const_iterator find = start;
        while (find != str.end()){
            utf8::uint32_t code = utf8::next(find, str.end());
            distance    = find - start;
            start       = find;
            ansi_len    += (distance > 1 ? 2 : distance);
        }
    }
    catch (...){
        ansi_len = str.length();
    }

    return ansi_len;
}

std::size_t TelnetConst::screen_getline(std::size_t max_columns, const char* str, std::size_t size, char *output)
{
    const std::size_t LINE_SIZE = 512;
    char line[LINE_SIZE + 4];
    char *l = line, *end_l = l + LINE_SIZE;
    mbstate_t mbs, mbs_bak;
    memset(&mbs_bak, '\0', sizeof(mbstate_t));
    mbs = mbs_bak;

    std::size_t column  = 0;
    const char* p       = str;
    const char *end_p   = str + size;
    while(p < end_p && l < end_l && column < max_columns){
        if (*p == '\t'){
            column += 1 + ((column & 3) ^ 3);
            *l++ = *p++;
        }
        else if (*p != '\r' && *p != '\n'){
            std::size_t n = (*p >= 0 ? 1 : mbrtowc(NULL, p, (end_p - p), &mbs));
            switch(n){
            case (std::size_t)-1:
            case (std::size_t)-2:
                mbs = mbs_bak;
            case 1:
                column += std::isprint(*p) ? 1 : 0;
                *l++ = *p++;
                break;
            case 0:
                p++;
                break;
            default:
                column += 2;
                if (column >= max_columns)
                    break;
                for(auto i = 0; i < n && l < end_l; ++i) *l++ = *p++;
                break;
            }
        }

        if (*p == '\n'){
            *l++ = '\r';
            *l++ = *p++;
            break;
        }
        else if (*p == '\r'){
            *l++ = *p++;
            if (*p == '\n'){
                *l++ = *p++;
                break;
            }

            column = 0;
        }
    }

    *l++ = '\0';
    if (output) memcpy(output, line, l - line);
    return p - str;
}

std::size_t TelnetConst::screen_getline_num(std::size_t max_columns, const std::string &str)
{
    std::size_t rows = 0;
    const char *p       = str.c_str();
    std::size_t size    = str.size();
    while(size){
        auto n = screen_getline(max_columns, p, size, NULL);
        size -= n;
        p    += n;
        ++rows;
    }
    return rows;
}

bool TelnetConst::screen_full(std::size_t max_columns, std::size_t max_rows, const std::string &str)
{
    std::size_t lines   = 0;
    const char *p       = str.c_str();
    std::size_t size    = str.size();
    while(size){
        auto n = screen_getline(max_columns, p, size, NULL);
        size -= n;
        p    += n;
        if (++lines == max_rows)
            return true;
    }

    return false;
}

std::string TelnetConst::screen_getlines(std::size_t max_columns, std::size_t max_rows, std::string &str)
{
    std::string output;
    output.reserve(max_columns * max_rows);

    Poco::UInt16 print_line = 0;
    const char *p   = str.c_str();
    std::size_t len = str.size();
    char line[512];
    do{
        auto n = TelnetConst::screen_getline(max_columns, p, len, line);
        output.append(line);

        p   += n;
        len -= n;
    }while((++print_line < max_rows - 1) && len);
    if (len){
        std::size_t n = strlen(line);
        if (n && line[n - 1] != '\n')
            output.append(TelnetConst::CRLF);
        output.append(TelnetConst::MORE);
    }

    str.erase(str.begin(), str.begin() + (str.size() - len));
    return std::move(output);
}

NS_VCN_OAM_END
