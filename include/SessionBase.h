//
// Created by ltc on 2021/7/25.
//

#ifndef LIVESTREAMSERVER_SESSIONBASE_H
#define LIVESTREAMSERVER_SESSIONBASE_H

#include <sstream>
#include "my_pthread/include/RwLock.h"
#include "my_pthread/include/SpinLock.h"
#include "net/include/TcpSession.h"
#include "net/include/Ping.h"
#include "Stream.h"
#include "StreamCenter.h"

class SessionBase: public TcpSession {
public:
    explicit SessionBase(bool isRtmp, int bufferChunkSize);
    void writeFlvHead();
    void writeRtmpHead();
    void addSink();
    void handleReadDone(iter pos, size_t n) override;
    void sessionInit() override;
    void sessionClear() override;
    void handleTickerTimeOut(shared_ptr<Time> t) override;
    ~SessionBase() override = default;
protected:
    void parseTag();
    void flvToRtmp(Buffer& temTag, Buffer& tag);
protected:
    bool isRtmp;
    bool isSource;
    bool isAVC;
    bool isAAC;
	bool isRelay;
	bool isAddSink;
    int count;
    int lastCount;
    int currNum;
    int frameNum;
    int flushNum;
    int flushBufferSize;
    unsigned int chunkSize;
    string streamName;
    string app;
    string vhost;
    Stream stream;
	shared_ptr<Time> time;
    weak_ptr<SessionBase> sourceSession;
	Buffer flvTemTag;
	Buffer rtmpTemTag;
    unordered_set<shared_ptr<SessionBase>> waitPlay;
    unordered_set<shared_ptr<SessionBase>> sinkManager;
    RwLock rwLock;
    SpinLock lock;
};


#endif //LIVESTREAMSERVER_SESSIONBASE_H
