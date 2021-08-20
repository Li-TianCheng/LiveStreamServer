//
// Created by ltc on 2021/8/12.
//

#ifndef LIVESTREAMSERVER_HTTPFLVRELAYSESSION_H
#define LIVESTREAMSERVER_HTTPFLVRELAYSESSION_H

#include "HttpFlvSession.h"
#include "http/include/HttpSession.h"

class HttpFlvRelaySession: public HttpFlvSession {
public:
    HttpFlvRelaySession(bool isRelaySource, const string& vhost, const string& app, const string& streamName, int bufferChunkSize);
    void sessionInit() override;
    void handleReadDone(iter pos, size_t n) override;
private:
    bool isRelaySource;
};


#endif //LIVESTREAMSERVER_HTTPFLVRELAYSESSION_H
