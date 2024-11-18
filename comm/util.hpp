#pragma once
#include <stdio.h>
#include <iostream>
#include <atomic>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>
#include <boost/algorithm/string.hpp>

namespace ns_util
{
    const std::string temp_path = "./temp/";

    class TimeUtil
    {
    public:
        static std::string GetTime()
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return std::to_string(tv.tv_sec);
        }

        static std::string GetTimeMs()
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return std::to_string(tv.tv_sec * 1000 + tv.tv_usec / 1000);
        }
    };

    class PathUtil
    {
    public:
        static std::string AddSuffix(const std::string &file_name, const std::string &suffix)
        {
            FILE* fh = fopen( "temp" , "r" );
            if (fh == NULL)
            {
                int n = ::mkdir(temp_path.c_str(), 0777);
            }
            std::string tmp = temp_path;
            tmp += file_name;
            tmp += suffix;
            return tmp;
        }
        static std::string Src(const std::string &file_name)
        {
            return AddSuffix(file_name, ".cpp");
        }
        static std::string Exe(const std::string &file_name)
        {
            return AddSuffix(file_name, ".exe");
        }
        static std::string Compiler_error(const std::string &file_name)
        {
            return AddSuffix(file_name, ".compiler_error");
        }

        static std::string Stdin(const std::string &file_name)
        {
            return AddSuffix(file_name, ".Stdin");
        }
        static std::string Stdout(const std::string &file_name)
        {
            return AddSuffix(file_name, ".Stdout");
        }
        static std::string Stderr(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stderr");
        }
    };

    class FileUtil
    {
    public:
        static bool IsFileExists(const std::string &Path_name)
        {
            struct stat st;
            if (stat(Path_name.c_str(), &st) == 0)
            {
                return true;
            }
            return false;
        }

        static std::string UniqFileName()
        {
            std::atomic_uint id(0);
            id++;
            // 方法:毫秒级时间戳+原子性递增唯一值
            std::string time = TimeUtil::GetTimeMs();
            std::string uniq_id = std::to_string(id);
            return time + "_" + uniq_id;
        }

        static bool WriteFile(const std::string &target, const std::string &content)
        {
            std::ofstream out(target);
            if (!out.is_open())
            {
                return false;
            }
            out.write(content.c_str(), content.size());
            out.close();
            return true;
        }

        static bool ReadFile(const std::string &target, std::string *content, bool keep = false)
        {
            (*content).clear();
            std::ifstream in(target);
            if (!in.is_open())
                return false;
            std::string line;
            while (std::getline(in, line))
            {
                (*content) += line;
                (*content) += keep ? "\n" : "";
            }
            in.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        /***********************
         * str : 需要切割的字符串
         * target : 输出型, 保存切分完毕的结果
         * sep : 指定的分隔符
         * 
        ************************/
        static void SplitString(const std::string &str, std::vector<std::string> *target, std::string sep)
        {
            boost::split((*target), str, boost::is_any_of(sep), boost::algorithm::token_compress_on);
        }
    };

}
