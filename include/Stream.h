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
    shared_ptr<vector<unsigned char>> head;
    shared_ptr<vector<unsigned char>> script;
    shared_ptr<vector<unsigned char>> avcTag;
    shared_ptr<vector<unsigned char>> aacTag;
    shared_ptr<vector<unsigned char>> currTag;
    vector<shared_ptr<vector<unsigned char>>> gop;
    int idx;
    Stream(): idx(0) {
        head = ObjPool::allocate<vector<unsigned char>>();
        script = ObjPool::allocate<vector<unsigned char>>();
        avcTag = ObjPool::allocate<vector<unsigned char>>();
        aacTag = ObjPool::allocate<vector<unsigned char>>();
        currTag = ObjPool::allocate<vector<unsigned char>>();
    }
};


#endif //LIVESTREAMSERVER_STREAM_H
