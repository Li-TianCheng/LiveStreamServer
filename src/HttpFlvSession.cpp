//
// Created by ltc on 2021/6/28.
//

#include "HttpFlvSession.h"

HttpFlvSession::HttpFlvSession() : request(nullptr), status(0), chunked(false), chunkSize(0), tmpSize(0) {

}

void HttpFlvSession::handleReadDone(iter pos, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (status != 8) {
            parseHttpHead(*(pos++));
        } else {
            if (streamName.empty()) {
                sink();
                return;
            }
            if (request->line["method"] == "POST") {
                source(*(pos++));
            }
        }
    }
    if (status == 8 && request->line["method"] == "GET") {
        sink();
    }
    readDone(n);
}

void HttpFlvSession::parseHttpHead(const char &c) {
    switch(status) {
        case 0:{
            request = ObjPool::allocate<Http>();
            status = 1;
        }
        case 1:{
            if (c == ' ') {
                status = 2;
            } else {
                request->line["method"] += c;
            }
            break;
        }
        case 2:{
            if (c == ' ') {
                status = 3;
            } else {
                request->line["url"] += c;
            }
            break;
        }
        case 3:{
            if (c == '\r') {
                status = 4;
            } else {
                request->line["version"] += c;
            }
            break;
        }
        case 4:{
            if (c == '\n') {
                key = "";
                status = 5;
            } else {
                request = nullptr;
                status = 0;
            }
            break;
        }
        case 5:{
            if (c == ':') {
                status = 6;
            } else if (c == '\r') {
                status = 7;
            } else {
                key += c;
            }
            break;
        }
        case 6:{
            if (c == '\r') {
                request->head[key].erase(0, request->head[key].find_first_not_of(' '));
                request->head[key].erase(request->head[key].find_last_not_of(' ')+1);
                status = 4;
            } else {
                request->head[key] += c;
            }
            break;
        }
        case 7:{
            if (c == '\n') {
                if (request->head.find("Transfer-Encoding") != request->head.end() && request->head["Transfer-Encoding"] == "chunked") {
                    chunked = true;
                } else {
                    chunked = false;
                }
                parseStreamInfo(request->line["url"]);
                status = 8;
                break;
            } else {
                request = nullptr;
                status = 0;
            }
            break;
        }
        default: break;
    }
}

void HttpFlvSession::sink() {
    auto response = ObjPool::allocate<Http>();
    response->line["version"] = request->line["version"];
    if (streamName.empty()) {
        response->line["status"] = "400";
        response->line["msg"] = "Bad Request";
        response->head["Content-Length"] = "0";
        writeHeader(response);
        return;
    }
    sourceSession = StreamCenter::getStream(vhost, app, streamName);
    if (sourceSession == nullptr) {
        response->line["status"] = "404";
        response->line["msg"] = "NOT FOUND";
        response->head["Content-Length"] = "0";
        writeHeader(response);
        return;
    }
    response->line["status"] = "200";
    response->line["msg"] = "OK";
    response->head["Access-Control-Allow-Origin"] = "*";
    writeHeader(response);
    addSink();
}

void HttpFlvSession::source(const char &c) {
    if (chunked) {
        if (chunkSize != 0) {
            parseFlv(c);
            chunkSize--;
        } else {
            if (c == '\r') {
                return;
            }
            if (c == '\n') {
                chunkSize = tmpSize;
                tmpSize = 0;
                return;
            }
            if (c >= '0' && c <= '9') {
                tmpSize = tmpSize*16 + c - '0';
            } else if (c >= 'A' && c <= 'F') {
                tmpSize = tmpSize*16 + c - 'A' + 10;
            } else {
                tmpSize = tmpSize*16 + c - 'a' + 10;
            }
        }
    } else {
        parseFlv(c);
    }
}

void HttpFlvSession::parseStreamInfo(const string &url) {
    vector<string> split = utils::split(url, '/', 3);
    if (split.size() < 3) {
        return;
    }
    vhost = split[0];
    app = split[1];
    streamName = split[2];
}

void HttpFlvSession::writeHeader(shared_ptr<Http> response) {
    string msg;
    msg += std::move(response->line["version"])+" ";
    msg += std::move(response->line["status"])+" ";
    msg += std::move(response->line["msg"])+"\r\n";
    for (auto& h : response->head) {
        msg += h.first+":"+h.second+"\r\n";
    }
    msg += "\r\n";
    auto m = ObjPool::allocate<string>(std::move(msg));
    write(m);
}

void HttpFlvSession::writeFlv(shared_ptr<vector<unsigned char>> tag) {
    write(tag);
}

void HttpFlvSession::writeFlvHead() {
    write(sourceSession->stream.head);
    write(sourceSession->stream.script);
    if (sourceSession->isAAC) {
        write(sourceSession->stream.aacTag);
    }
    if (sourceSession->isAVC) {
        write(sourceSession->stream.avcTag);
    }
    if (sourceSession->isVideo) {
        for (int i = 0; i < (sourceSession->stream.idx-sourceSession->frameNum); i++) {
            writeFlv(sourceSession->stream.gop[i]);
        }
    }
}
