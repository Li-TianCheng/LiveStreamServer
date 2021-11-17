//
// Created by ltc on 2021/7/25.
//

#ifndef LIVESTREAMSERVER_SESSIONBASE_H
#define LIVESTREAMSERVER_SESSIONBASE_H

#include <sstream>
#include "my_pthread/include/RwLock.h"
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
    void flvToRtmp(shared_ptr<vector<unsigned char>> temTag, shared_ptr<vector<unsigned char>> tag);
protected:
    bool isRtmp;
    bool isSource;
    bool isAVC;
    bool isAAC;
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
    shared_ptr<SessionBase> sourceSession;
    shared_ptr<vector<unsigned char>> flvTemTag;
    shared_ptr<vector<unsigned char>> rtmpTemTag;
    unordered_set<shared_ptr<SessionBase>> waitPlay;
    unordered_set<shared_ptr<SessionBase>> sinkManager;
    RwLock rwLock;
    Mutex mutex;
};


#endif //LIVESTREAMSERVER_SESSIONBASE_H
