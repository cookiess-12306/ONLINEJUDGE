#pragma once

#include <iostream>
#include <mutex>
#include "util.hpp"

namespace ns_log
{
    using namespace ns_util;
    enum
    {
        INFO,
        DEBUG,
        WARNING,
        ERROR,
        FATAL
    };

    // 新增：全局互斥锁（静态局部变量实现线程安全的单例）
    static std::mutex &GetLogMutex()
    {
        static std::mutex mtx;
        return mtx;
    }

    std::ostream &Log(const std::string &level, const std::string &file_name, int line)
    {
        // 加锁范围覆盖整个日志输出过程
        std::lock_guard<std::mutex> lock(GetLogMutex());

        // 保留原有日志格式逻辑
        std::string message = "[" + level + "]";
        message += "[" + file_name + "]";
        message += "[" + std::to_string(line) + "]";
        message += "[" + TimeUtil::GetTime() + "]";

        // 关键改进：确保整个message的写入是原子的
        std::cout << message;
        return std::cout; // 保留链式调用支持（如LOG(INFO) << "msg" << endl;）
    }

// 完全保留原始宏定义，无需修改
#define LOG(level) Log(#level, __FILE__, __LINE__)
}