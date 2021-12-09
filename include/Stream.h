//
// Created by ltc on 2021/6/28.
//

#ifndef LIVESTREAMSERVER_STREAM_H
#define LIVESTREAMSERVER_STREAM_H

#include <string>
#include <vector>
#include "mem_pool/include/ObjPool.hpp"

using std::string;
using std::vector;

struct Stream {
    Buffer currTag;
	Buffer script;
	Buffer avcTag;
	Buffer aacTag;
    vector<Buffer> gop;
    int idx = 0;
};


#endif //LIVESTREAMSERVER_STREAM_H
