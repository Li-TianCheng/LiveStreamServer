//
// Created by ltc on 2021/6/28.
//

#ifndef LIVESTREAMSERVER_HTTPFLVSESSION_H
#define LIVESTREAMSERVER_HTTPFLVSESSION_H

#include "net/include/TcpSession.h"
#include "http/include/HttpSession.h"
#include "Stream.h"
#include "FlvSessionBase.h"

class HttpFlvSession : public FlvSessionBase {
public:
    explicit HttpFlvSession(int bufferChunkSize);
    ~HttpFlvSession() override = default;
private:
    void handleReadDone(iter pos, size_t n) override;
    void parseHttpHead(const char &c);
    void writeHeader(shared_ptr<Http> response);
    void parseStreamInfo(const string& url);
    void sink();
    void source(const char& c);
private:
    string key;
    int status;
    int headSize;
    bool chunked;
    bool isPost;
    size_t tmpSize;
    size_t chunkSize;
    shared_ptr<Http> request;
};


#endif //LIVESTREAMSERVER_HTTPFLVSESSION_H
