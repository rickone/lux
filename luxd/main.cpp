#include "lux_core.h"
#include "resp_object.h"
#include <iostream>

int main(int argc, char* argv[])
{
    RespObject obj(RespType::RESP_ARRAY);
    obj.set_array("SET", "age", 33);
    std::cout << obj << std::endl;

    std::string str;
    obj.serialize(str);
    std::cout << str << std::endl;

    RespObject obj2;
    std::string str2 = "*2\r\n$5\r\nhello\r\n$5\r\nworld\r\n";
    //std::string::size_type pos = 0;
    size_t pos = 0;
    for (;;)
    {
        //std::string::size_type tail = str2.find("\r\n", pos, 2);
        //if (tail == std::string::npos)
            //break;

        size_t used_len = 0;
        bool ret = obj2.deserialize(str2, pos, &used_len);
        pos += used_len;

        if (ret)
            break;
    }

    std::cout << obj2 << std::endl;

    //return lux::Core::main(argc, argv);
    return 0;
}
