//
// Created by ltc on 2021/7/25.
//

#include "SessionBase.h"

SessionBase::SessionBase(bool isRtmp, int bufferChunkSize): isRtmp(isRtmp), currNum(0), frameNum(0), chunkSize(ConfigSystem::getConfig()["live_stream_server"]["rtmp_chunk_size"].asInt()),
                                  count(0), lastCount(0), isAVC(false), isSource(false), isAAC(false), flushNum(ConfigSystem::getConfig()["live_stream_server"]["flush_num"].asInt()),
                                  isRelay(false), flushBufferSize(ConfigSystem::getConfig()["live_stream_server"]["flush_buffer_size"].asInt()), TcpSession(bufferChunkSize) {
    flvTemTag = ObjPool::allocate<vector<unsigned char>>();
    rtmpTemTag = ObjPool::allocate<vector<unsigned char>>();
}

void SessionBase::parseTag() {
    count++;
    currNum++;
    flvTemTag->insert(flvTemTag->end(), stream.currTag->begin(), stream.currTag->end());
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
            if (StreamCenter::getStream(isRelay, vhost, app, streamName) != nullptr) {
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
	auto session = sourceSession.lock();
    if (session != nullptr) {
        StreamCenter::updateSinkNum(1);
	    session->mutex.lock();
	    session->waitPlay.insert(static_pointer_cast<SessionBase>(shared_from_this()));
	    session->mutex.unlock();
    }
}

void SessionBase::sessionInit() {
    time = addTicker(0, 0, 1, 0);
}

void SessionBase::sessionClear() {
    if (isSource) {
        StreamCenter::deleteStream(vhost, app, streamName);
        std::ostringstream log;
        log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " source[" << shared_from_this() << "] end";
        LOG(Info, log.str());
    } else {
	    auto session = sourceSession.lock();
        if (session != nullptr) {
            StreamCenter::updateSinkNum(-1);
	        session->mutex.lock();
	        session->waitPlay.erase(static_pointer_cast<SessionBase>(shared_from_this()));
	        session->mutex.unlock();
	        session->rwLock.wrLock();
	        session->sinkManager.erase(static_pointer_cast<SessionBase>(shared_from_this()));
	        session->rwLock.unlock();
            std::ostringstream log;
            log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " sink[" << shared_from_this() << "] end";
            LOG(Info, log.str());
        }
    }
}

void SessionBase::handleTickerTimeOut(shared_ptr<Time> t) {
    if (time == t) {
        if (lastCount == count) {
            deleteSession();
        } else {
            lastCount = count;
        }
    }
}

void SessionBase::writeFlvHead() {
    auto head = ObjPool::allocate<vector<unsigned char>>();
	auto session = sourceSession.lock();
	if (session != nullptr) {
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
		write(session->stream.script);
		if (session->isAAC) {
			write(session->stream.aacTag);
		}
		if (session->isAVC) {
			write(session->stream.avcTag);
		}
		for (int i = 0; i < (session->stream.idx-session->frameNum); i++) {
			write(session->stream.gop[i]);
		}
	}
}

void SessionBase::flvToRtmp(shared_ptr<vector<unsigned char>> temTag, shared_ptr<vector<unsigned char>> tag) {
    int csId;
    if ((*tag)[0] == 8) {
        csId = 4;
    } else if ((*tag)[0] == 9 || (*tag)[0] == 18) {
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
	auto session = sourceSession.lock();
	if (session != nullptr) {
		flvToRtmp(chunk, session->stream.script);
		if (session->isAAC) {
			flvToRtmp(chunk, session->stream.aacTag);
		}
		if (session->isAVC) {
			flvToRtmp(chunk, session->stream.avcTag);
		}
		for (int i = 0; i < (session->stream.idx-session->frameNum); i++) {
			flvToRtmp(chunk, session->stream.gop[i]);
		}
		write(chunk);
	}
}

void SessionBase::handleReadDone(iter pos, size_t n) {
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
}
