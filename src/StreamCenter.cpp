//
// Created by ltc on 2021/6/28.
//

#include "StreamCenter.h"

shared_ptr<FlvSessionBase> StreamCenter::getStream(const string& vhost, const string& app, const string& streamName) {
    getStreamCenter().rwLock.rdLock();
    shared_ptr<FlvSessionBase> stream = nullptr;
    if (getStreamCenter().streamCenter.find(vhost) != getStreamCenter().streamCenter.end()) {
        if (getStreamCenter().streamCenter[vhost].find(app) != getStreamCenter().streamCenter[vhost].end()) {
            if (getStreamCenter().streamCenter[vhost][app].find(streamName) != getStreamCenter().streamCenter[vhost][app].end()) {
                stream = getStreamCenter().streamCenter[vhost][app][streamName];
            }
        }
    }
    getStreamCenter().rwLock.unlock();
    return stream;
}

void StreamCenter::addStream(const string& vhost, const string& app, const string& streamName, shared_ptr<FlvSessionBase> stream) {
    getStreamCenter().rwLock.wrLock();
    getStreamCenter().sourceNum++;
    getStreamCenter().streamCenter[vhost][app][streamName] = stream;
    getStreamCenter().rwLock.unlock();
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
