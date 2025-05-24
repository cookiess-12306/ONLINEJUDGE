#include "compile_run.hpp"
#include <stdexcept>
#include "ThreadPool.hpp"
#include "../comm/httplib.h"
using namespace ns_compile_and_run;
using namespace httplib;
using namespace ns_thread_pool;

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
        ns_thread_pool::ThreadPool pool(std::thread::hardware_concurrency());
        svr.Post("/compiler_run", [&pool](const Request &req, Response &resp)
                 {
                    std::time_t now = std::time(nullptr);
                    std::tm *timeinfo = std::localtime(&now);
                    std::cerr << "[Log] Received request:\n" << req.body << "time:" << std::put_time(timeinfo, "%H:%M:%S") << std::endl;
                    
        std::string in_json = req.body;
        std::string out_json;

        std::mutex mtx;
        std::condition_variable cv;
        bool done = false;

        pool.enqueue([&]() {
            CompileAndRun::Start(in_json, &out_json);
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
            cv.notify_one();
        });

        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&done]() { return done; });
        resp.set_content(out_json, "application/json;charset=utf-8"); });

        svr.Get("/health", [](const Request &req, Response &res)
                {
                    res.status = 200; // 只需返回 200 状态码
                });

        svr.listen("0.0.0.0", atoi(argv[1]));
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << '\n';
    }
    return 0;
}