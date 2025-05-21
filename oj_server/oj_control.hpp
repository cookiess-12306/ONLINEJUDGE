#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <cassert>
#include <json/json.h>
#include <chrono>

#include "../comm/util.hpp"
#include "../comm/log.hpp"
#include "../comm/httplib.h"
#include "oj_model_mysql.hpp"
#include "oj_view.hpp"

namespace ns_control
{
    using namespace std;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model_mysql;
    using namespace ns_view;
    using namespace httplib;

    // 提供服务的主机
    class Machine
    {
    public:
        std::string ip;
        int port;
        std::atomic<uint64_t> load;          // 原子变量
        std::atomic<bool> is_offline{false}; // 新增离线状态标记

        // 添加移动构造函数
        Machine(Machine &&other) noexcept
            : ip(std::move(other.ip)),
              port(other.port),
              load(other.load.load()) {}

        // 添加移动赋值运算符
        Machine &operator=(Machine &&other) noexcept
        {
            ip = std::move(other.ip);
            port = other.port;
            load = other.load.load();
            return *this;
        }

        // 显式删除拷贝构造函数和拷贝赋值运算符
        Machine(const Machine &) = delete;
        Machine &operator=(const Machine &) = delete;

        Machine() : ip(""), port(0), load(0) {}
        void IncLoad() { ++load; }
        void DecLoad() { --load; }
        void ResetLoad() { load = 0; }
        uint64_t Load() { return load.load(); }
    };

    const std::string service_machine = "./conf/service_machine.conf";
    // 负载均衡模块
    class LoadBlance
    {
    private:
        // 可以给我们提供编译服务的所有的主机
        // 每一台主机都有自己的下标，充当当前主机的id
        std::vector<Machine> machines;
        // 所有在线的主机id
        std::vector<int> online;
        // 所有离线的主机id
        std::vector<int> offline;
        // 保证LoadBlance它的数据安全
        std::mutex mtx;
        // 负载阈值
        const uint64_t MAX_LOAD = 6;

    public:
        LoadBlance()
        {
            assert(LoadConf(service_machine));
            LOG(INFO) << "加载 " << service_machine << " 成功"
                      << "\n";
        }
        ~LoadBlance()
        {
        }

    public:
        bool LoadConf(const std::string &machine_conf)
        {
            std::ifstream in(machine_conf);
            if (!in.is_open())
            {
                LOG(FATAL) << "加载: " << machine_conf << " 失败\n";
                return false;
            }

            std::string line;
            while (std::getline(in, line))
            {
                // ...解析ip和port...
                std::vector<std::string> tokens;
                StringUtil::SplitString(line, &tokens, ":");
                if (tokens.size() != 2)
                {
                    LOG(WARNING) << " 切分 " << line << " 失败"
                                 << "\n";
                    continue;
                }
                std::string ip = tokens[0];
                int port = atoi(tokens[1].c_str());
                // 健康检查：尝试建立TCP连接
                HealthCheck(ip, port);
            }

            in.close();
            return true;
        }

        void AddToOffline(const std::string &ip, int port)
        {
            machines.emplace_back();
            auto &m = machines.back();
            m.ip = ip;
            m.port = port;
            offline.push_back(machines.size() - 1);
        }

        void HealthCheck(std::string ip, int port)
        {
            httplib::Client cli(ip, port);
            cli.set_connection_timeout(1); // 设置超时时间1秒

            if (auto res = cli.Get("/health"))
            { // 假设编译服务提供健康检查端点
                if (res->status == 200)
                {
                    machines.emplace_back();
                    auto &m = machines.back();
                    m.ip = ip;
                    m.port = port;
                    online.push_back(machines.size() - 1);
                    LOG(INFO) << "主机 " << ip << ":" << port << " 在线\n";
                }
                else
                {
                    LOG(WARNING) << "主机 " << ip << ":" << port << " 健康检查失败\n";
                    AddToOffline(ip, port);
                }
            }
            else
            {
                LOG(WARNING) << "主机 " << ip << ":" << port << " 无法连接\n";
                AddToOffline(ip, port);
            }
        }

        // id: 输出型参数
        // m : 输出型参数
        bool SmartChoice(int *id, Machine **m)
        {
            // 1. 使用选择好的主机(更新该主机的负载)
            // 2. 我们需要可能离线该主机
            std::lock_guard<std::mutex> lock(mtx);
            // 负载均衡的算法
            // 1. 随机数+hash
            // 2. 轮询+hash
            int online_num = online.size();
            if (online_num == 0)
            {
                mtx.unlock();
                LOG(FATAL) << " 所有的后端编译主机已经离线"
                           << "\n";
                return false;
            }
            // 通过遍历的方式，找到所有负载最小的机器
            *id = online[0];
            *m = &machines[online[0]];
            uint64_t min_load = machines[online[0]].Load();
            for (int i = 1; i < online_num; i++)
            {
                uint64_t curr_load = machines[online[i]].Load();
                if (min_load > curr_load)
                {
                    min_load = curr_load;
                    *id = online[i];
                    *m = &machines[online[i]];
                }
            }
            mtx.unlock();
            return true;
        }
        void OfflineMachine(int which)
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (auto iter = online.begin(); iter != online.end(); iter++)
            {
                if (*iter == which)
                {
                    machines[which].ResetLoad();
                    online.erase(iter);
                    offline.push_back(which);
                    break;
                }
            }
        }

