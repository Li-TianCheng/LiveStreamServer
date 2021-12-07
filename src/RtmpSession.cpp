//
// Created by ltc on 2021/7/21.
//

#include "RtmpSession.h"

RtmpSession::RtmpSession(int bufferChunkSize): status(0), size(1536), chunkStatus(0), c1(ObjPool::allocate<vector<unsigned char>>()),
                                  remoteChunkSize(128), remoteWindowAckSize(2500000), chunkSize(128), windowAckSize(2500000),
                                  received(0), ackReceived(0), SessionBase(true, bufferChunkSize), extend(false) {
    c1->reserve(1536);
}

void RtmpSession::handleReadDone(iter pos, size_t n) {
    SessionBase::handleReadDone(pos, n);
    idx = 0;
    msgLength = n;
    while (idx < n) {
        if (status != 3) {
            shakeHand(pos);
        } else {
            parseHead(pos);
        }
    }
    readDone(n);
}

void RtmpSession::shakeHand(iter& pos) {
    switch (status) {
        case 0:{
            auto s0s1 = ObjPool::allocate<vector<char>>(1536+1, 0);
            (*s0s1)[0] = 3;
            write(s0s1);
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
            }
            idx++;
            break;
        }
        default: break;
    }
}

void RtmpSession::parseHead(iter& pos) {
    switch (chunkStatus) {
        case 0: {
            unsigned char c = *(pos+idx);
            fmt = c >> 6;
            csId = c & 0x3f;
            if (fmt == 0) {
                msgHeaderSize = 11;
            } else if (fmt == 1) {
                msgHeaderSize = 7;
            } else if (fmt == 2) {
                msgHeaderSize = 3;
            } else {
                msgHeaderSize = 0;
            }
            if (csId == 0) {
                csId = 0;
                basicHeaderSize = 1;
            } else if (csId == 1) {
                csId = 0;
                basicHeaderSize = 2;
            } else {
                head = &chunkMap[csId];
                head->csId = csId;
                basicHeaderSize = 0;
            }
            if (msgHeaderSize == 0) {
                if (basicHeaderSize != 0) {
                    size = 0;
                    chunkStatus = 1;
                } else {
                    if (extend) {
                        size = 0;
                        chunkStatus = 3;
                    } else {
                        size = 0;
                        chunkStatus = 4;
                    }
                }
            } else {
                if (basicHeaderSize == 0) {
                    size = 0;
                    chunkStatus = 2;
                } else {
                    size = 0;
                    chunkStatus = 1;
                }
            }
            idx++;
            break;
        }
        case 1: {
            unsigned char c = *(pos+idx);
            size++;
            csId = (csId << 8) | ((unsigned char)c);
            if (size == basicHeaderSize) {
                csId += 64;
                head = &chunkMap[csId];
                head->csId = csId;
                if (msgHeaderSize == 0) {
                    if (extend) {
                        size = 0;
                        chunkStatus = 3;
                    } else {
                        size = 0;
                        chunkStatus = 4;
                    }
                } else {
                    size = 0;
                    chunkStatus = 2;
                }
            }
            idx++;
            break;
        }
        case 2: {
            unsigned char c = *(pos+idx);
            size++;
            if (size == 1) {
                time = 0;
                if (fmt == 0) {
                    head->time = 0;
                    head->timeDelta = 0;
                    head->length = 0;
                    head->streamId = 0;
                }
                if (fmt == 1) {
                    head->timeDelta = 0;
                    head->length = 0;
                }
                if (fmt == 2) {
                    head->timeDelta = 0;
                }
            }
            if (size <= 3) {
                time = (time << 8) | ((unsigned char)c);
                if (size == 3) {
                    if (time == 0xffffff) {
                        extend = true;
                        time = 0;
                    } else {
                        extend = false;
                        if (fmt == 0) {
                            head->time = time;
                        } else {
                            head->timeDelta = time;
                        }
                    }
                }
            } else if (size <= 6) {
                head->length = (head->length << 8) | ((unsigned char)c);
            } else if (size <= 7) {
                head->typeId = (unsigned char)c;
            } else if (size <= 11) {
                head->streamId = head->streamId | ((unsigned int)c << (8*(size-8)));
            }
            if (size == msgHeaderSize) {
                if (extend) {
                    size = 0;
                    chunkStatus = 3;
                } else {
                    size = 0;
                    chunkStatus = 4;
                }
            }
            idx++;
            break;
        }
        case 3: {
            unsigned char c = *(pos+idx);
            size++;
            time = (time << 8) | ((unsigned char)c);
            if (size == 4) {
                if (fmt == 0) {
                    head->time = time;
                } else {
                    head->timeDelta = time;
                }
                size = 0;
                chunkStatus = 4;
                extend = false;
            }
            idx++;
            break;
        }
        case 4: {
            size_t remain = head->length-head->idx < remoteChunkSize-size ? head->length-head->idx : remoteChunkSize-size;
            size_t inc = remain < msgLength-idx ? remain : msgLength-idx;
            if (head->idx == 0) {
                head->time += head->timeDelta;
            }
            if (head->typeId == 1 || head->typeId == 5 || head->typeId == 17 || head->typeId == 20) {
                copy(pos+idx, inc, cmd);
            } else if (head->typeId == 8 || head->typeId == 9 || head->typeId == 18) {
                if (head->idx == 0) {
                    stream.currTag->push_back(head->typeId);
                    stream.currTag->push_back((head->length >> 16) & 0x0000ff);
                    stream.currTag->push_back((head->length >> 8) & 0x0000ff);
                    stream.currTag->push_back(head->length & 0x0000ff);
                    stream.currTag->push_back((head->time >> 16) & 0x000000ff);
                    stream.currTag->push_back((head->time >> 8) & 0x000000ff);
                    stream.currTag->push_back(head->time & 0x000000ff);
                    stream.currTag->push_back((head->time >> 24) & 0x000000ff);
                    stream.currTag->push_back(0);
                    stream.currTag->push_back(0);
                    stream.currTag->push_back(0);
                }
                copy(pos+idx, inc, *stream.currTag);
            }
            size += inc;
            head->idx += inc;
            idx += inc;
            if (head->idx == head->length) {
                ack();
                if (head->typeId == 1 || head->typeId == 5) {
                    parseControlMsg();
                    cmd.clear();
                } else if (head->typeId == 17 || head->typeId == 20) {
                    parseCmdMsg();
                    cmd.clear();
                } else if (head->typeId == 8 || head->typeId == 9 || head->typeId == 18) {
                    stream.currTag->push_back(((head->length+11)  >> 24) & 0x000000ff);
                    stream.currTag->push_back(((head->length+11)  >> 16) & 0x000000ff);
                    stream.currTag->push_back(((head->length+11)  >> 8) & 0x000000ff);
                    stream.currTag->push_back((head->length+11) & 0x000000ff);
                    parseTag();
                }
                head->idx = 0;
                size = 0;
                chunkStatus = 0;
            } else if (size == remoteChunkSize) {
                size = 0;
                chunkStatus = 0;
            }
            break;
        }
        default: break;
    }
}

