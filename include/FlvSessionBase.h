//
// Created by ltc on 2021/7/25.
//

#ifndef LIVESTREAMSERVER_FLVSESSIONBASE_H
#define LIVESTREAMSERVER_FLVSESSIONBASE_H

#include <sstream>
#include "my_pthread/include/RwLock.h"
#include "net/include/TcpSession.h"
#include "net/include/Ping.h"
#include "Stream.h"
#include "StreamCenter.h"

class FlvSessionBase: public TcpSession {
public:
    explicit FlvSessionBase(bool isRtmp, int bufferChunkSize);
    void writeFlvHead();
    void writeRtmpHead();
    void addSink();
    void sessionInit() override;
    void sessionClear() override;
    void handleTickerTimeOut(const string &uuid) override;
    ~FlvSessionBase() override = default;
protected:
    void parseFlv(const char& c);
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
    int flvStatus;
    unsigned int flvSize;
    unsigned int chunkSize;
    string streamName;
    string app;
    string vhost;
    string uuid;
    Stream stream;
    shared_ptr<FlvSessionBase> sourceSession;
    shared_ptr<vector<unsigned char>> flvTemTag;
    shared_ptr<vector<unsigned char>> rtmpTemTag;
    unordered_set<shared_ptr<FlvSessionBase>> waitPlay;
    unordered_set<shared_ptr<FlvSessionBase>> sinkManager;
    RwLock rwLock;
    Mutex mutex;
};


#endif //LIVESTREAMSERVER_FLVSESSIONBASE_H
