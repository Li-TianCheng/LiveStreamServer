//
// Created by ltc on 2021/7/21.
//

#ifndef LIVESTREAMSERVER_RTMPFLVSESSION_H
#define LIVESTREAMSERVER_RTMPFLVSESSION_H

#include "FlvSessionBase.h"

struct ChunkStreamHead {
    unsigned int fmt = 0;
    unsigned int csId = 0;
    unsigned int basicHeaderSize = 0;
    unsigned int msgHeaderSize = 0;
    unsigned int time = 0;
    unsigned int typeId = 0;
    unsigned int length = 0;
    unsigned int idx = 0;
    bool extend = false;
    shared_ptr<vector<unsigned char>> writeChunk(shared_ptr<vector<unsigned char>> msg, unsigned int chunkSize) {
        shared_ptr<vector<unsigned char>> chunk;
        length = msg->size();
        if (typeId == 8) {
            csId = 4;
        } else if (typeId == 9 || typeId == 18 || typeId == 0xf) {
            csId = 6;
        }
        unsigned int num = length / chunkSize;
        for (int i = 0; i < num; i++) {
            if (i == 0) {
                chunk->push_back(csId);
                chunk->push_back(time >> 16);
                chunk->push_back((time & 0x0000ffff) >> 8);
                chunk->push_back(time & 0x000000ff);
                chunk->push_back(length >> 16);
                chunk->push_back((length & 0x0000ffff) >> 8);
                chunk->push_back(length & 0x000000ff);
                chunk->push_back(typeId);
                chunk->push_back(0);
                chunk->push_back(0);
                chunk->push_back(0);
                chunk->push_back(0);
            } else {
                chunk->push_back(csId | 0xc0);
            }
            chunk->insert(chunk->end(), msg->begin()+i*chunkSize, msg->begin()+(i+1)*chunkSize);
        }
        return chunk;
    }
};

class RtmpFlvSession: public FlvSessionBase {
public:
    RtmpFlvSession();
    ~RtmpFlvSession() override = default;
    void writeFlv(shared_ptr<vector<unsigned char>> tag) override;
    void writeFlvHead() override;
private:
    void handleReadDone(iter pos, size_t n) override;
    void shakeHand(const char& c);
    void parseHead(const char& c);
    void parseCmdMsg(const char& c);
    void ack();
    void parseControlMsg(const char& c);
private:
    int size;
    int status;
    int chunkStatus;
    unsigned int chunkSize;
    unsigned int remoteChunkSize;
    unsigned int windowAckSize;
    unsigned int remoteWindowAckSize;
    unsigned int received;
    unsigned int ackReceived;
    ChunkStreamHead head;
    shared_ptr<vector<unsigned char>> c1;
    vector<unsigned char> tmp;
};


#endif //LIVESTREAMSERVER_RTMPFLVSESSION_H
