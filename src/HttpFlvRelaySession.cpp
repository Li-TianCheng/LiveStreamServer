//
// Created by ltc on 2021/8/12.
//

#include "HttpFlvRelaySession.h"

HttpFlvRelaySession::HttpFlvRelaySession(bool isRelaySource, const string& vhost, const string& app, const string& streamName, int bufferChunkSize) :
                        isRelaySource(isRelaySource), HttpFlvSession(bufferChunkSize) {
    this->vhost = vhost;
    this->app = app;
    this->streamName = streamName;
}

void HttpFlvRelaySession::sessionInit() {
    SessionBase::sessionInit();
    if (isRelaySource) {
        shared_ptr<string> head = ObjPool::allocate<string>();
        *head = "GET /"+vhost+"/"+app+"/"+streamName+" HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
        write(head);
    } else {
        shared_ptr<string> head = ObjPool::allocate<string>();
        *head = "POST /"+vhost+"/"+app+"/"+streamName+" HTTP/1.1\r\n\r\n\r\n";
        write(head);
    }
}

void HttpFlvRelaySession::handleReadDone(iter pos, size_t n) {
    SessionBase::handleReadDone(pos, n);
    idx = 0;
    msgLength = n;
    while (idx < n) {
        if (status != 8) {
            parseHttpHead(pos);
            idx++;
        } else {
            if (request->line["url"] == "200") {
                if (isRelaySource) {
                    source(pos);
                } else {
                    sourceSession = StreamCenter::getStream(vhost, app, streamName);
                    addSink();
                }
            } else {
                return;
            }
        }
    }
    readDone(n);
}
