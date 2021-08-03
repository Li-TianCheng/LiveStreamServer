//
// Created by ltc on 2021/7/21.
//

#include "RtmpFlvSession.h"

RtmpFlvSession::RtmpFlvSession(int bufferChunkSize): status(0), size(1536), chunkStatus(0), c1(ObjPool::allocate<vector<unsigned char>>()),
                                  remoteChunkSize(128), remoteWindowAckSize(2500000), chunkSize(128), windowAckSize(2500000),
                                  received(0), ackReceived(0), FlvSessionBase(bufferChunkSize){
    c1->reserve(1536);
}

void RtmpFlvSession::writeFlv(shared_ptr<vector<unsigned char>> tag) {

}

void RtmpFlvSession::handleReadDone(iter pos, size_t n) {
    for (size_t i = 0; i < n; i++){
        if (status != 3) {
            shakeHand(*(pos++));
        } else {
            parseHead(*(pos++));
        }
    }
    readDone(n);
}

void RtmpFlvSession::shakeHand(const char &c) {
    switch (status) {
        case 0:{
            auto s0s1 = ObjPool::allocate<vector<char>>(1536+1, 0);
            (*s0s1)[0] = 3;
            write(s0s1);
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
            }
            break;
        }
        default: break;
    }
}

void RtmpFlvSession::writeFlvHead() {
    FlvSessionBase::writeFlvHead();
}

void RtmpFlvSession::parseHead(const char &c) {
    switch (chunkStatus) {
        case 0: {
            head.fmt = (unsigned char)c >> 6;
            head.csId = (unsigned char)c & 0x3f;
            if (head.fmt == 0) {
                head.msgHeaderSize = 11;
            } else if (head.fmt == 1) {
                head.msgHeaderSize = 7;
            } else if (head.fmt == 2) {
                head.msgHeaderSize = 3;
            } else {
                head.msgHeaderSize = 0;
            }
            if (head.csId == 0) {
                head.csId = 0;
                head.basicHeaderSize = 1;
            } else if (head.csId == 1) {
                head.csId = 0;
                head.basicHeaderSize = 2;
            } else {
                head.basicHeaderSize = 0;
            }
            if (head.msgHeaderSize == 0) {
                if (head.basicHeaderSize != 0) {
                    size = 0;
                    chunkStatus = 1;
                } else {
                    if (head.extend) {
                        size = 0;
                        chunkStatus = 3;
                    } else {
                        size = 0;
                        chunkStatus = 4;
                    }
                }
            } else {
                if (head.basicHeaderSize == 0) {
                    size = 0;
                    chunkStatus = 2;
                } else {
                    size = 0;
                    chunkStatus = 1;
                }
            }
            break;
        }
        case 1: {
            size++;
            head.csId = (head.csId << 8) | ((unsigned char)c);
            if (size == head.basicHeaderSize) {
                head.csId += 64;
                if (head.msgHeaderSize == 0) {
                    if (head.extend) {
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
            break;
        }
        case 2: {
            size++;
            if (size <= 3) {
                head.time = (head.time << 8) | ((unsigned char)c);
            } else if (size <= 6) {
                head.length = (head.length << 8) | ((unsigned char)c);
            } else if (size <= 7) {
                head.typeId = (unsigned char)c;
            }
            if (size == head.msgHeaderSize) {
                if (head.time == 0xffffff) {
                    head.extend = true;
                    size = 0;
                    head.time = 0;
                    chunkStatus = 3;
                } else {
                    size = 0;
                    chunkStatus = 4;
                }
            }
            break;
        }
        case 3: {
            size++;
            if (size == 4) {
                size = 0;
                chunkStatus = 4;
            }
            break;
        }
        case 4: {
            size++;
            head.idx++;
            if (head.typeId == 1 || head.typeId == 5) {
                parseControlMsg(c);
            } else if (head.typeId == 17 || head.typeId == 20) {
                parseCmdMsg(c);
            }
            if (head.idx == head.length) {
                head.idx = 0;
                size = 0;
                chunkStatus = 0;
                ack();
            } else if (size == remoteChunkSize) {
                size = 0;
                chunkStatus = 0;
            }
            break;
        }
        default: break;
    }
}

void RtmpFlvSession::parseCmdMsg(const char &c) {
    tmp.push_back(c);
}

void RtmpFlvSession::ack() {
    received += head.length;
    ackReceived += head.length;
    if (received >= 0xf0000000) {
        received = 0;
    }
    if (ackReceived >= remoteWindowAckSize) {
        ChunkStreamHead head;
        head.fmt = 0;
        head.csId = 2;
        head.typeId = 3;
        head.length = 4;
        auto msg = ObjPool::allocate<vector<unsigned char>>();
        msg->push_back(chunkSize >> 24);
        msg->push_back((chunkSize & 0x00ffffff) >> 16);
        msg->push_back((chunkSize & 0x0000ffff) >> 8);
        msg->push_back(chunkSize & 0x000000ff);
        auto chunkMsg = head.writeChunk(msg, chunkSize);
        write(chunkMsg);
        ackReceived = 0;
    }
}

void RtmpFlvSession::parseControlMsg(const char &c) {
    if (head.typeId == 1) {
        if (size == 1) {
            remoteChunkSize = 0;
        }
        remoteChunkSize = (remoteChunkSize << 8) | (unsigned char)c;
    } else if (head.typeId == 5) {
        if (size == 1) {
            remoteWindowAckSize = 0;
        }
        remoteWindowAckSize = (remoteWindowAckSize << 8) | (unsigned char)c;
    }
}
