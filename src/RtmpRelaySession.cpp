//
// Created by ltc on 2021/8/12.
//

#include "RtmpRelaySession.h"

RtmpRelaySession::RtmpRelaySession(bool isRelaySource, const string& address, const string &vhost, const string &app, const string &streamName,
                                   int bufferChunkSize): isRelaySource(isRelaySource), RtmpSession(bufferChunkSize), address(address){
    this->vhost = vhost;
    this->app = app;
    this->streamName = streamName;
    c1->reserve(1536);
}

void RtmpRelaySession::sessionInit() {
    SessionBase::sessionInit();
    auto c0c1 = ObjPool::allocate<vector<char>>(1536+1, 0);
    (*c0c1)[0] = 3;
    write(c0c1);
}

void RtmpRelaySession::shakeHand(const char &c) {
    switch (status) {
        case 0:{
            status = 1;
            break;
        }
        case 1:{
            c1->push_back(c);
            size--;
            if (size == 0) {
                write(c1);
                status = 2;
                size = 1536;
            }
            break;
        }
        case 2: {
            size--;
            if (size == 0) {
                status = 3;
                connect();
            }
            break;
        }
        default: break;
    }
}

void RtmpRelaySession::connect() {
    ChunkStreamHead connect;
    connect.csId = 3;
    connect.typeId = 20;
    auto connectMsg = ObjPool::allocate<vector<unsigned char>>();
    connectMsg->push_back(2);
    connectMsg->push_back(0);
    connectMsg->push_back(7);
    string tmp = "connect";
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());

    connectMsg->push_back(0);
    double c = 1;
    unsigned long l = *(unsigned long*)&(c);
    connectMsg->push_back((l >> 56) & 0x00000000000000ff);
    connectMsg->push_back((l >> 48) & 0x00000000000000ff);
    connectMsg->push_back((l >> 40) & 0x00000000000000ff);
    connectMsg->push_back((l >> 32) & 0x00000000000000ff);
    connectMsg->push_back((l >> 24) & 0x00000000000000ff);
    connectMsg->push_back((l >> 16) & 0x00000000000000ff);
    connectMsg->push_back((l >> 8) & 0x00000000000000ff);
    connectMsg->push_back(l & 0x00000000000000ff);

    connectMsg->push_back(3);
    connectMsg->push_back(0);
    connectMsg->push_back(3);
    tmp = "app";
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());

    tmp = vhost+"/"+app;
    connectMsg->push_back(2);
    connectMsg->push_back((tmp.size() >> 8) & 0x00ff);
    connectMsg->push_back(tmp.size() & 0x00ff);
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());
    connectMsg->push_back(0);
    connectMsg->push_back(4);
    tmp = "type";
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());
    connectMsg->push_back(2);
    connectMsg->push_back(0);
    connectMsg->push_back(10);
    tmp = "nonprivate";
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());
    connectMsg->push_back(0);
    connectMsg->push_back(8);
    tmp = "flashVer";
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());
    connectMsg->push_back(2);
    connectMsg->push_back(0);
    connectMsg->push_back(36);
    tmp = "FMLE/3.0 (compatible; Lavf57.83.100)";
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());
    connectMsg->push_back(0);
    connectMsg->push_back(5);
    tmp = "tcUrl";
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());
    connectMsg->push_back(2);
    tmp = "rtmp://"+address+"/"+vhost+"/"+app;
    connectMsg->push_back((tmp.size() >> 8) & 0x00ff);
    connectMsg->push_back(tmp.size() & 0x00ff);
    connectMsg->insert(connectMsg->end(), tmp.begin(), tmp.end());
    connectMsg->push_back(0);
    connectMsg->push_back(0);
    connectMsg->push_back(9);
    writeChunk(connect, connectMsg);
}

