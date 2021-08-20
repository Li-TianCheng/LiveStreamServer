//
// Created by ltc on 2021/7/21.
//

#ifndef LIVESTREAMSERVER_LIVESTREAMSERVER_H
#define LIVESTREAMSERVER_LIVESTREAMSERVER_H

#include "net/include/Listener.h"
#include "HttpFlvSession.h"
#include "RtmpSession.h"
#include "HttpFlvRelaySession.h"
#include "RtmpRelaySession.h"

class LiveStreamServer {
public:
    static void serve();
    static void httpFlvRelay(bool isRelaySource, const string& address, const string& vhost, const string& app, const string& streamName);
    static void rtmpRelay(bool isRelaySource, const string& address, const string& vhost, const string& app, const string& streamName);
    static void close();
    static void addListener(int port, shared_ptr<TcpServerBase> server);
private:
    static LiveStreamServer& getServer();
    void addNewSession(const string& address, shared_ptr<SessionBase> session);
    LiveStreamServer();
private:
    int chunkSize;
    shared_ptr<Listener> listener;
    shared_ptr<TcpServer<HttpFlvSession>> httpFlvServer;
    shared_ptr<TcpServer<RtmpSession>> rtmpServer;
    HttpServer apiServer;
    HttpServer httpServer;
};

#endif //LIVESTREAMSERVER_LIVESTREAMSERVER_H