        void OnlineMachine()
        {
            // 我们统一上线，后面统一解决
            std::lock_guard<std::mutex> lock(mtx);
            // online.insert(online.end(), offline.begin(), offline.end());
            machines.erase(machines.begin(), machines.end());
            online.erase(online.begin(), online.end());
            offline.erase(offline.begin(), offline.end());
            assert(LoadConf(service_machine));

            LOG(INFO) << "所有的主机都上线啦!" << "\n";
        }
        // for test
        void ShowMachines()
        {
            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "当前在线主机列表: ";
            for (auto &id : online)
            {
                std::cout << id << " ";
            }
            std::cout << std::endl;
            std::cout << "当前离线主机列表: ";
            for (auto &id : offline)
            {
                std::cout << id << " ";
            }
            std::cout << std::endl;
        }
    };
    // 这是我们的核心业务逻辑的控制器
    class Control
    {
    private:
        Model model_;            // 提供后台数据
        View view_;              // 提供html渲染功能
        LoadBlance load_blance_; // 核心负载均衡器

    public:
        Control()
        {
        }
        ~Control()
        {
        }

    public:
        void RecoveryMachine()
        {
            load_blance_.OnlineMachine();
        }
        // 根据题目数据构建网页
        //  html: 输出型参数
        bool AllQuestions(std::string *html)
        {
            bool ret = true;
            vector<struct Question> all;
            if (model_.GetAllQuestions(&all))
            {
                sort(all.begin(), all.end(), [](const struct Question &q1, const struct Question &q2)
                     { return atoi(q1.number.c_str()) < atoi(q2.number.c_str()); });
                // 获取题目信息成功，将所有的题目数据构建成网页
                view_.AllExpandHtml(all, html);
            }
            else
            {
                *html = "获取题目失败, 形成题目列表失败";
                ret = false;
            }
            return ret;
        }
        bool Question(const std::string &number, std::string *html)
        {
            bool ret = true;
            struct Question q;
            if (model_.GetOneQuestion(number, &q))
            {
                // 获取指定题目信息成功，将所有的题目数据构建成网页
                view_.OneExpandHtml(q, html);
            }
            else
            {
                *html = "指定题目: " + number + " 不存在!";
                ret = false;
            }
            return ret;
        }

        // code: #include...
        // input: ""
        void Judge(const std::string &number, const std::string in_json, std::string *out_json)
        {
            // LOG(DEBUG) << in_json << " \nnumber:" << number << "\n";

            // 0. 根据题目编号，直接拿到对应的题目细节
            struct Question q;
            model_.GetOneQuestion(number, &q);

            // 1. in_json进行反序列化，得到题目的id，得到用户提交源代码，input
            Json::Reader reader;
            Json::Value in_value;
            reader.parse(in_json, in_value);
            std::string code = in_value["code"].asString();

            // 2. 重新拼接用户代码+测试用例代码，形成新的代码
            Json::Value compile_value;
            compile_value["input"] = in_value["input"].asString();
            compile_value["code"] = code + "\n" + q.tail;
            compile_value["cpu_limit"] = q.cpu_limit;
            compile_value["mem_limit"] = q.mem_limit;
            Json::FastWriter writer;
            std::string compile_string = writer.write(compile_value);

            // 3. 选择负载最低的主机(差错处理)
            // 规则: 一直选择，直到主机可用，否则，就是全部挂掉
            while (true)
            {
                int id = 0;
                Machine *m = nullptr;
                if (!load_blance_.SmartChoice(&id, &m))
                {
                    break;
                }

                // 4. 然后发起http请求，得到结果
                httplib::Client cli(m->ip, m->port);
                m->IncLoad();
                LOG(INFO) << " 选择主机成功, 主机id: " << id << " 详情: " << m->ip << ":" << m->port << " 当前主机的负载是: " << m->Load() << "\n";
                if (auto res = cli.Post("/compiler_run", compile_string, "application/json;charset=utf-8"))
                {
                    // 5. 将结果赋值给out_json
                    if (res->status == 200)
                    {
                        *out_json = res->body;
                        m->DecLoad();
                        LOG(INFO) << "请求编译和运行服务成功..." << "\n";
                        load_blance_.ShowMachines(); // 仅仅是为了用来调试
                        break;
                    }
                    m->DecLoad();
                }
                else
                {
                    // 请求失败
                    LOG(ERROR) << " 当前请求的主机id: " << id << " 详情: " << m->ip << ":" << m->port << " 可能已经离线" << "\n";
                    load_blance_.OfflineMachine(id);
                    load_blance_.ShowMachines(); // 仅仅是为了用来调试
                }
            }
        }
    };

} // namespace name