void RtmpRelaySession::parseCmdMsg() {
    vector<string> type;
    vector<string> value;
    if (head->typeId == 20) {
        parseAMF0(type, value);
        if (value[0] == "_result") {
            unsigned long l = ((unsigned long)((unsigned char)value[1][0])<<56)|
                              ((unsigned long)((unsigned char)value[1][1])<<48)|
                              ((unsigned long)((unsigned char)value[1][2])<<40)|
                              ((unsigned long)((unsigned char)value[1][3])<<32)|
                              ((unsigned long)((unsigned char)value[1][4])<<24)|
                              ((unsigned long)((unsigned char)value[1][5])<<16)|
                              ((unsigned long)((unsigned char)value[1][6])<<8)|
                              (unsigned long)((unsigned char)value[1][7]);
            double n = *(double*)(&l);
            if (n == 1) {
                if (isRelaySource) {
                    createStream(2);
                } else {
                    setChunkSize();
                    releaseStream(2);
                    FCPublish(3);
                    createStream(4);
                }
            }
            if (isRelaySource) {
                if (n == 2) {
                    play();
                }
            } else {
                if (n == 4) {
                    publish();
                    sourceSession = StreamCenter::getStream(vhost, app, streamName);
                    addSink();
                }
            }
        }
    }
}

void RtmpRelaySession::createStream(double num) {
    ChunkStreamHead createStreamHead;
    createStreamHead.csId = 3;
    createStreamHead.typeId = 20;
    shared_ptr<vector<unsigned char>> createStreamMsg = ObjPool::allocate<vector<unsigned char>>();
    createStreamMsg->push_back(2);
    createStreamMsg->push_back(0);
    createStreamMsg->push_back(12);
    string tmp = "createStream";
    createStreamMsg->insert(createStreamMsg->end(), tmp.begin(), tmp.end());
    createStreamMsg->push_back(0);
    unsigned long l = *(unsigned long*)&(num);
    createStreamMsg->push_back((l >> 56) & 0x00000000000000ff);
    createStreamMsg->push_back((l >> 48) & 0x00000000000000ff);
    createStreamMsg->push_back((l >> 40) & 0x00000000000000ff);
    createStreamMsg->push_back((l >> 32) & 0x00000000000000ff);
    createStreamMsg->push_back((l >> 24) & 0x00000000000000ff);
    createStreamMsg->push_back((l >> 16) & 0x00000000000000ff);
    createStreamMsg->push_back((l >> 8) & 0x00000000000000ff);
    createStreamMsg->push_back(l & 0x00000000000000ff);

    createStreamMsg->push_back(5);
    writeChunk(createStreamHead, createStreamMsg);
}

