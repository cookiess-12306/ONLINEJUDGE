#pragma once

#include <iostream>
#include <string>
#include "util.hpp"

namespace ns_log{
    using namespace ns_util;
    enum{
        INFO,
        DEBUG,
        WARNING,
        ERROR,
        FATAL
    };

    std::ostream& Log(const std::string& level, const std::string& file_name, int line)
    {
        //添加日志等级
        std::string message = "[";
        message += level;
        message += "]";

        //添加文件信息
        message += "[";
        message += file_name;
        message += "]";

        //添加行数
        message += "[";
        message += std::to_string(line);
        message += "]";

        //添加日志时间
        message += "[";
        message += TimeUtil::GetTime();
        message += "]";

        std::cout << message;
        return std::cout;

    }
    #define LOG(level) Log(#level, __FILE__, __LINE__)
}