void RtmpSession::parseCmdMsg() {
    vector<string> type;
    vector<string> value;
    if (head->typeId == 20) {
        parseAMF0(type, value);
        if (value[0] == "connect") {
            vector<string> split = utils::split(value[3], '/', 2);
            if (split.size() == 2) {
                vhost = split[0];
                app = split[1];
            } else {
                deleteSession();
            }
            responseConnect(type, value);
        }
        if (value[0] == "createStream") {
            responseCreateStream(type, value);
        }
        if (value[0] == "publish") {
            responsePublish(type, value);
        }
        if (value[0] == "play") {
            streamName = value[2];
            sourceSession = StreamCenter::getStream(isRelay, vhost, app, streamName);
            if (sourceSession.lock() != nullptr) {
                responsePlay(type, value);
                addSink();
            }
        }
        if (value[0] == "releaseStream") {
            streamName = value[2];
        }
    }
}

void RtmpSession::ack() {
    received += head->length;
    ackReceived += head->length;
    if (received >= 0xf0000000) {
        received = 0;
    }
    if (ackReceived >= remoteWindowAckSize) {
        ChunkStreamHead head;
        head.csId = 2;
        head.typeId = 3;
        auto msg = ObjPool::allocate<vector<unsigned char>>();
        msg->push_back((chunkSize >> 24) & 0x000000ff);
        msg->push_back((chunkSize >> 16) & 0x000000ff);
        msg->push_back((chunkSize >> 8) & 0x000000ff);
        msg->push_back(chunkSize & 0x000000ff);
        writeChunk(head, msg);
        ackReceived = 0;
    }
}

