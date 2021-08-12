//
// Created by ltc on 2021/7/21.
//

#include "LiveStreamServer.h"

LiveStreamServer::LiveStreamServer(): listener(ObjPool::allocate<Listener>()), httpFlvServer(ObjPool::allocate<TcpServer<HttpFlvSession>>()),
                  rtmpServer(ObjPool::allocate<TcpServer<RtmpSession>>()), apiServer(listener, ConfigSystem::getConfig()["live_stream_server"]["api_server_port"].asInt(), IPV4),
                  httpServer(listener, ConfigSystem::getConfig()["live_stream_server"]["http_server_port"].asInt(), IPV4) {
    listener->registerListener(ConfigSystem::getConfig()["live_stream_server"]["http_flv_server_port"].asInt(), IPV4, httpFlvServer);
    listener->registerListener(ConfigSystem::getConfig()["live_stream_server"]["rtmp_server_port"].asInt(), IPV4, rtmpServer);
    apiServerInit(apiServer);
    httpServer.registerHandler("/", frontPage);
    httpServer.registerRegexHandler("/play\?.*", play);
    httpServer.registerRegexHandler("/resource/.*", getResource);
}

void LiveStreamServer::serve() {
    listener->listen();
}
