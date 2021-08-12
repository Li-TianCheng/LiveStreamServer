//
// Created by ltc on 2021/7/21.
//

#ifndef LIVESTREAMSERVER_RTMPSESSION_H
#define LIVESTREAMSERVER_RTMPSESSION_H

#include "SessionBase.h"

struct ChunkStreamHead {
    unsigned int csId = 0;
    unsigned int time = 0;
    unsigned int timeDelta = 0;
    unsigned int streamId = 0;
    unsigned int typeId = 0;
    unsigned int length = 0;
    unsigned int idx = 0;
};

class RtmpSession: public SessionBase {
public:
    explicit RtmpSession(int bufferChunkSize);
    ~RtmpSession() override = default;
private:
    void handleReadDone(iter pos, size_t n) override;
    void shakeHand(const char& c);
    void parseHead(const char& c);
    void parseCmdMsg();
    void parseAMF0(vector<string>& type, vector<string>& value);
    void parseAMF3(vector<string>& type, vector<string>& value);
    void responseConnect(vector<string> &type, vector<string> &value);
    void responseCreateStream(vector<string> &type, vector<string> &value);
    void responsePublish(vector<string> &type, vector<string> &value);
    void responsePlay(vector<string> &type, vector<string> &value);
    void ack();
    void parseControlMsg(const char& c);
    void writeChunk(ChunkStreamHead& head, shared_ptr<vector<unsigned char>> msg);
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
    unsigned int fmt;
    unsigned int csId;
    unsigned int basicHeaderSize;
    unsigned int msgHeaderSize;
    unsigned int time;
    bool extend;
    unordered_map<unsigned int, ChunkStreamHead> chunkMap;
    ChunkStreamHead* head;
    shared_ptr<vector<unsigned char>> c1;
    vector<unsigned char> cmd;
};


#endif //LIVESTREAMSERVER_RTMPSESSION_H
