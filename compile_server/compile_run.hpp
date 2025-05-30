#pragma once

#include "compiler.hpp"
#include "runner.hpp"
#include "../comm/log.hpp"
#include "../comm/util.hpp"

#include <signal.h>
#include <unistd.h>
#include <json/json.h>

namespace ns_compile_and_run
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_compile;
    using namespace ns_run;

    class CompileAndRun
    {
    public:
        // 辅助函数：提取 class Solution {} 内部的全部内容
        static std::string ExtractSolutionClassBody(const std::string &code)
        {
            const std::string class_start = "public:";
            size_t class_pos = code.find(class_start);
            if (class_pos == std::string::npos)
            {
                return "";
            }

            // 定位类的 { 和 }
            size_t brace_open = code.find('{', class_pos);
            size_t brace_close = code.find('}');
            if (brace_open == std::string::npos || brace_close == std::string::npos)
            {
                return "";
            }
            return code.substr(brace_open + 1, brace_close - brace_open - 1);
        }

        // 检查类内部是否有有效代码（忽略注释和空白）
        static bool HasValidCodeInSolution(const std::string &code)
        {
            std::string class_body = ExtractSolutionClassBody(code);
            if (class_body.empty())
            {
                return false; // 无 Solution 类或类为空
            }

            // 检查是否存在非注释、非空白的字符
            std::istringstream iss(class_body);
            std::string line;
            bool has_real_code = false;

            while (std::getline(iss, line))
            {
                // 移除行首行尾空白
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);

                // 如果不是空行且不是注释行
                if (!line.empty() && line.find("//") != 0)
                {
                    has_real_code = true;
                    break;
                }
            }
            return has_real_code;
        }

        static void RemoveTempFile(const std::string &file_name)
        {
            // 清理文件的个数是不确定的，但是有哪些我们是知道的
            std::string _src = PathUtil::Src(file_name);
            if (FileUtil::IsFileExists(_src))
                unlink(_src.c_str());

            std::string _compiler_error = PathUtil::Compiler_error(file_name);
            if (FileUtil::IsFileExists(_compiler_error))
                unlink(_compiler_error.c_str());

            std::string _execute = PathUtil::Exe(file_name);
            if (FileUtil::IsFileExists(_execute))
                unlink(_execute.c_str());

            std::string _stdin = PathUtil::Stdin(file_name);
            if (FileUtil::IsFileExists(_stdin))
                unlink(_stdin.c_str());

            std::string _stdout = PathUtil::Stdout(file_name);
            if (FileUtil::IsFileExists(_stdout))
                unlink(_stdout.c_str());

            std::string _stderr = PathUtil::Stderr(file_name);
            if (FileUtil::IsFileExists(_stderr))
                unlink(_stderr.c_str());
        }
        // code > 0 : 进程收到了信号导致异常奔溃
        // code < 0 : 整个过程非运行报错(代码为空，编译报错等)
        // code = 0 : 整个过程全部完成
        static std::string CodeToDesc(int code, const std::string &file_name)
        {
            std::string desc;
            switch (code)
            {
            case 0:
                desc = "编译运行成功";
                break;
            case -1:
                desc = "提交的代码是空";
                break;
            case -2:
                desc = "未知错误";
                break;
            case -3:
                // desc = "代码编译的时候发生了错误";
                FileUtil::ReadFile(PathUtil::Compiler_error(file_name), &desc, true);
                break;
            case SIGABRT: // 6
                desc = "内存超过范围";
                break;
            case SIGXCPU: // 24
                desc = "CPU使用超时";
                break;
            case SIGFPE: // 8
                desc = "浮点数溢出";
                break;
            default:
                desc = "未知: " + std::to_string(code);
                break;
            }

            return desc;
        }

        /***************************************
         * 输入:
         * code： 用户提交的代码
         * input: 用户给自己提交的代码对应的输入，不做处理
         * cpu_limit: 时间要求
         * mem_limit: 空间要求
         *
         * 输出:
         * 必填
         * status: 状态码
         * reason: 请求结果
         * 选填：
         * stdout: 我的程序运行完的结果
         * stderr: 我的程序运行完的错误结果
         *
         * 参数：
         * in_json: {"code": "#include...", "input": "","cpu_limit":1, "mem_limit":10240}
         * out_json: {"status":"0", "reason":"","stdout":"","stderr":"",}
         * ************************************/
        static void Start(const std::string &in_json, std::string *out_json)
        {

            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value); // 最后在处理差错问题

            std::string code = in_value["code"].asString();
            std::string input = in_value["input"].asString();
            int cpu_limit = in_value["cpu_limit"].asInt();
            int mem_limit = in_value["mem_limit"].asInt();
            int status_code = 0;
            Json::Value out_value;
            int run_result = 0;
            std::string file_name; // 需要内部形成的唯一文件名

            // 检查有效代码内容（忽略注释和空行）
            if (!HasValidCodeInSolution(code))
            {
                status_code = -1;
                goto END;
            }
            // 形成的文件名只具有唯一性，没有目录没有后缀
            // 毫秒级时间戳+原子性递增唯一值: 来保证唯一性
            file_name = FileUtil::UniqFileName();
            // 形成临时src文件
            if (!FileUtil::WriteFile(PathUtil::Src(file_name), code))
            {
                status_code = -2; // 未知错误
                LOG(ERROR) << "打开文件失败,文件名为: " << file_name << std::endl;
                goto END;
            }

            if (!Compiler::Compile(file_name))
            {
                // 编译失败
                status_code = -3; // 代码编译的时候发生了错误
                goto END;
            }

            run_result = Runner::Run(file_name, cpu_limit, mem_limit);
            if (run_result < 0)
            {
                status_code = -2; // 未知错误
                LOG(ERROR) << "运行失败,文件名为: " << file_name << std::endl;
            }
            else if (run_result > 0)
            {
                // 程序运行崩溃了
                status_code = run_result;
            }
            else
            {
                // 运行成功
                status_code = 0;
            }
        END:
            out_value["status"] = status_code;
            out_value["reason"] = CodeToDesc(status_code, file_name);
            if (status_code == 0)
            {
                // 整个过程全部成功
                std::string _stdout;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), &_stdout, true);
                out_value["stdout"] = _stdout;

                std::string _stderr;
                FileUtil::ReadFile(PathUtil::Stderr(file_name), &_stderr, true);
                out_value["stderr"] = _stderr;
            }

            Json::StyledWriter writer;
            *out_json = writer.write(out_value);

            RemoveTempFile(file_name);
        }
    };
}
