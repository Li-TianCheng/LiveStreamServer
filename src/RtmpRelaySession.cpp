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

void RtmpRelaySession::shakeHand(iter& pos) {
    switch (status) {
        case 0:{
            status = 1;
            idx++;
            break;
        }
        case 1:{
            size_t inc = size < msgLength-idx ? size : msgLength-idx;
            copy(pos+idx, inc, *c1);
            size -= inc;
            idx += inc;
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
            idx++;
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
    writeAMF0String(connectMsg, "connect");
    writeAMF0Num(connectMsg, 1);

    connectMsg->push_back(3);
    writeAMF0Key(connectMsg, "app");
    writeAMF0String(connectMsg, vhost+"/"+app);
    writeAMF0Key(connectMsg, "type");
    writeAMF0String(connectMsg, "nonprivate");
    writeAMF0Key(connectMsg, "flashVer");
    writeAMF0String(connectMsg, "FMLE/3.0 (compatible; Lavf57.83.100)");
    writeAMF0Key(connectMsg, "tcUrl");
    writeAMF0String(connectMsg, "rtmp://"+address+"/"+vhost+"/"+app);
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
    writeAMF0String(createStreamMsg, "createStream");
    writeAMF0Num(createStreamMsg, num);
    createStreamMsg->push_back(5);
    writeChunk(createStreamHead, createStreamMsg);
}

void RtmpRelaySession::play() {
    ChunkStreamHead getStreamLengthHead;
    getStreamLengthHead.csId = 8;
    getStreamLengthHead.typeId = 20;
    shared_ptr<vector<unsigned char>> getStreamLengthMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(getStreamLengthMsg, "getStreamLength");
    writeAMF0Num(getStreamLengthMsg, 3);
    getStreamLengthMsg->push_back(5);
    writeAMF0String(getStreamLengthMsg, streamName);
    writeChunk(getStreamLengthHead, getStreamLengthMsg);

    ChunkStreamHead playHead;
    playHead.csId = 8;
    playHead.typeId = 20;
    shared_ptr<vector<unsigned char>> playMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(playMsg, "play");
    writeAMF0Num(playMsg, 4);
    playMsg->push_back(5);
    writeAMF0String(playMsg, streamName);
    writeAMF0Num(playMsg, -2000);
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
    writeAMF0String(publishMsg, "publish");
    writeAMF0Num(publishMsg, 5);
    publishMsg->push_back(5);
    writeAMF0String(publishMsg, streamName);
    writeAMF0String(publishMsg, "live");
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
    writeAMF0String(FCPublishMsg, "FCPublish");
    writeAMF0Num(FCPublishMsg, num);
    FCPublishMsg->push_back(5);
    writeAMF0String(FCPublishMsg, streamName);
    writeChunk(FCPublishHead, FCPublishMsg);
}

void RtmpRelaySession::releaseStream(double num) {
    ChunkStreamHead releaseStreamHead;
    releaseStreamHead.csId = 3;
    releaseStreamHead.typeId = 20;
    shared_ptr<vector<unsigned char>> releaseStreamMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(releaseStreamMsg, "releaseStream");
    writeAMF0Num(releaseStreamMsg, num);
    releaseStreamMsg->push_back(5);
    writeAMF0String(releaseStreamMsg, streamName);
    writeChunk(releaseStreamHead, releaseStreamMsg);
}