void RtmpRelaySession::play() {
    ChunkStreamHead getStreamLengthHead;
    getStreamLengthHead.csId = 8;
    getStreamLengthHead.typeId = 20;
    shared_ptr<vector<unsigned char>> getStreamLengthMsg = ObjPool::allocate<vector<unsigned char>>();
    getStreamLengthMsg->push_back(2);
    getStreamLengthMsg->push_back(0);
    getStreamLengthMsg->push_back(15);
    string tmp = "getStreamLength";
    getStreamLengthMsg->insert(getStreamLengthMsg->end(), tmp.begin(), tmp.end());
    getStreamLengthMsg->push_back(0);
    double c = 3;
    unsigned long l = *(unsigned long*)&(c);
    getStreamLengthMsg->push_back((l >> 56) & 0x00000000000000ff);
    getStreamLengthMsg->push_back((l >> 48) & 0x00000000000000ff);
    getStreamLengthMsg->push_back((l >> 40) & 0x00000000000000ff);
    getStreamLengthMsg->push_back((l >> 32) & 0x00000000000000ff);
    getStreamLengthMsg->push_back((l >> 24) & 0x00000000000000ff);
    getStreamLengthMsg->push_back((l >> 16) & 0x00000000000000ff);
    getStreamLengthMsg->push_back((l >> 8) & 0x00000000000000ff);
    getStreamLengthMsg->push_back(l & 0x00000000000000ff);

    getStreamLengthMsg->push_back(5);
    getStreamLengthMsg->push_back(2);
    tmp = streamName;
    getStreamLengthMsg->push_back((tmp.size() >> 8) & 0x00ff);
    getStreamLengthMsg->push_back(tmp.size() & 0x00ff);
    getStreamLengthMsg->insert(getStreamLengthMsg->end(), tmp.begin(), tmp.end());
    writeChunk(getStreamLengthHead, getStreamLengthMsg);

    ChunkStreamHead playHead;
    playHead.csId = 8;
    playHead.typeId = 20;
    shared_ptr<vector<unsigned char>> playMsg = ObjPool::allocate<vector<unsigned char>>();
    playMsg->push_back(2);
    playMsg->push_back(0);
    playMsg->push_back(4);
    tmp = "play";
    playMsg->insert(playMsg->end(), tmp.begin(), tmp.end());
    playMsg->push_back(0);
    c = 4;
    l = *(unsigned long*)&(c);
    playMsg->push_back((l >> 56) & 0x00000000000000ff);
    playMsg->push_back((l >> 48) & 0x00000000000000ff);
    playMsg->push_back((l >> 40) & 0x00000000000000ff);
    playMsg->push_back((l >> 32) & 0x00000000000000ff);
    playMsg->push_back((l >> 24) & 0x00000000000000ff);
    playMsg->push_back((l >> 16) & 0x00000000000000ff);
    playMsg->push_back((l >> 8) & 0x00000000000000ff);
    playMsg->push_back(l & 0x00000000000000ff);

    playMsg->push_back(5);
    playMsg->push_back(2);
    tmp = streamName;
    playMsg->push_back((tmp.size() >> 8) & 0x00ff);
    playMsg->push_back(tmp.size() & 0x00ff);
    playMsg->insert(playMsg->end(), tmp.begin(), tmp.end());

    playMsg->push_back(0);
    c = -2000;
    l = *(unsigned long*)&(c);
    playMsg->push_back((l >> 56) & 0x00000000000000ff);
    playMsg->push_back((l >> 48) & 0x00000000000000ff);
    playMsg->push_back((l >> 40) & 0x00000000000000ff);
    playMsg->push_back((l >> 32) & 0x00000000000000ff);
    playMsg->push_back((l >> 24) & 0x00000000000000ff);
    playMsg->push_back((l >> 16) & 0x00000000000000ff);
    playMsg->push_back((l >> 8) & 0x00000000000000ff);
    playMsg->push_back(l & 0x00000000000000ff);
    writeChunk(playHead, playMsg);

    ChunkStreamHead setBufferLengthHead;
    setBufferLengthHead.csId = 2;
    setBufferLengthHead.typeId = 4;
    setBufferLengthHead.time = 1;
    shared_ptr<vector<unsigned char>> setBufferLengthMsg = ObjPool::allocate<vector<unsigned char>>();
    setBufferLengthMsg->push_back(0);
    setBufferLengthMsg->push_back(3);
    setBufferLengthMsg->push_back(0);
    setBufferLengthMsg->push_back(0);
    setBufferLengthMsg->push_back(0);
    setBufferLengthMsg->push_back(1);
    setBufferLengthMsg->push_back(0);
    setBufferLengthMsg->push_back(0);
    setBufferLengthMsg->push_back(11);
    setBufferLengthMsg->push_back(184);
    writeChunk(setBufferLengthHead, setBufferLengthMsg);
}

void RtmpRelaySession::publish() {
    ChunkStreamHead publishHead;
    publishHead.csId = 8;
    publishHead.typeId = 20;
    shared_ptr<vector<unsigned char>> publishMsg = ObjPool::allocate<vector<unsigned char>>();
    publishMsg->push_back(2);
    publishMsg->push_back(0);
    publishMsg->push_back(7);
    string tmp = "publish";
    publishMsg->insert(publishMsg->end(), tmp.begin(), tmp.end());
    publishMsg->push_back(0);
    double c = 5;
    unsigned long l = *(unsigned long*)&(c);
    publishMsg->push_back((l >> 56) & 0x00000000000000ff);
    publishMsg->push_back((l >> 48) & 0x00000000000000ff);
    publishMsg->push_back((l >> 40) & 0x00000000000000ff);
    publishMsg->push_back((l >> 32) & 0x00000000000000ff);
    publishMsg->push_back((l >> 24) & 0x00000000000000ff);
    publishMsg->push_back((l >> 16) & 0x00000000000000ff);
    publishMsg->push_back((l >> 8) & 0x00000000000000ff);
    publishMsg->push_back(l & 0x00000000000000ff);

    publishMsg->push_back(5);
    publishMsg->push_back(2);
    tmp = streamName;
    publishMsg->push_back((tmp.size() >> 8) & 0x00ff);
    publishMsg->push_back(tmp.size() & 0x00ff);
    publishMsg->insert(publishMsg->end(), tmp.begin(), tmp.end());

    publishMsg->push_back(2);
    publishMsg->push_back(0);
    publishMsg->push_back(4);
    tmp = "live";
    publishMsg->insert(publishMsg->end(), tmp.begin(), tmp.end());
    writeChunk(publishHead, publishMsg);
}

