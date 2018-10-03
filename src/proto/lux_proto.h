#pragma once

#include "buffer.h"

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

class LuxProto : public Buffer
{
public:
    LuxProto() = default;

    template<typename T>
    void save(T t);

    template<typename T>
    T load();

private:
};

template<>
void LuxProto::save(int t)
{

}