#ifndef COMPILER_ONLINE
#include "header.cpp"
#endif

int main()
{
    Solution s;
    cout << s.isPalindrome(121) << endl;
    cout << s.isPalindrome(-10) << endl;
    cout << s.isPalindrome(123454321) << endl;
    return 0;
}