void RtmpSession::parseControlMsg() {
    if (head->typeId == 1) {
        remoteChunkSize = 0;
        remoteChunkSize = ((unsigned int)cmd[0] << 24) | ((unsigned int)cmd[1] << 16) | ((unsigned int)cmd[2] << 8) | (unsigned int)cmd[3];
    } else if (head->typeId == 5) {
        remoteWindowAckSize = 0;
        remoteWindowAckSize = ((unsigned int)cmd[0] << 24) | ((unsigned int)cmd[1] << 16) | ((unsigned int)cmd[2] << 8) | (unsigned int)cmd[3];
    }
}

void RtmpSession::parseAMF0(vector<string> &type, vector<string> &value) {
    bool isObj = false;
    for (int i = 0; i < cmd.size();) {
        switch (cmd[i]) {
            case 0: {
                type.emplace_back("double");
                value.emplace_back();
                value.back().insert(value.back().end(), cmd.begin()+i+1, cmd.begin()+i+9);
                i += 9;
                if (isObj) {
                    goto OBJ;
                }
                break;
            }
            case 1: {
                type.emplace_back("bool");
                value.emplace_back();
                value.back().insert(value.back().end(), cmd.begin()+i+1, cmd.begin()+i+2);
                i += 2;
                if (isObj) {
                    goto OBJ;
                }
                break;
            }
            case 2: {
                type.emplace_back("string");
                int length = (cmd[i+1] << 8) | cmd[i+2];
                value.emplace_back();
                value.back().insert(value.back().end(), cmd.begin()+i+3, cmd.begin()+i+3+length);
                i += 3 + length;
                if (isObj) {
                    goto OBJ;
                }
                break;
            }
            case 5: {
                i++;
                if (isObj) {
                    goto OBJ;
                }
                break;
            }
            case 3: {
                i++;
                isObj = true;
OBJ:            if (cmd[i] == 0 && cmd[i+1] == 0 && cmd[i+2] == 9) {
                    i += 3;
                    isObj = false;
                    break;
                }
                int length = (cmd[i] << 8) | cmd[i+1];
                type.emplace_back("key");
                value.emplace_back();
                value.back().insert(value.back().end(), cmd.begin()+i+2, cmd.begin()+i+2+length);
                i += 2 + length;
                break;
            }
        }
    }
}

void RtmpSession::parseAMF3(vector<string> &type, vector<string> &value) {
    //TODO: AMF3
}

void RtmpSession::writeChunk(ChunkStreamHead &head, shared_ptr<vector<unsigned char>> msg) {
    shared_ptr<vector<unsigned char>> chunk = ObjPool::allocate<vector<unsigned char>>();
    head.length = msg->size();
    if (head.typeId == 8) {
        head.csId = 4;
    } else if (head.typeId == 9 || head.typeId == 18 || head.typeId == 0xf) {
        head.csId = 6;
    }
    unsigned int num = head.length / chunkSize;
    int total = 0;
    for (int i = 0; i <= num; i++) {
        if (total == head.length) {
            break;
        }
        if (i == 0) {
            chunk->push_back(head.csId);
            chunk->push_back((head.time >> 16) & 0x0000ff);
            chunk->push_back((head.time >> 8) & 0x0000ff);
            chunk->push_back(head.time & 0x0000ff);
            chunk->push_back((head.length >> 16) & 0x0000ff);
            chunk->push_back((head.length >> 8) & 0x0000ff);
            chunk->push_back(head.length & 0x0000ff);
            chunk->push_back(head.typeId);
            chunk->push_back(head.streamId & 0x000000ff);
            chunk->push_back((head.streamId >> 8) & 0x000000ff);
            chunk->push_back((head.streamId >> 16) & 0x000000ff);
            chunk->push_back((head.streamId >> 24) & 0x000000ff);
        } else {
            chunk->push_back(head.csId | 0xc0);
        }
        write(chunk);
        chunk = ObjPool::allocate<vector<unsigned char>>();
        int inc = chunkSize;
        if (head.length - i*chunkSize <= inc) {
            inc = head.length - i*chunkSize;
        }
        total += inc;
        write(msg, i*chunkSize, i*chunkSize+inc);
    }
}

