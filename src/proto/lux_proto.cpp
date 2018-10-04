#include "lux_proto.h"
#include "error.h"

/* Header Byte Code
*
* Varient Int:
* 0xxx,xxxx -- 1B
* 10xx,xxxx -> [1xxx,xxxx] -> [0xxx,xxxx] -- nB littile-endian
* 1100,0000 -- 1B type
*
* 0xC0 - nil/nullptr +[0]
* 0xC1 - false +[0]
* 0xC2 - true +[0]
* 0xC3 - number/float +[4]
* 0xC4 - number/double +[8]
* 0xC5 - string/const char * +(len:varint) +[len]
* 0xC6 - dict/map +(n:varint) +(key,value) x n
* 0xC7 - list/vector +(n:varint) +(value) x n
* 0xC8 - object +(len:varint) +[len]
*/

template<>
void LuxProto::pack(bool value)
{
    _str.append(value ? "\xC2" : "\xC1", 1);
}

template<>
bool LuxProto::unpack()
{
    uint8_t header = (uint8_t)_str.at(_pos);
    if (header == 0xC1)
    {
        _pos++;
        return false;
    }

    if (header == 0xC2)
    {
        _pos++;
        return true;
    }

    throw_error(std::runtime_error, "header=0x%02X", header);
}
