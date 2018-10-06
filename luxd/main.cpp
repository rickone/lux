#include "lux_core.h"
#include "lux_proto.h"
#include <cstdio>
#include <iostream>
#include <climits>

void test(bool a, unsigned char b, unsigned short c, unsigned int d, unsigned long e, unsigned long long f, float g, double h, const std::string &str, const char *c_str)
{
    std::cout << a << std::endl
        << (unsigned int)b << std::endl
        << c << std::endl
        << d << std::endl
        << e << std::endl
        << f << std::endl
        << g << std::endl
        << h << std::endl
        << str << std::endl
        << c_str << std::endl;
}

void test2(const std::vector<int> &arr)
{
    std::cout << "vector:" << std::endl;
    for (int n : arr)
        std::cout << n << std::endl;
}

void test3(const std::map<std::string, int> &dict)
{
    std::cout << "dict:" << std::endl;
    for (auto &pair : dict)
        std::cout << pair.first << ": " << pair.second << std::endl;
}

int main(int argc, char* argv[])
{
    //return LuxCore::main(argc, argv);
    LuxProto pt;
    pt.pack(true);
    pt.pack((unsigned char)UCHAR_MAX);
    pt.pack((unsigned short)USHRT_MAX);
    pt.pack(UINT_MAX);
    pt.pack(ULONG_MAX);
    pt.pack(ULLONG_MAX);
    pt.pack(3.14f);
    pt.pack(2.71);
    pt.pack("Hello");
    pt.pack("LuxProto");

    std::cout << pt.dump() << std::endl;
    pt.invoke(test);

    pt.clear();
    std::vector<int> arr = {1,2,3,4,5,6,100};
    pt.pack(arr);

    std::cout << pt.dump() << std::endl;
    pt.invoke(test2);

    pt.clear();
    pt.pack("This is a test for calling puts()");

    std::cout << pt.dump() << std::endl;
    int ret = pt.call(puts);
    std::cout << "puts ret = " << ret << std::endl;

    pt.clear();
    std::map<std::string, int> dict = {{"Rick", 33}, {"Tina", 28}};
    pt.pack(dict);

    std::cout << pt.dump() << std::endl;
    pt.invoke(test3);

    pt.clear();
    pt.pack(nullptr);

    std::cout << pt.dump() << std::endl;

    return 0;
}
