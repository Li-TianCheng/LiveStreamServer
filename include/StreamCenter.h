//
// Created by ltc on 2021/6/28.
//

#ifndef LIVESTREAMSERVER_STREAMCENTER_H
#define LIVESTREAMSERVER_STREAMCENTER_H

#include <unordered_map>
#include <string>
#include <memory>
#include <atomic>
#include "my_pthread/include/RwLock.h"

using std::unordered_map;
using std::shared_ptr;
using std::string;

class FlvSessionBase;

class StreamCenter {
public:
    static shared_ptr<FlvSessionBase> getStream(const string& vhost, const string& app, const string& streamName);
    static void addStream(const string& vhost, const string& app, const string& streamName, shared_ptr<FlvSessionBase> stream);
    static void deleteStream(const string& vhost, const string& app, const string& streamName);
    static void updateSinkNum(int diff);
    static int getSinkNum();
    static int getSourceNum();
private:
    static StreamCenter& getStreamCenter();
    StreamCenter();
private:
    std::atomic<int> sourceNum;
    std::atomic<int> sinkNum;
    RwLock rwLock;
    unordered_map<string, unordered_map<string, unordered_map<string, shared_ptr<FlvSessionBase>>>> streamCenter;
};


#endif //LIVESTREAMSERVER_STREAMCENTER_H
