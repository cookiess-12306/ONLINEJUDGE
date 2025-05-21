#pragma once
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../comm/util.hpp"
#include "../comm/log.hpp"

namespace ns_run
{
    using namespace ns_util;
    using namespace ns_log;
    class Runner
    {
    public:
        Runner() {}
        ~Runner() {}

    public:
        static void SetProcLimit(int _cpu_limit, int _mem_limit)
        {
            struct rlimit cpu_limit;
            cpu_limit.rlim_max = RLIM_INFINITY;
            cpu_limit.rlim_cur = _cpu_limit;
            setrlimit(RLIMIT_CPU, &cpu_limit);

            struct rlimit mem_limit;
            mem_limit.rlim_max = RLIM_INFINITY;
            mem_limit.rlim_cur = _mem_limit * 1024; //(KB)
            setrlimit(RLIMIT_AS, &mem_limit);
        }

        /******************************
         *
         * 返回值 > 0 : 异常，收到了异常信号，返回值就是对应的信息编号
         * 返回值 == 0 ： 正常退出
         * 返回值 < 0 : 内部错误
         *
         *
         * cpu_limit : 程序运行时候，可以使用的最大的cpu资源上限
         * mem_limit : 程序运行时候，可以使用的最大的内存大小（kb）
         * ****************************/
        static int Run(const std::string &file_name, int cpu_limit, int mem_limit)
        {
            /**************************************
             *
             * 程序运行
             * 1.代码跑完，运行正确
             * 2.代码跑完，运行错误
             * 3.代码没跑完，异常了
             * 不关心代码正确，是否正确，是由测试用例决定的
             *
             *
             * 前提得知道可执行程序是谁？？
             * 程序默认启动的时候
             * 标准输入：不处理
             * 标准输出：程序运行完成，输出结果
             * 标准错误：运行时错误信息
             ****************************************/
            std::string _execute = PathUtil::Exe(file_name);
            std::string _stdin = PathUtil::Stdin(file_name);
            std::string _stdout = PathUtil::Stdout(file_name);
            std::string _stderr = PathUtil::Stderr(file_name);

            umask(0);
            int _stdin_fd = open(_stdin.c_str(), O_CREAT | O_RDONLY, 0644);
            int _stdout_fd = open(_stdout.c_str(), O_CREAT | O_WRONLY, 0644);
            int _stderr_fd = open(_stderr.c_str(), O_CREAT | O_WRONLY, 0644);
            if (_stdin_fd < 0 || _stdout_fd < 0 || _stderr_fd < 0)
            {
                LOG(ERROR) << "运行时打开文件失败" << "\n";
                return -1; // 代表文件打开失败
            }

            pid_t pid = fork();

            if (pid < 0)
            {
                LOG(ERROR) << "运行时创建子进程失败" << "\n";
                // 进程创建失败了
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderr_fd);
                return -2; // 代表创建子进程失败
            }

            else if (pid == 0)
            {
                // 重定向stdin, stdout, stderr
                dup2(_stdin_fd, 0);
                dup2(_stdout_fd, 1);
                dup2(_stderr_fd, 2);
                SetProcLimit(cpu_limit, mem_limit); // 仅影响子进程！！！！

                execl(_execute.c_str() /*执行谁*/, _execute.c_str() /*在命令行如何执行该程序*/, nullptr);
                exit(1);
            }

            else
            {
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderr_fd);
                int status = 0;
                waitpid(pid, &status, 0);
                LOG(INFO) << "运行完毕，info: " << (status & 0X7F) << "\n";

                // 程序运行异常，是因为收到了信号
                return status & 0x7F;
            }
        }
    };
}