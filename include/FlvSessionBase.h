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
    explicit FlvSessionBase(int bufferChunkSize);
    virtual void writeFlv(shared_ptr<vector<unsigned char>> tag);
    virtual void writeFlvHead();
    void addSink();
    void sessionInit() override;
    void sessionClear() override;
    void handleTickerTimeOut(const string &uuid) override;
    ~FlvSessionBase() override = default;
protected:
    friend class HttpFlvSession;
    friend class RtmpFlvSession;
    void parseFlv(const char& c);
    void parseTag();
protected:
    bool isSource;
    bool isVideo;
    bool isAVC;
    bool isAAC;
    int flvStatus;
    int count;
    int lastCount;
    int currNum;
    int frameNum;
    int flushNum;
    int flushBufferSize;
    unsigned int flvSize;
    string streamName;
    string app;
    string vhost;
    string uuid;
    Stream stream;
    shared_ptr<Ping> ping;
    shared_ptr<FlvSessionBase> sourceSession;
    shared_ptr<vector<unsigned char>> temTag;
    unordered_set<shared_ptr<FlvSessionBase>> waitPlay;
    unordered_set<shared_ptr<FlvSessionBase>> sinkManager;
    RwLock rwLock;
    Mutex mutex;
};


#endif //LIVESTREAMSERVER_FLVSESSIONBASE_H
