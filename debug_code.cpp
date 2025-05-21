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
