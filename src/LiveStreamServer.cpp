//
// Created by ltc on 2021/7/21.
//

#include "LiveStreamServer.h"
#include "HttpApiServer.h"
#include "HttpServer.h"

LiveStreamServer::LiveStreamServer(): listener(ObjPool::allocate<Listener>()), httpFlvServer(ObjPool::allocate<TcpServer<HttpFlvSession>>()), chunkSize(ConfigSystem::getConfig()["system"]["net"]["read_buffer_chunk_size"].asInt()),
                  rtmpServer(ObjPool::allocate<TcpServer<RtmpSession>>()), apiServer(listener, ConfigSystem::getConfig()["live_stream_server"]["api_server_port"].asInt(), IPV4),
                  httpServer(listener, ConfigSystem::getConfig()["live_stream_server"]["http_server_port"].asInt(), IPV4) {
    listener->registerListener(ConfigSystem::getConfig()["live_stream_server"]["http_flv_server_port"].asInt(), IPV4, httpFlvServer);
    listener->registerListener(ConfigSystem::getConfig()["live_stream_server"]["rtmp_server_port"].asInt(), IPV4, rtmpServer);
    apiServerInit(apiServer);
    httpServerInit(httpServer);
}

void LiveStreamServer::serve() {
    getServer().listener->listen();
}

LiveStreamServer &LiveStreamServer::getServer() {
    static LiveStreamServer server;
    return server;
}

void LiveStreamServer::addNewSession(const string &address, shared_ptr<SessionBase> session) {
    auto arg = ObjPool::allocate<addNewSessionArg>();
    arg->listener = listener;
    arg->session = session;
    arg->address = address;
    arg->addressType = IPV4;
    auto e = ObjPool::allocate<Event>(EventAddSession, arg);
    listener->receiveEvent(e);
}

void LiveStreamServer::httpFlvRelay(bool isRelaySource, const string &address, const string &vhost, const string &app,
                                    const string &streamName) {
    auto session = ObjPool::allocate<HttpFlvRelaySession>(isRelaySource, vhost, app, streamName, getServer().chunkSize);
    getServer().addNewSession(address, session);
}

void LiveStreamServer::rtmpRelay(bool isRelaySource, const string &address, const string &vhost, const string &app,
                                 const string &streamName) {
    auto session = ObjPool::allocate<RtmpRelaySession>(isRelaySource, address, vhost, app, streamName, getServer().chunkSize);
    getServer().addNewSession(address, session);
}

void LiveStreamServer::close() {
    getServer().httpFlvServer->close();
    getServer().rtmpServer->close();
    getServer().apiServer.close();
    getServer().httpServer.close();
}

void LiveStreamServer::addListener(int port, shared_ptr<TcpServerBase> server) {
    auto arg = ObjPool::allocate<addListenerArg>();
    arg->listener = getServer().listener;
    arg->port = port;
    arg->addressType = IPV4;
    arg->server = server;
    auto e = ObjPool::allocate<Event>(EventAddListener, arg);
    getServer().listener->receiveEvent(e);
}
