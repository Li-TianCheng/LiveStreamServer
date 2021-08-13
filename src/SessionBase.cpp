//
// Created by ltc on 2021/7/25.
//

#include "SessionBase.h"

SessionBase::SessionBase(bool isRtmp, int bufferChunkSize): isRtmp(isRtmp), sourceSession(nullptr), currNum(0), frameNum(0), flvStatus(0), flvSize(0), chunkSize(ConfigSystem::getConfig()["live_stream_server"]["rtmp_chunk_size"].asInt()),
                                  count(0), lastCount(0), isAVC(false), isSource(false), isAAC(false), flushNum(ConfigSystem::getConfig()["live_stream_server"]["flush_num"].asInt()),
                                  flushBufferSize(ConfigSystem::getConfig()["live_stream_server"]["flush_buffer_size"].asInt()), TcpSession(bufferChunkSize) {
    flvTemTag = ObjPool::allocate<vector<unsigned char>>();
    rtmpTemTag = ObjPool::allocate<vector<unsigned char>>();
}

void SessionBase::parseFlv(const char &c) {
    if (!waitPlay.empty()) {
        mutex.lock();
        for (auto& session : waitPlay) {
            if (session->isRtmp) {
                session->writeRtmpHead();
            } else {
                session->writeFlvHead();
            }
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
            stream.currTag->push_back(c);
            flvTemTag->push_back(c);
            if (stream.currTag->size() == 11) {
                flvStatus = 1;
                flvSize = (unsigned int)(*stream.currTag)[1] << 16 | (unsigned int)(*stream.currTag)[2] << 8 | (unsigned int)(*stream.currTag)[3];
                flvSize += 4;
            }
            break;
        }
        case 1: {
            stream.currTag->push_back(c);
            flvTemTag->push_back(c);
            flvSize--;
            if (flvSize == 0) {
                parseTag();
                flvStatus = 0;
            }
            break;
        }
        default: break;
    }
}

void SessionBase::parseTag() {
    count++;
    currNum++;
    flvToRtmp(rtmpTemTag, stream.currTag);
    if (currNum == flushNum || ((*stream.currTag)[0] == 9 && (*stream.currTag)[11] >> 4 == 1)) {
        rwLock.rdLock();
        for (auto& session : sinkManager) {
            if (session->isRtmp) {
                session->write(rtmpTemTag);
            } else {
                session->write(flvTemTag);
            }
            session->count++;
        }
        rwLock.unlock();
        currNum = 0;
        frameNum = 0;
        flvTemTag = ObjPool::allocate<vector<unsigned char>>();
        flvTemTag->reserve(flushBufferSize);
        rtmpTemTag = ObjPool::allocate<vector<unsigned char>>();
        rtmpTemTag->reserve(flushBufferSize);
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
            StreamCenter::addStream(vhost, app, streamName, static_pointer_cast<SessionBase>(shared_from_this()));
            std::ostringstream log;
            log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " source[" << shared_from_this() << "] publish";
            LOG(Access, log.str());
            break;
        }
        default: break;
    }
    stream.currTag->clear();
}

void SessionBase::addSink() {
    if (sourceSession != nullptr) {
        StreamCenter::updateSinkNum(1);
        sourceSession->mutex.lock();
        sourceSession->waitPlay.insert(static_pointer_cast<SessionBase>(shared_from_this()));
        sourceSession->mutex.unlock();
    }
}

void SessionBase::sessionInit() {
    uuid = addTicker(0, 0, 1, 0);
}

void SessionBase::sessionClear() {
    if (isSource) {
        StreamCenter::deleteStream(vhost, app, streamName);
        mutex.lock();
        waitPlay.clear();
        mutex.unlock();
        rwLock.wrLock();
        for (auto& session : sinkManager) {
            if (session->isRtmp) {
                session->write(rtmpTemTag);
            } else {
                session->write(flvTemTag);
            }
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
            sourceSession->waitPlay.erase(static_pointer_cast<SessionBase>(shared_from_this()));
            mutex.unlock();
            sourceSession->rwLock.wrLock();
            sourceSession->sinkManager.erase(static_pointer_cast<SessionBase>(shared_from_this()));
            sourceSession->rwLock.unlock();
            std::ostringstream log;
            log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " sink[" << shared_from_this() << "] end";
            LOG(Info, log.str());
        }
    }
    sourceSession = nullptr;
}

