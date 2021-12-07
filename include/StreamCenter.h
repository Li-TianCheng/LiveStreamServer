//
// Created by ltc on 2021/6/28.
//

#ifndef LIVESTREAMSERVER_STREAMCENTER_H
#define LIVESTREAMSERVER_STREAMCENTER_H

#include <unordered_map>
#include <string>
#include <memory>
#include <atomic>
#include <jsoncpp/json/json.h>
#include "my_pthread/include/RwLock.h"

using std::unordered_map;
using std::shared_ptr;
using std::string;

class SessionBase;

class StreamCenter {
public:
    static shared_ptr<SessionBase> getStream(bool isRelay, const string& vhost, const string& app, const string& streamName);
    static void addStream(const string& vhost, const string& app, const string& streamName, shared_ptr<SessionBase> stream);
    static void deleteStream(const string& vhost, const string& app, const string& streamName);
    static void updateSinkNum(int diff);
    static Json::Value getStreamName();
    static int getSinkNum();
    static int getSourceNum();
private:
    static StreamCenter& getStreamCenter();
    StreamCenter();
private:
    std::atomic<int> sourceNum;
    std::atomic<int> sinkNum;
    RwLock rwLock;
    unordered_map<string, unordered_map<string, unordered_map<string, shared_ptr<SessionBase>>>> streamCenter;
};


#endif //LIVESTREAMSERVER_STREAMCENTER_H
