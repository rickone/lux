#include "lux_core.h"
#include "lux_proto.h"
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
    return 0;
}
