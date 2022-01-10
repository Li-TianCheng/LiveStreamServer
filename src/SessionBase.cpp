//
// Created by ltc on 2021/7/25.
//

#include "SessionBase.h"

SessionBase::SessionBase(bool isRtmp, int bufferChunkSize): isRtmp(isRtmp), currNum(0), frameNum(0), chunkSize(ConfigSystem::getConfig()["live_stream_server"]["rtmp_chunk_size"].asInt()),
                                  count(0), lastCount(0), isAVC(false), isSource(false), isAAC(false), flushNum(ConfigSystem::getConfig()["live_stream_server"]["flush_num"].asInt()),
                                  isRelay(false), isAddSink(false), flushBufferSize(ConfigSystem::getConfig()["live_stream_server"]["flush_buffer_size"].asInt()), TcpSession(bufferChunkSize),
								  flvTemTag(ConfigSystem::getConfig()["live_stream_server"]["flush_buffer_size"].asInt()),
								  rtmpTemTag(ConfigSystem::getConfig()["live_stream_server"]["flush_buffer_size"].asInt()) {

}

void SessionBase::parseTag() {
    count++;
    currNum++;
    flvTemTag.insert(stream.currTag, 0, stream.currTag.size());
    flvToRtmp(rtmpTemTag, stream.currTag);
    if (currNum == flushNum || (stream.currTag[0] == 9 && stream.currTag[11] >> 4 == 1)) {
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
        flvTemTag = Buffer(flushBufferSize);
        rtmpTemTag = Buffer(flushBufferSize);
    }
    switch(stream.currTag[0]) {
        case 8: {
            if (stream.currTag[11] >> 4 == 10 && stream.currTag[12] == 0) {
                isAAC = true;
	            stream.currTag.swap(stream.aacTag);
            }
            break;
        }
        case 9: {
            if (stream.currTag[11] == 23 && stream.currTag[12] == 0) {
                isAVC = true;
				stream.currTag.swap(stream.avcTag);
                break;
            }
            if (stream.currTag[11] >> 4 == 1) {
                stream.idx = 0;
            } else if (currNum != 0){
                frameNum++;
            }
            if (stream.idx == stream.gop.size()) {
                stream.gop.emplace_back();
            }
	        stream.currTag.swap(stream.gop[stream.idx]);
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
    stream.currTag.clear();
}

void SessionBase::addSink() {
	auto session = sourceSession.lock();
    if (session != nullptr) {
	    isAddSink = true;
        StreamCenter::updateSinkNum(1);
	    session->lock.lock();
	    session->waitPlay.insert(static_pointer_cast<SessionBase>(shared_from_this()));
	    session->lock.unlock();
    }
}

void SessionBase::sessionInit() {
    time = addTicker(0, 0, 10, 0);
}

void SessionBase::sessionClear() {
    if (isSource) {
        StreamCenter::deleteStream(vhost, app, streamName);
        std::ostringstream log;
        log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " source[" << shared_from_this() << "] idx";
        LOG(Info, log.str());
    } else {
		if (isAddSink) {
			StreamCenter::updateSinkNum(-1);
		}
	    auto session = sourceSession.lock();
        if (session != nullptr) {
	        session->lock.lock();
	        session->waitPlay.erase(static_pointer_cast<SessionBase>(shared_from_this()));
	        session->lock.unlock();
	        session->rwLock.wrLock();
	        session->sinkManager.erase(static_pointer_cast<SessionBase>(shared_from_this()));
	        session->rwLock.unlock();
            std::ostringstream log;
            log << "vhost:" << vhost << " app:" << app << " streamName:" << streamName << " sink[" << shared_from_this() << "] idx";
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
		const unsigned char tmp[] = {'F', 'L', 'V', 1, 5, 0, 0, 0, 9, 0, 0, 0, 0};
		head->insert(head->end(), tmp, tmp+sizeof(tmp));
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

void SessionBase::flvToRtmp(Buffer& temTag, Buffer& tag) {
    int csId;
    if (tag[0] == 8) {
        csId = 4;
    } else if (tag[0] == 9 || tag[0] == 18) {
        csId = 6;
    }
    int length = tag.size()-15;
    unsigned int num = length / chunkSize;
    int total = 0;
    for (int i = 0; i <= num; i++) {
        if (total == length) {
            break;
        }
        if (i == 0) {
	        const unsigned char tmp[] = {(unsigned char)csId, tag[4], tag[5], tag[6],
								   (unsigned char)((length >> 16) & 0x0000ff),
	                               (unsigned char)((length >> 8) & 0x0000ff),
	                               (unsigned char)(length & 0x0000ff),
	                               tag[0], 1, 0, 0, 0};
			temTag.insert(tmp, sizeof(tmp));
        } else {
            temTag.push_back(csId | 0xc0);
        }
        int inc = chunkSize;
        if (length - i*chunkSize <= inc) {
            inc = length - i*chunkSize;
        }
        total += inc;
        temTag.insert(tag, i * chunkSize + 11, i * chunkSize + inc + 11);
    }
}

void SessionBase::writeRtmpHead() {
    auto chunk = Buffer();
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
        lock.lock();
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
        lock.unlock();
    }
}