void SessionBase::handleTickerTimeOut(const string &uuid) {
    if (this->uuid == uuid) {
        if (lastCount == count) {
            deleteSession();
        } else {
            lastCount = count;
        }
    }
}

void SessionBase::writeFlvHead() {
    auto head = ObjPool::allocate<vector<unsigned char>>();
    head->push_back('F');
    head->push_back('L');
    head->push_back('V');
    head->push_back(1);
    head->push_back(5);
    head->push_back(0);
    head->push_back(0);
    head->push_back(0);
    head->push_back(9);
    head->push_back(0);
    head->push_back(0);
    head->push_back(0);
    head->push_back(0);
    write(head);
    write(sourceSession->stream.script);
    if (sourceSession->isAAC) {
        write(sourceSession->stream.aacTag);
    }
    if (sourceSession->isAVC) {
        write(sourceSession->stream.avcTag);
    }
    for (int i = 0; i < (sourceSession->stream.idx-sourceSession->frameNum); i++) {
        write(sourceSession->stream.gop[i]);
    }
}

void SessionBase::flvToRtmp(shared_ptr<vector<unsigned char>> temTag, shared_ptr<vector<unsigned char>> tag) {
    int csId;
    if ((*tag)[0] == 8) {
        csId = 4;
    } else if ((*tag)[0] == 9) {
        csId = 6;
    }
    int length = tag->size()-15;
    unsigned int num = length / chunkSize;
    int total = 0;
    for (int i = 0; i <= num; i++) {
        if (total == length) {
            break;
        }
        if (i == 0) {
            temTag->push_back(csId);
            temTag->push_back((*tag)[4]);
            temTag->push_back((*tag)[5]);
            temTag->push_back((*tag)[6]);
            temTag->push_back((length >> 16) & 0x0000ff);
            temTag->push_back((length >> 8) & 0x0000ff);
            temTag->push_back(length & 0x0000ff);
            temTag->push_back((*tag)[0]);
            temTag->push_back(1);
            temTag->push_back(0);
            temTag->push_back(0);
            temTag->push_back(0);
        } else {
            temTag->push_back(csId | 0xc0);
        }
        int inc = chunkSize;
        if (length - i*chunkSize <= inc) {
            inc = length - i*chunkSize;
        }
        total += inc;
        temTag->insert(temTag->end(), tag->begin()+i*chunkSize+11, tag->begin()+i*chunkSize+inc+11);
    }
}

void SessionBase::writeRtmpHead() {
    auto chunk = ObjPool::allocate<vector<unsigned char>>();
    int length = sourceSession->stream.script->size()+1;
    unsigned int num = length / chunkSize;
    int total = 0;
    for (int i = 0; i <= num; i++) {
        if (total == length) {
            break;
        }
        if (i == 0) {
            chunk->push_back(6);
            chunk->push_back(0);
            chunk->push_back(0);
            chunk->push_back(0);
            chunk->push_back((length >> 16) & 0x0000ff);
            chunk->push_back((length >> 8) & 0x0000ff);
            chunk->push_back(length & 0x0000ff);
            chunk->push_back(18);
            chunk->push_back(1);
            chunk->push_back(0);
            chunk->push_back(0);
            chunk->push_back(0);
            chunk->push_back(2);
            chunk->push_back(0);
            chunk->push_back(13);
            chunk->push_back('@');
            chunk->push_back('s');
            chunk->push_back('e');
            chunk->push_back('t');
            chunk->push_back('D');
            chunk->push_back('a');
            chunk->push_back('t');
            chunk->push_back('a');
            chunk->push_back('F');
            chunk->push_back('r');
            chunk->push_back('a');
            chunk->push_back('m');
            chunk->push_back('e');
            total += 16;
        } else {
            chunk->push_back(6 | 0xc0);
        }
        int inc = chunkSize;
        if (length-16-i*chunkSize <= inc) {
            inc = length-16- i*chunkSize;
        }
        total += inc;
        chunk->insert(chunk->end(), sourceSession->stream.script->begin()+i*chunkSize+11, sourceSession->stream.script->begin()+i*chunkSize+inc+11);
    }
    if (sourceSession->isAAC) {
        flvToRtmp(chunk, sourceSession->stream.aacTag);
    }
    if (sourceSession->isAVC) {
        flvToRtmp(chunk, sourceSession->stream.avcTag);
    }
    for (int i = 0; i < (sourceSession->stream.idx-sourceSession->frameNum); i++) {
        flvToRtmp(chunk, sourceSession->stream.gop[i]);
    }
    write(chunk);
}