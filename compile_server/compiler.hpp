#pragma once
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../comm/util.hpp"
#include "../comm/log.hpp"

namespace ns_compile
{
    using namespace ns_util;
    using namespace ns_log;
    class Compiler
    {
    public:
        Compiler() {}
        ~Compiler() {}

        // 编译功能
        // 生成可执行文件
        static bool Compile(const std::string &file_name)
        {
            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(ERROR) << "内部错误，创建子进程失败" << "\n";
                return false;
            }
            else if (pid == 0)
            {
                umask(0);
                int _stderr = open(PathUtil::Compiler_error(file_name).c_str(), O_CREAT | O_WRONLY, 0644);
                if (_stderr < 0) // 创建失败了
                {
                    LOG(WARNING) << "没有成功形成stderr文件" << "\n";
                    exit(1);
                }
                // 如果成功了，将本来要写在屏幕上的错误信息重定向到stderr文件中
                dup2(_stderr, 2); // dup2(int oldfd, int newfd)将new作为old的一份拷贝，把本来应该写入到2号文件描述符的数据，写入到_stderr中
                // 程序替换，不影响进程文件的描述符表
                // 子进程
                // 调用编译器，用"g++ -o target src -std=c++11"格式
                execlp("g++", "g++", "-o", PathUtil::Exe(file_name).c_str(), PathUtil::Src(file_name).c_str(), "-std=c++11", "-D", "COMPILER_ONLINE", "-Werror=return-type", nullptr);
                LOG(ERROR) << "启动编译器g++失败，可能是参数错误" << "\n";
                exit(2);
            }
            else
            {
                // 父进程
                // 等待子进程
                waitpid(pid, nullptr, 0);

                // 判断编译是否成功，取决于是否生成同名的.exe文件
                if (FileUtil::IsFileExists(PathUtil::Exe(file_name)))
                {
                    LOG(INFO) << PathUtil::Src(file_name) << "编译成功" << "\n";
                    return true;
                }
            }
            LOG(ERROR) << "编译失败，没有形成可执行程序" << "\n";
            return false;
        }
    };
}