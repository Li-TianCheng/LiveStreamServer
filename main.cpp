#include <iostream>
#include <string>
#include "Modules.h"
#include "net/include/TcpServer.h"
#include "HttpFlvSession.h"
#include "RtmpSession.h"
#include "HttpApi.h"

using namespace std;

void getResource(shared_ptr<Http> request, shared_ptr<Http> response) {
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

void play(shared_ptr<Http> request, shared_ptr<Http> response) {
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

void frontPage(shared_ptr<Http> request, shared_ptr<Http> response) {
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

int main(int argc, char *argv[]) {
    int opt;
    string path;
    const char *optString = "c:";
    while ((opt = getopt(argc, argv, optString)) != -1) {
        if (opt == 'c') {
            path = optarg;
        }
    }
    modules::init(path);
    auto server = ObjPool::allocate<TcpServer<HttpFlvSession>>();
    auto rtmpServer = ObjPool::allocate<TcpServer<RtmpSession>>();
    auto listener = ObjPool::allocate<Listener>();
    HttpServer apiServer(listener, 7070, IPV4);
    apiServerInit(apiServer);
    HttpServer httpServer(listener, 8080, IPV4);
    httpServer.registerHandler("/", frontPage);
    httpServer.registerRegexHandler("/play\?.*", play);
    httpServer.registerRegexHandler("/resource/.*", getResource);
    listener->registerListener(8070, IPV4, server);
    listener->registerListener(1935, IPV4, rtmpServer);
    listener->listen();
    modules::close();
    return 0;
}
