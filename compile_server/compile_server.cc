#include "compile_run.hpp"
#include <stdexcept>
#include "../comm/httplib.h"
using namespace ns_compile_and_run;
using namespace httplib;

void Usage(std::string proc)
{
    std::cerr << "Usage: " << "\n\t" << proc << " port" << std::endl;
}

// 编译服务可能会有多人请求，必须保证传递上来的code，形成源文件名称的时候
// 具有唯一性，不然多用户之间会相互影响
int main(int argc, char *argv[])
{
    try
    {
        if (argc != 2)
        {
            Usage(argv[0]);
            return 1;
        }
        Server svr;
        // svr.Get("/hello", [](const Request &req, Response &resp)
        //         { resp.set_content("hello", "text/plain;charset=utf-8"); });

        svr.Post("/compiler_run", [](const Request &req, Response &resp)
                 {
        //用户请求的服务正文，就是我们需要的json string
        std::string in_json = req.body;
        std::string out_json;
        if (!in_json.empty())
        {
            CompileAndRun::Start(in_json, &out_json);
            resp.set_content(out_json, "application/json;setchar=utf-8");
        }
        else
        {
            resp.set_content("in_json为空", "application/json;setchar=utf-8");
        }
        });
        
        svr.listen("0.0.0.0", atoi(argv[1]));
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << '\n';
    }

    // std::string in_json;
    // Json::Value in_value;
    // in_value["code"] = R"(#include <iostream>
    // int main()
    // {
    //     while(1);
    //     std::cout << "OK" << std::endl;
    //     return 0;
    // })";
    // in_value["input"] = "";
    // in_value["cpu_limit"] = 1;
    // in_value["mem_limit"] = 10240 * 3;
    // Json::FastWriter writer;
    // in_json = writer.write(in_value);

    // std::string out_json;
    // CompileAndRun::Start(in_json, &out_json);
    // std::cout << out_json << std::endl;

    return 0;
}