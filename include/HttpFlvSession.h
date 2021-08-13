//
// Created by ltc on 2021/6/28.
//

#ifndef LIVESTREAMSERVER_HTTPFLVSESSION_H
#define LIVESTREAMSERVER_HTTPFLVSESSION_H

#include "net/include/TcpSession.h"
#include "http/include/HttpSession.h"
#include "Stream.h"
#include "SessionBase.h"

class HttpFlvSession : public SessionBase {
public:
    explicit HttpFlvSession(int bufferChunkSize);
    ~HttpFlvSession() override = default;
protected:
    void handleReadDone(iter pos, size_t n) override;
    void parseHttpHead(iter& pos);
    void writeHeader(shared_ptr<Http> response);
    void parseStreamInfo(const string& url);
    void sink();
    void source(iter& pos);
    void parseFlv(iter& pos);
protected:
    string key;
    int status;
    int headSize;
    bool chunked;
    bool isPost;
    int flvStatus;
    unsigned int typeId;
    unsigned int flvSize;
    size_t size;
    size_t chunkSize;
    size_t tmpSize;
    size_t msgLength;
    size_t idx;
    shared_ptr<Http> request;
};


#endif //LIVESTREAMSERVER_HTTPFLVSESSION_H
