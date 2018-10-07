#include "lux_core.h"
#include "lux_proto.h"
#include "buffer.h"
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

struct TestProto : LuxpObject
{
    std::string name;
    int age;
    bool gender;

    virtual void pack(LuxProto *proto) override
    {
        proto->pack(name);
        proto->pack(age);
        proto->pack(gender);
    }

    virtual void unpack(LuxProto *proto) override
    {
        name = proto->unpack<std::string>();
        age = proto->unpack<int>();
        gender = proto->unpack<bool>();
    }
};

void test4(const TestProto *info, int score)
{
    std::cout << "TestProto:" << std::endl
        << info->name << std::endl
        << info->age << std::endl
        << info->gender << std::endl
        << score << std::endl;
}

#pragma pack(1)
struct TestProto2
{
    char name[32];
    uint8_t age;
    uint8_t gender;
};
#pragma pack()

void test5(const TestProto2 *info, int score)
{
    std::cout << "TestProto2:" << std::endl
        << info->name << std::endl
        << (int)info->age << std::endl
        << (int)info->gender << std::endl
        << score << std::endl;
}

int main(int argc, char* argv[])
{
    //return LuxCore::main(argc, argv);
    LuxProto pt;
    pt.pack(true);
    pt.pack<unsigned char>(UCHAR_MAX);
    pt.pack<unsigned short>(USHRT_MAX);
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
    TestProto tp;
    tp.name = "Rick Yan";
    tp.age = 33;
    tp.gender = true;
    pt.pack(&tp);
    pt.pack(100);

    std::cout << pt.dump() << std::endl;
    pt.invoke(test4);

    pt.clear();
    TestProto2 tp2;
    sprintf(tp2.name, "%s", "Tina");
    tp2.age = 28;
    tp2.gender = false;
    pt.pack(&tp2);
    pt.pack(200);

    std::cout << pt.dump() << std::endl;
    pt.invoke(test5);

    return 0;
}
