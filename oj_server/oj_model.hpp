#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <fstream>
#include <unordered_map>
#include <cstdlib>
#include "../comm/log.hpp"
#include "../comm/util.hpp"

// 根据题目list文件，加载所有的题目信息到内存中
// model:主要用来和数据交互，对外提供访问数据的接口

namespace ns_model
{
    using namespace ns_log;
    using namespace std;
    using namespace ns_util;

    const string questions_list = "./questions/questions.list";
    const string questions_path = "./questions/";

    struct Question
    {
        string number; // 题目编号(唯一)
        string title;  // 题目标题
        string star;   // 题目难度
        int cpu_limit; // 时间要求(S)
        int mem_limit; // 空间要求(KB)
        string desc;   // 题目描述
        string header; // 题目预设给用户在线编辑器的代码
        string tail;   // 题目的测试用例,需要和header拼接,形成完整代码
    };
    class Model
    {
    private:
        // 题目 : 题目细节
        unordered_map<string, Question> questions;

    public:
        Model()
        {
            assert(LoadQuestionList(questions_list));
        }

        bool LoadQuestionList(const string &question_lis)
        {
            // 加载配置文件:questions/question.list + 题目编号文件
            fstream in(questions_list);
            if (!in.is_open())
            {
                LOG(FATAL) << " 加载题库失败，请检查是否存在题库文件" << "\n";
                return false;
            }
            std::string line;
            while (getline(in, line))
            {
                std::vector<string> tokens;
                StringUtil::SplitString(line, &tokens, " ");
                if (tokens.size() != 5)
                {
                    LOG(WARNING) << "加载部分题目失败，请检查文件格式" << "\n";
                    continue;
                }
                Question q;
                q.number = tokens[0];
                q.title = tokens[1];
                q.star = tokens[2];
                q.cpu_limit = atoi(tokens[3].c_str());
                q.mem_limit = atoi(tokens[4].c_str());

                std::string question_number_path = questions_path;
                question_number_path += q.number;
                question_number_path += "/";

                FileUtil::ReadFile(question_number_path + "desc.txt", &(q.desc), true);
                FileUtil::ReadFile(question_number_path + "header.cpp", &(q.header), true);
                FileUtil::ReadFile(question_number_path + "tail.cpp", &(q.tail), true);

                questions.insert({q.number, q});
            }

            LOG(INFO) << "加载题库成功" << "\n";
            in.close();
            return true;
        }

        bool GetAllQuestions(vector<Question> *out)
        {
            if (questions.size() == 0)
            {
                LOG(ERROR) << "用户获取题库失败" << "\n";
                return false;
            }
            for (const auto &q : questions)
            {
                out->push_back(q.second);
            }
            return true;
        }

        bool GetOneQuestion(const std::string &number, Question *q)
        {
            const auto &iter = questions.find(number);
            if (iter == questions.end())
            {
                LOG(ERROR) << "用户获取题目失败, 题目编号: " << number << "\n";
                return false;
            }
            (*q) = iter->second;
            return true;
        }
        ~Model() {}
    };
}