void RtmpSession::responseConnect(vector<string> &type, vector<string> &value) {
    ChunkStreamHead windowAckHead;
    windowAckHead.csId = 2;
    windowAckHead.typeId = 5;
    shared_ptr<vector<unsigned char>> windowAckMsg = ObjPool::allocate<vector<unsigned char>>();
    windowAckMsg->push_back((windowAckSize >> 24) & 0x000000ff);
    windowAckMsg->push_back((windowAckSize >> 16) & 0x000000ff);
    windowAckMsg->push_back((windowAckSize >> 8) & 0x000000ff);
    windowAckMsg->push_back(windowAckSize & 0x000000ff);
    writeChunk(windowAckHead, windowAckMsg);

    ChunkStreamHead setPeerBandWidthHead;
    setPeerBandWidthHead.csId = 2;
    setPeerBandWidthHead.typeId = 6;
    shared_ptr<vector<unsigned char>> setPeerBandWidthMsg = ObjPool::allocate<vector<unsigned char>>();
    setPeerBandWidthMsg->push_back((2500000 >> 24) & 0x000000ff);
    setPeerBandWidthMsg->push_back((2500000 >> 16) & 0x000000ff);
    setPeerBandWidthMsg->push_back((2500000 >> 8) & 0x000000ff);
    setPeerBandWidthMsg->push_back(2500000 & 0x000000ff);
    setPeerBandWidthMsg->push_back(2);
    writeChunk(setPeerBandWidthHead, setPeerBandWidthMsg);

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

    ChunkStreamHead resp;
    resp.csId = head->csId;
    resp.streamId = head->streamId;
    resp.typeId = 20;
    shared_ptr<vector<unsigned char>> respMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(respMsg, "_result");

    respMsg->push_back(0);
    respMsg->insert(respMsg->end(), value[1].begin(), value[1].end());

    respMsg->push_back(3);
    writeAMF0Key(respMsg, "fmsVer");
    writeAMF0String(respMsg, "FMS/3,0,1,123");
    writeAMF0Key(respMsg, "capabilities");
    writeAMF0Num(respMsg, 31);
    respMsg->push_back(0);
    respMsg->push_back(0);
    respMsg->push_back(9);

    respMsg->push_back(3);
    writeAMF0Key(respMsg, "level");
    writeAMF0String(respMsg, "status");
    writeAMF0Key(respMsg, "code");
    writeAMF0String(respMsg, "NetConnection.Connect.Success");
    writeAMF0Key(respMsg, "description");
    writeAMF0String(respMsg, "Connection succeeded.");
    writeAMF0Key(respMsg, "objectEncoding");
    writeAMF0Num(respMsg, 0);
    respMsg->push_back(0);
    respMsg->push_back(0);
    respMsg->push_back(9);
    writeChunk(resp, respMsg);
}

void RtmpSession::responseCreateStream(vector<string> &type, vector<string> &value) {
    ChunkStreamHead resp;
    resp.csId = head->csId;
    resp.streamId = head->streamId;
    resp.typeId = 20;
    shared_ptr<vector<unsigned char>> respMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(respMsg, "_result");

    respMsg->push_back(0);
    respMsg->insert(respMsg->end(), value[1].begin(), value[1].end());
    respMsg->push_back(5);

    writeAMF0Num(respMsg, 1);
    writeChunk(resp, respMsg);
}

void RtmpSession::responsePublish(vector<string> &type, vector<string> &value) {
    ChunkStreamHead resp;
    resp.csId = head->csId;
    resp.streamId = head->streamId;
    resp.typeId = 20;
    shared_ptr<vector<unsigned char>> respMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(respMsg, "onStatus");
    writeAMF0Num(respMsg, 0);

    respMsg->push_back(5);

    respMsg->push_back(3);
    writeAMF0Key(respMsg, "level");
    writeAMF0String(respMsg, "status");
    writeAMF0Key(respMsg, "code");
    writeAMF0String(respMsg, "NetStream.Publish.Start");
    writeAMF0Key(respMsg, "description");
    writeAMF0String(respMsg, "Start publishing.");
    respMsg->push_back(0);
    respMsg->push_back(0);
    respMsg->push_back(9);
    writeChunk(resp, respMsg);
}

