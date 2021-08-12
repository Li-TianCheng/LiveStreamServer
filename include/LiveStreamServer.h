//
// Created by ltc on 2021/7/21.
//

#ifndef LIVESTREAMSERVER_LIVESTREAMSERVER_H
#define LIVESTREAMSERVER_LIVESTREAMSERVER_H

#include "net/include/Listener.h"
#include "HttpFlvSession.h"
#include "RtmpSession.h"
#include "HttpApi.h"

class LiveStreamServer {
public:
    LiveStreamServer();
    void serve();
private:
    shared_ptr<Listener> listener;
    shared_ptr<TcpServer<HttpFlvSession>> httpFlvServer;
    shared_ptr<TcpServer<RtmpSession>> rtmpServer;
    HttpServer apiServer;
    HttpServer httpServer;
};

static void getResource(shared_ptr<Http> request, shared_ptr<Http> response) {
    string path = ".." + request->line["url"];
    std::ifstream file(path);
    file.seekg(0, file.end);
    int length = file.tellg();
    if (length > 0) {
        file.seekg(0, file.beg);
        response->data.resize(length);
        file.read(response->data.data(), length);
    }
    file.close();
}

static void play(shared_ptr<Http> request, shared_ptr<Http> response) {
    vector<string> split = utils::split(request->line["url"], '?', 2);
    if (split.size() < 2) {
        response->line["status"] = "400";
        response->line["msg"] = "Bad Request";
        return;
    }
    split = utils::split(split[1], '&');
    unordered_map<string, string> map;
    for (auto& p : split) {
        vector<string> s = utils::split(p, '=', 2);
        if (s.size() == 2) {
            map[s[0]] = s[1];
        }
    }
    string path = "../resource/play.html";
    std::ifstream file(path);
    string s;
    int line = 0;
    while (getline(file, s)) {
        line++;
        if (line == 36) {
            vector<string> split = utils::split(request->head["Host"], ':');
            s = "url: 'http://" + split[0] + ":8070/" + map["vhost"] + "/" + map["app"] + "/" + map["stream"] + "'";
        }
        response->data.insert(response->data.end(), s.begin(), s.end());
    }
    file.close();
}

static void frontPage(shared_ptr<Http> request, shared_ptr<Http> response) {
    string path = "../resource/index.html";
    std::ifstream file(path);
    string s;
    int line = 0;
    while (getline(file, s)) {
        line++;
        if (line == 25) {
            s = "\"" + request->head["Host"] + "\"";
        }
        response->data.insert(response->data.end(), s.begin(), s.end());
    }
    file.close();
}


#endif //LIVESTREAMSERVER_LIVESTREAMSERVER_H
