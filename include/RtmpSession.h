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
protected:
    void handleReadDone(iter pos, size_t n) override;
    virtual void shakeHand(iter& pos);
    void parseHead(iter& pos);
    virtual void parseCmdMsg();
    void parseAMF0(vector<string>& type, vector<string>& value);
    void writeAMF0Num(shared_ptr<vector<unsigned char>> chunk, double num);
    void writeAMF0String(shared_ptr<vector<unsigned char>> chunk, const string& str);
    void writeAMF0Key(shared_ptr<vector<unsigned char>> chunk, const string& str);
    void parseAMF3(vector<string>& type, vector<string>& value);
    void responseConnect(vector<string> &type, vector<string> &value);
    void responseCreateStream(vector<string> &type, vector<string> &value);
    void responsePublish(vector<string> &type, vector<string> &value);
    void responsePlay(vector<string> &type, vector<string> &value);
    void ack();
    void parseControlMsg();
    void writeChunk(ChunkStreamHead& head, shared_ptr<vector<unsigned char>> msg);
protected:
    int size;
    int status;
    int chunkStatus;
    size_t idx;
    size_t msgLength;
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
