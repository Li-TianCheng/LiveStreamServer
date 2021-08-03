//
// Created by ltc on 2021/7/31.
//

#ifndef LIVESTREAMSERVER_HTTPAPI_H
#define LIVESTREAMSERVER_HTTPAPI_H

#include <jsoncpp/json/json.h>

void getStreamInfo(shared_ptr<Http> request, shared_ptr<Http> response) {
    Json::Value status;
    status["sink num"] = StreamCenter::getSinkNum();
    status["source num"] = StreamCenter::getSourceNum();
    vector<char> data;
    string s = status.toStyledString();
    data.insert(data.end(), s.begin(), s.end());
    response->data = data;
}

void apiServerInit(HttpServer& apiServer) {
    apiServer.registerHandler("/v1/stream/status", getStreamInfo);
}

#endif //LIVESTREAMSERVER_HTTPAPI_H