if (x < 0)
    return false;
int y = 0, t = x;
while (t)
{
    y = y * 10 + t % 10;
    t /= 10;
}
return x == y;