void RtmpRelaySession::setChunkSize() {
    chunkSize = ConfigSystem::getConfig()["live_stream_server"]["rtmp_chunk_size"].asInt();
    ChunkStreamHead setChunkSizeHead;
    setChunkSizeHead.csId = 2;
    setChunkSizeHead.typeId = 1;
    shared_ptr<vector<unsigned char>> setChunkSizeMsg = ObjPool::allocate<vector<unsigned char>>();
    setChunkSizeMsg->push_back((chunkSize >> 24) & 0x000000ff);
    setChunkSizeMsg->push_back((chunkSize >> 16) & 0x000000ff);
    setChunkSizeMsg->push_back((chunkSize >> 8) & 0x000000ff);
    setChunkSizeMsg->push_back(chunkSize & 0x000000ff);
    writeChunk(setChunkSizeHead, setChunkSizeMsg);
}

void RtmpRelaySession::FCPublish(double num) {
    ChunkStreamHead FCPublishHead;
    FCPublishHead.csId = 3;
    FCPublishHead.typeId = 20;
    shared_ptr<vector<unsigned char>> FCPublishMsg = ObjPool::allocate<vector<unsigned char>>();
    FCPublishMsg->push_back(2);
    FCPublishMsg->push_back(0);
    FCPublishMsg->push_back(9);
    string tmp = "FCPublish";
    FCPublishMsg->insert(FCPublishMsg->end(), tmp.begin(), tmp.end());
    FCPublishMsg->push_back(0);
    unsigned long l = *(unsigned long*)&(num);
    FCPublishMsg->push_back((l >> 56) & 0x00000000000000ff);
    FCPublishMsg->push_back((l >> 48) & 0x00000000000000ff);
    FCPublishMsg->push_back((l >> 40) & 0x00000000000000ff);
    FCPublishMsg->push_back((l >> 32) & 0x00000000000000ff);
    FCPublishMsg->push_back((l >> 24) & 0x00000000000000ff);
    FCPublishMsg->push_back((l >> 16) & 0x00000000000000ff);
    FCPublishMsg->push_back((l >> 8) & 0x00000000000000ff);
    FCPublishMsg->push_back(l & 0x00000000000000ff);

    FCPublishMsg->push_back(5);
    FCPublishMsg->push_back(2);
    tmp = streamName;
    FCPublishMsg->push_back((tmp.size() >> 8) & 0x00ff);
    FCPublishMsg->push_back(tmp.size() & 0x00ff);
    FCPublishMsg->insert(FCPublishMsg->end(), tmp.begin(), tmp.end());
    writeChunk(FCPublishHead, FCPublishMsg);
}

void RtmpRelaySession::releaseStream(double num) {
    ChunkStreamHead releaseStreamHead;
    releaseStreamHead.csId = 3;
    releaseStreamHead.typeId = 20;
    shared_ptr<vector<unsigned char>> releaseStreamMsg = ObjPool::allocate<vector<unsigned char>>();
    releaseStreamMsg->push_back(2);
    releaseStreamMsg->push_back(0);
    releaseStreamMsg->push_back(13);
    string tmp = "releaseStream";
    releaseStreamMsg->insert(releaseStreamMsg->end(), tmp.begin(), tmp.end());
    releaseStreamMsg->push_back(0);
    unsigned long l = *(unsigned long*)&(num);
    releaseStreamMsg->push_back((l >> 56) & 0x00000000000000ff);
    releaseStreamMsg->push_back((l >> 48) & 0x00000000000000ff);
    releaseStreamMsg->push_back((l >> 40) & 0x00000000000000ff);
    releaseStreamMsg->push_back((l >> 32) & 0x00000000000000ff);
    releaseStreamMsg->push_back((l >> 24) & 0x00000000000000ff);
    releaseStreamMsg->push_back((l >> 16) & 0x00000000000000ff);
    releaseStreamMsg->push_back((l >> 8) & 0x00000000000000ff);
    releaseStreamMsg->push_back(l & 0x00000000000000ff);

    releaseStreamMsg->push_back(5);
    releaseStreamMsg->push_back(2);
    tmp = streamName;
    releaseStreamMsg->push_back((tmp.size() >> 8) & 0x00ff);
    releaseStreamMsg->push_back(tmp.size() & 0x00ff);
    releaseStreamMsg->insert(releaseStreamMsg->end(), tmp.begin(), tmp.end());
    writeChunk(releaseStreamHead, releaseStreamMsg);
}