void RtmpSession::responsePlay(vector<string> &type, vector<string> &value) {
    ChunkStreamHead resp;
    resp.csId = head->csId;
    resp.streamId = head->streamId;
    resp.typeId = 20;
    shared_ptr<vector<unsigned char>> respMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(respMsg, "onStatus");
    writeAMF0Num(respMsg, 0);

    respMsg->push_back(5);

    respMsg->push_back(3);
    writeAMF0Key(respMsg, "level");
    writeAMF0String(respMsg, "status");
    writeAMF0Key(respMsg, "code");
    writeAMF0String(respMsg, "NetStream.Play.Reset");
    writeAMF0Key(respMsg, "description");
    writeAMF0String(respMsg, "Playing and resetting stream.");
    respMsg->push_back(0);
    respMsg->push_back(0);
    respMsg->push_back(9);
    writeChunk(resp, respMsg);

    respMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(respMsg, "onStatus");
    writeAMF0Num(respMsg, 0);

    respMsg->push_back(5);

    respMsg->push_back(3);
    writeAMF0Key(respMsg, "level");
    writeAMF0String(respMsg, "status");
    writeAMF0Key(respMsg, "code");
    writeAMF0String(respMsg, "NetStream.Play.Start");
    writeAMF0Key(respMsg, "description");
    writeAMF0String(respMsg, "Started playing stream.");
    respMsg->push_back(0);
    respMsg->push_back(0);
    respMsg->push_back(9);
    writeChunk(resp, respMsg);

    respMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(respMsg, "onStatus");
    writeAMF0Num(respMsg, 0);

    respMsg->push_back(5);

    respMsg->push_back(3);
    writeAMF0Key(respMsg, "level");
    writeAMF0String(respMsg, "status");
    writeAMF0Key(respMsg, "code");
    writeAMF0String(respMsg, "NetStream.Data.Start");
    writeAMF0Key(respMsg, "description");
    writeAMF0String(respMsg, "Started playing stream.");
    respMsg->push_back(0);
    respMsg->push_back(0);
    respMsg->push_back(9);
    writeChunk(resp, respMsg);

    respMsg = ObjPool::allocate<vector<unsigned char>>();
    writeAMF0String(respMsg, "onStatus");
    writeAMF0Num(respMsg, 0);

    respMsg->push_back(5);

    respMsg->push_back(3);
    writeAMF0Key(respMsg, "level");
    writeAMF0String(respMsg, "status");
    writeAMF0Key(respMsg, "code");
    writeAMF0String(respMsg, "NetStream.Play.PublishNotify");
    writeAMF0Key(respMsg, "description");
    writeAMF0String(respMsg, "Started playing notify.");
    respMsg->push_back(0);
    respMsg->push_back(0);
    respMsg->push_back(9);
    writeChunk(resp, respMsg);
}

void RtmpSession::writeAMF0Num(shared_ptr<vector<unsigned char>> chunk, double num) {
    chunk->push_back(0);
    unsigned long l = *(unsigned long*)&(num);
    chunk->push_back((l >> 56) & 0x00000000000000ff);
    chunk->push_back((l >> 48) & 0x00000000000000ff);
    chunk->push_back((l >> 40) & 0x00000000000000ff);
    chunk->push_back((l >> 32) & 0x00000000000000ff);
    chunk->push_back((l >> 24) & 0x00000000000000ff);
    chunk->push_back((l >> 16) & 0x00000000000000ff);
    chunk->push_back((l >> 8) & 0x00000000000000ff);
    chunk->push_back(l & 0x00000000000000ff);
}

void RtmpSession::writeAMF0String(shared_ptr<vector<unsigned char>> chunk, const string& str) {
    chunk->push_back(2);
    chunk->push_back((str.size() >> 8) & 0x00ff);
    chunk->push_back(str.size() & 0x00ff);
    chunk->insert(chunk->end(), str.begin(), str.end());
}

void RtmpSession::writeAMF0Key(shared_ptr<vector<unsigned char>> chunk, const string &str) {
    chunk->push_back((str.size() >> 8) & 0x00ff);
    chunk->push_back(str.size() & 0x00ff);
    chunk->insert(chunk->end(), str.begin(), str.end());
}
