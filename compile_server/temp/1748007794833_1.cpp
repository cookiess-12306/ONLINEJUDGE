#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

class Solution
{
public:
    bool isPalindrome(long long x)
    {
        if (x < 0) return false; long long y = 0, t = x; while (t) { y = y * 10 + t % 10; t /= 10; } return x == y;
    }
};
#ifndef COMPILER_ONLINE
#include "header.cpp"
#endif

void Test1()
{
    // 通过定义临时对象，来完成方法的调用
    bool ret = Solution().isPalindrome(121);
    if (ret)
    {
        std::cout << "通过用例1, 测试121通过 ... OK!" << std::endl;
    }
    else
    {
        std::cout << "没有通过用例1, 测试的值是: 121" << std::endl;
    }
}

void Test2()
{
    // 通过定义临时对象，来完成方法的调用
    bool ret = Solution().isPalindrome(-10);
    if (!ret)
    {
        std::cout << "通过用例2, 测试-10通过 ... OK!" << std::endl;
    }
    else
    {
        std::cout << "没有通过用例2, 测试的值是: -10" << std::endl;
    }
}

void Test3()
{
    // 通过定义临时对象，来完成方法的调用
    bool ret = Solution().isPalindrome(123454321);
    if (ret)
    {
        std::cout << "通过用例2, 测试123454321通过 ... OK!" << std::endl;
    }
    else
    {
        std::cout << "没有通过用例2, 测试的值是: 123454321" << std::endl;
    }
}
void Test4()
{
    // 通过定义临时对象，来完成方法的调用
    bool ret = Solution().isPalindrome(1234567890987654321);
    if (ret)
    {
        std::cout << "通过用例2, 测试1234567890987654321通过 ... OK!" << std::endl;
    }
    else
    {
        std::cout << "没有通过用例2, 测试的值是: 1234567890987654321" << std::endl;
    }
}

int main()
{
    Test1();
    Test2();
    Test3();
    Test4();

    return 0;
}