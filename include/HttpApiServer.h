//
// Created by ltc on 2021/7/31.
//

#ifndef LIVESTREAMSERVER_HTTPAPISERVER_H
#define LIVESTREAMSERVER_HTTPAPISERVER_H

#include <jsoncpp/json/json.h>
#include "LiveStreamServer.h"

static void getStreamInfo(shared_ptr<Http> request, shared_ptr<Http> response) {
    Json::Value status;
    status["sink num"] = StreamCenter::getSinkNum();
    status["source num"] = StreamCenter::getSourceNum();
    vector<char> data;
    string s = status.toStyledString();
    data.insert(data.end(), s.begin(), s.end());
    response->data = data;
}

static void restart(shared_ptr<Http> request, shared_ptr<Http> response) {
    pid_t pid = getpid();
    string cmd = "ps -efc | grep " + std::to_string(pid);
    FILE* pp = ::popen(cmd.data(),"r");
    char buff[512];
    memset(buff,0,sizeof(buff));
    fgets(buff,sizeof(buff),pp);
    vector<string> split = utils::split(buff, ' ');
    pclose(pp);
    pid_t p = fork();
    if (p == 0) {
        sleep(1);
        execl(split[8].data(), split[8].data(), "-c", ConfigSystem::path, nullptr);
        abort();
    } else {
        abort();
    }
}

static void getStreamName(shared_ptr<Http> request, shared_ptr<Http> response) {
    Json::Value status = StreamCenter::getStreamName();
    vector<char> data;
    string s = status.toStyledString();
    data.insert(data.end(), s.begin(), s.end());
    response->data = data;
}

static void stop(shared_ptr<Http> request, shared_ptr<Http> response) {
    abort();
}

static void getStatus(shared_ptr<Http> request, shared_ptr<Http> response) {
    pid_t pid = getpid();
    string filePath = "/proc/" + std::to_string(pid) + "/stat";
    char lastBuffer[1024];
    char buffer[1024];
    memset(lastBuffer, 0, sizeof(lastBuffer));
    memset(buffer, 0, sizeof(buffer));
    FILE *file = fopen(filePath.data(), "r");
    fgets(lastBuffer, sizeof(lastBuffer), file);
    fclose(file);
    sleep(1);
    file = fopen(filePath.data(), "r");
    fgets(buffer, sizeof(buffer), file);
    fclose(file);
    vector<string> split = utils::split(lastBuffer, ' ');
    long _userCpu = stol(split[13]);
    long _kernelCpu = stol(split[14]);
    split = utils::split(buffer, ' ');
    long userCpu = stol(split[13]);
    long kernelCpu = stol(split[14]);
    int threadNum = stoi(split[19]);
    long virtualMem = stol(split[22]);
    long realMem = stol(split[23]);
    long user = (userCpu - _userCpu)*100 / sysconf(_SC_CLK_TCK);
    long kernel = (kernelCpu - _kernelCpu)*100 / sysconf(_SC_CLK_TCK);
    Json::Value status;
    status["cpu"]["num"] =  (int)sysconf( _SC_NPROCESSORS_CONF);
    status["cpu"]["user"] = std::to_string(user) + "%";
    status["cpu"]["system"] = std::to_string(kernel) + "%";
    status["cpu"]["total"] = std::to_string(user+kernel) + "%";
    status["thread num"] = threadNum;
    status["memory"]["virtual"] = std::to_string((double)virtualMem/1024/1024) + " MiB";
    status["memory"]["physical"] = std::to_string(realMem*getpagesize()/1024) + " KiB";
    vector<char> data;
    string s = status.toStyledString();
    data.insert(data.end(), s.begin(), s.end());
    response->data = data;
}

static void relay(shared_ptr<Http> request, shared_ptr<Http> response) {
    vector<string> split = utils::split(request->line["url"], '/', 3);
    split = utils::split(split[2], '?', 2);
    if (split.size() == 2) {
        if (split[0] == "relay_source") {
            split = utils::split(split[1], '/', 5);
            if (split.size() == 5) {
                if (split[0] == "rtmp:") {
                    LiveStreamServer::rtmpRelay(true, split[1], split[2], split[3], split[4]);
                }
                if (split[0] == "http:") {
                    LiveStreamServer::httpFlvRelay(true, split[1], split[2], split[3], split[4]);
                }
            }
        }
        if (split[0] == "relay_sink") {
            split = utils::split(split[1], '/', 5);
            if (split.size() == 5) {
                if (split[0] == "rtmp:") {
                    LiveStreamServer::rtmpRelay(false, split[1], split[2], split[3], split[4]);
                }
                if (split[0] == "http:") {
                    LiveStreamServer::httpFlvRelay(false, split[1], split[2], split[3], split[4]);
                }
            }
        }
    }
}

static void apiServerInit(HttpServer& apiServer) {
    apiServer.registerHandler("/v1/stream/status", getStreamInfo);
    apiServer.registerHandler("/v1/stream/name", getStreamName);
    apiServer.registerRegexHandler("/v1/stream/relay_.*", relay);
    apiServer.registerHandler("/v1/server/restart", restart);
    apiServer.registerHandler("/v1/server/stop", stop);
    apiServer.registerHandler("/v1/server/status", getStatus);
}

#endif //LIVESTREAMSERVER_HTTPAPISERVER_H