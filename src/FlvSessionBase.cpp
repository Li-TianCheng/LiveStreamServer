//
// Created by ltc on 2021/7/25.
//

#include "FlvSessionBase.h"

FlvSessionBase::FlvSessionBase(int bufferChunkSize): isVideo(false), flvStatus(0), flvSize(0), sourceSession(nullptr), currNum(0), frameNum(0),
                                  ping(nullptr), count(0), lastCount(0), isAVC(false), isSource(false), isAAC(false), flushNum(ConfigSystem::getConfig()["live_stream_write"]["flush_num"].asInt()),
                                  flushBufferSize(ConfigSystem::getConfig()["live_stream_write"]["flush_buffer_size"].asInt()), TcpSession(bufferChunkSize) {
    temTag = ObjPool::allocate<vector<unsigned char>>();
}

void FlvSessionBase::parseFlv(const char &c) {
    if (!waitPlay.empty()) {
        mutex.lock();
        for (auto& session : waitPlay) {
            session->writeFlvHead();
            std::ostringstream log;
            log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " sink[" << session << "] play";
            LOG(Access, log.str());
            rwLock.wrLock();
            sinkManager.insert(session);
            rwLock.unlock();
        }
        waitPlay.clear();
        mutex.unlock();
    }
    switch(flvStatus) {
        case 0: {
            stream.head->push_back(c);
            if (stream.head->size() == 13) {
                isVideo = (*stream.head)[4] == 0x05;
                flvStatus = 1;
            }
            break;
        }
        case 1: {
            stream.currTag->push_back(c);
            temTag->push_back(c);
            if (stream.currTag->size() == 11) {
                flvStatus = 2;
                flvSize = (unsigned int)(*stream.currTag)[1] << 16 | (unsigned int)(*stream.currTag)[2] << 8 | (unsigned int)(*stream.currTag)[3];
                flvSize += 4;
            }
            break;
        }
        case 2: {
            stream.currTag->push_back(c);
            temTag->push_back(c);
            flvSize--;
            if (flvSize == 0) {
                parseTag();
                flvStatus = 1;
            }
            break;
        }
        default: break;
    }
}

void FlvSessionBase::parseTag() {
    count++;
    currNum++;
    if (currNum == flushNum || ((*stream.currTag)[0] == 9 && (*stream.currTag)[11] >> 4 == 1)) {
        rwLock.rdLock();
        for (auto& session : sinkManager) {
            session->writeFlv(temTag);
            session->count++;
        }
        rwLock.unlock();
        currNum = 0;
        frameNum = 0;
        temTag = ObjPool::allocate<vector<unsigned char>>();
        temTag->reserve(flushBufferSize);
    }
    switch((*stream.currTag)[0]) {
        case 8: {
            if ((*stream.currTag)[11] >> 4 == 10 && (*stream.currTag)[12] == 0) {
                isAAC = true;
                auto tem = stream.currTag;
                stream.currTag = stream.aacTag;
                stream.aacTag = tem;
            }
            break;
        }
        case 9: {
            if ((*stream.currTag)[11] == 23 && (*stream.currTag)[12] == 0) {
                isAVC = true;
                auto tem = stream.currTag;
                stream.currTag = stream.avcTag;
                stream.avcTag = tem;
                break;
            }
            if ((*stream.currTag)[11] >> 4 == 1) {
                stream.idx = 0;
            } else if (currNum != 0){
                frameNum++;
            }
            if (stream.idx == stream.gop.size()) {
                stream.gop.push_back(ObjPool::allocate<vector<unsigned char>>());
            }
            auto tem = stream.currTag;
            stream.currTag = stream.gop[stream.idx];
            stream.gop[stream.idx] = tem;
            stream.idx++;
            break;
        }
        case 18: {
            auto tem = stream.currTag;
            stream.currTag = stream.script;
            stream.script = tem;
            if (StreamCenter::getStream(vhost, app, streamName) != nullptr) {
                deleteSession();
                break;
            }
            isSource = true;
            StreamCenter::addStream(vhost, app, streamName, static_pointer_cast<FlvSessionBase>(shared_from_this()));
            std::ostringstream log;
            log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " source[" << shared_from_this() << "] publish";
            LOG(Access, log.str());
            break;
        }
        default: break;
    }
    stream.currTag->clear();
}

void FlvSessionBase::writeFlv(shared_ptr<vector<unsigned char>> tag) {

}

void FlvSessionBase::addSink() {
    if (sourceSession != nullptr) {
        StreamCenter::updateSinkNum(1);
        sourceSession->mutex.lock();
        sourceSession->waitPlay.insert(static_pointer_cast<FlvSessionBase>(shared_from_this()));
        sourceSession->mutex.unlock();
    }
}

void FlvSessionBase::sessionInit() {
    ping = ObjPool::allocate<Ping>(*(sockaddr_in*)&address);
    ping->send();
    uuid = addTicker(0, 0, 1, 0);
}

void FlvSessionBase::sessionClear() {
    if (isSource) {
        StreamCenter::deleteStream(vhost, app, streamName);
        mutex.lock();
        waitPlay.clear();
        mutex.unlock();
        rwLock.wrLock();
        for (auto& session : sinkManager) {
            session->writeFlv(temTag);
        }
        sinkManager.clear();
        rwLock.unlock();
        std::ostringstream log;
        log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " source[" << shared_from_this() << "] end";
        LOG(Info, log.str());
    } else {
        if (sourceSession != nullptr) {
            StreamCenter::updateSinkNum(-1);
            mutex.lock();
            sourceSession->waitPlay.erase(static_pointer_cast<FlvSessionBase>(shared_from_this()));
            mutex.unlock();
            sourceSession->rwLock.wrLock();
            sourceSession->sinkManager.erase(static_pointer_cast<FlvSessionBase>(shared_from_this()));
            sourceSession->rwLock.unlock();
            std::ostringstream log;
            log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " sink[" << shared_from_this() << "] end";
            LOG(Info, log.str());
        }
    }
    sourceSession = nullptr;
}

void FlvSessionBase::handleTickerTimeOut(const string &uuid) {
    if (this->uuid == uuid) {
        if (!ping->recv() || lastCount == count) {
            deleteSession();
        } else {
            lastCount = count;
            ping->send();
        }
    }
}

void FlvSessionBase::writeFlvHead() {

}