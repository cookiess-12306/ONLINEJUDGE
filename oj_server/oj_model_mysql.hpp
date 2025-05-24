#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <cstdlib>
// #include <cppconn/driver.h>
// #include <cppconn/exception.h>
// #include <cppconn/resultset.h>
// #include <cppconn/statement.h>
// #include <cppconn/prepared_statement.h>
#include <cassert>
#include <memory>
#include "../comm/log.hpp"
#include "../comm/util.hpp"
#include <mysql/mysql.h>

namespace ns_model_mysql
{
    using namespace std;
    using namespace ns_log;
    using namespace ns_util;

    struct Question
    {
        std::string number; // 题目编号，唯一
        std::string title;  // 题目的标题
        std::string star;   // 难度: 简单 中等 困难
        std::string desc;   // 题目的描述
        std::string header; // 题目预设给用户在线编辑器的代码
        std::string tail;   // 题目的测试用例，需要和header拼接，形成完整代码
        int cpu_limit;      // 题目的时间要求(S)
        int mem_limit;      // 题目的空间要去(KB)
    };

    const std::string oj_questions = "oj_question";
    const std::string host = "121.43.228.177";
    const std::string user = "oj_client";
    const std::string passwd = "123456";
    const std::string db = "oj";
    const int port = 3306;

    class Model
    {
    public:
        Model()
        {
        }

        bool QueryMySql(const std::string &sql, vector<Question> *out)
        {
            // 创建mysql句柄
            MYSQL *my = mysql_init(nullptr);

            // 连接数据库
            if (mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
            {
                LOG(FATAL) << "连接数据库失败!" << "\n";
                return false;
            }

            mysql_set_character_set(my, "utf8");

            LOG(INFO) << "连接数据库成功!" << "\n";

            // 执行sql语句
            if (0 != mysql_query(my, sql.c_str()))
            {
                LOG(WARNING) << sql << " execute error!" << "\n";
                return false;
            }

            // 提取结果
            MYSQL_RES *res = mysql_store_result(my);

            // 分析结果
            int rows = mysql_num_rows(res);   // 获得行数量
            int cols = mysql_num_fields(res); // 获得列数量

            Question q;

            for (int i = 0; i < rows; i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                q.number = row[0];
                q.title = row[1];
                q.star = row[2];
                q.desc = row[3];
                q.header = row[4];
                q.tail = row[5];
                q.cpu_limit = atoi(row[6]);
                q.mem_limit = atoi(row[7]);

                out->push_back(q);
            }
            // 释放结果空间
            mysql_free_result(res);
            // 关闭mysql连接
            mysql_close(my);

            return true;
        }
        bool GetAllQuestions(vector<Question> *out)
        {
            std::string sql = "select * from ";
            sql += oj_questions;
            return QueryMySql(sql, out);
        }
        bool GetOneQuestion(const std::string &number, Question *q)
        {
            bool res = false;
            std::string sql = "select * from ";
            sql += oj_questions;
            sql += " where number=";
            sql += number;
            vector<Question> result;
            if (QueryMySql(sql, &result))
            {
                if (result.size() == 1)
                {
                    *q = result[0];
                    res = true;
                }
            }
            return res;
        }
        ~Model()
        {
        }
    };
}