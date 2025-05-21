#include <iostream>
#include <signal.h>
#include "../comm/httplib.h"
#include "oj_control.hpp"

using namespace httplib;
using namespace ns_control;
static Control *ctrl_p;

void Recovery(int signo)
{
    ctrl_p->RecoveryMachine();
}

int main()
{
    signal(SIGQUIT, Recovery);
    // 用户请求的服务器路由功能
    Server svr;
    Control ctrl;
    Model userModel;
    ctrl_p = &ctrl;

    // 获取所有的题目列表
    svr.Get("/all_questions", [&ctrl](const Request &req, Response &resp)
            {
        std::string html;
        ctrl.AllQuestions(&html);
        resp.set_content(html, "text/html; charset=utf-8"); });

    // 用户要根据题目编号，获取题目内容
    // /question/100 -> 正则匹配
    // R"()" 原始字符串,保持字符串内容原貌，不用做相关的转义
    svr.Get(R"(/question/(\d+))", [&ctrl](const Request &req, Response &resp)
            {
        std::string number = req.matches[1];
        std::string html;
        ctrl.Question(number, &html);
        resp.set_content(html, "text/html;charset=utf-8"); });

    // 用户提交代码，使用我们的判题功能(1.测试用例 2.compiler_run)
    svr.Post(R"(/judge/(\d+))", [&ctrl](const Request &req, Response &resp)
             {
        std::string number = req.matches[1];
        std::string result_json;
        ctrl.Judge(number, req.body, &result_json);
        resp.set_content(result_json, "application/json;charset = utf-8"); });

    svr.set_base_dir("./wwwroot");
    svr.listen("0.0.0.0", 8080);
    return 0;
}