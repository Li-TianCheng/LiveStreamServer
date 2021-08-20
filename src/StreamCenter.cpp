//
// Created by ltc on 2021/6/28.
//

#include "StreamCenter.h"
#include "LiveStreamServer.h"

shared_ptr<SessionBase> StreamCenter::getStream(const string& vhost, const string& app, const string& streamName) {
    getStreamCenter().rwLock.rdLock();
    shared_ptr<SessionBase> stream = nullptr;
    if (getStreamCenter().streamCenter.find(vhost) != getStreamCenter().streamCenter.end()) {
        if (getStreamCenter().streamCenter[vhost].find(app) != getStreamCenter().streamCenter[vhost].end()) {
            if (getStreamCenter().streamCenter[vhost][app].find(streamName) != getStreamCenter().streamCenter[vhost][app].end()) {
                stream = getStreamCenter().streamCenter[vhost][app][streamName];
            }
        }
    }
    getStreamCenter().rwLock.unlock();
    if (stream == nullptr) {
        Json::Value relaySourceGroup = ConfigSystem::getConfig()["live_stream_server"]["relay_source"][vhost][app];
        for (auto& g : relaySourceGroup) {
            if (g["type"] == "http") {
                LiveStreamServer::httpFlvRelay(true, g["host"].asString(), vhost, app, streamName);
            } else {
                LiveStreamServer::rtmpRelay(true, g["host"].asString(), vhost, app, streamName);
            }
        }
    }
    return stream;
}

void StreamCenter::addStream(const string& vhost, const string& app, const string& streamName, shared_ptr<SessionBase> stream) {
    getStreamCenter().rwLock.wrLock();
    getStreamCenter().sourceNum++;
    getStreamCenter().streamCenter[vhost][app][streamName] = stream;
    getStreamCenter().rwLock.unlock();
    Json::Value relaySinkGroup = ConfigSystem::getConfig()["live_stream_server"]["relay_sink"][vhost][app];
    for (auto& g : relaySinkGroup) {
        if (g["type"] == "http") {
            LiveStreamServer::httpFlvRelay(false, g["host"].asString(), vhost, app, streamName);
        } else {
            LiveStreamServer::rtmpRelay(false, g["host"].asString(), vhost, app, streamName);
        }
    }
}

StreamCenter &StreamCenter::getStreamCenter() {
    static StreamCenter streamCenter;
    return streamCenter;
}

void StreamCenter::deleteStream(const string& vhost, const string& app, const string& streamName) {
    getStreamCenter().rwLock.wrLock();
    getStreamCenter().sourceNum--;
    getStreamCenter().streamCenter[vhost][app].erase(streamName);
    getStreamCenter().rwLock.unlock();
}

StreamCenter::StreamCenter() : sourceNum(0), sinkNum(0) {

}

void StreamCenter::updateSinkNum(int diff) {
    getStreamCenter().sinkNum += diff;
}

int StreamCenter::getSinkNum() {
    return getStreamCenter().sinkNum;
}

int StreamCenter::getSourceNum() {
    return getStreamCenter().sourceNum;
}

Json::Value StreamCenter::getStreamName() {
    Json::Value value;
    getStreamCenter().rwLock.rdLock();
    for (auto& vhost : getStreamCenter().streamCenter) {
        for (auto& app : vhost.second) {
            for (auto& stream : app.second) {
                value[vhost.first][app.first].append(stream.first);
            }
        }
    }
    getStreamCenter().rwLock.unlock();
    return value;
}
