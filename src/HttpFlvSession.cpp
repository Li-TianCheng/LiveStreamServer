//
// Created by ltc on 2021/6/28.
//

#include "HttpFlvSession.h"

HttpFlvSession::HttpFlvSession(int bufferChunkSize) : request(nullptr), status(0), chunked(false), chunkSize(0), flvStatus(0), flvSize(0),tmpSize(0),
                                                      SessionBase(false, bufferChunkSize), isPost(false), headSize(13) {

}

void HttpFlvSession::handleReadDone(iter pos, size_t n) {
    SessionBase::handleReadDone(pos, n);
    idx = 0;
    msgLength = n;
    while (idx < n) {
        if (status != 8) {
            parseHttpHead(pos);
            idx++;
        } else {
            if (streamName.empty()) {
                sink();
                return;
            }
            if (isPost) {
                source(pos);
            } else {
                break;
            }
        }
    }
    if (status == 8 && !isPost) {
        sink();
    }
    readDone(n);
}

void HttpFlvSession::parseHttpHead(iter& pos) {
    char c = *(pos+idx);
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
                isPost = request->line["method"] == "POST";
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

void HttpFlvSession::source(iter& pos) {
    if (chunked) {
        if (chunkSize != 0) {
            parseFlv(pos);
        } else {
            char c = *(pos+idx);
            if (c == '\r') {
                idx++;
                return;
            }
            if (c == '\n') {
                chunkSize = tmpSize;
                tmpSize = 0;
                idx++;
                return;
            }
            if (c >= '0' && c <= '9') {
                tmpSize = tmpSize*16 + c - '0';
            } else if (c >= 'A' && c <= 'F') {
                tmpSize = tmpSize*16 + c - 'A' + 10;
            } else {
                tmpSize = tmpSize*16 + c - 'a' + 10;
            }
            idx++;
        }
    } else {
        parseFlv(pos);
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

void HttpFlvSession::parseFlv(iter &pos) {
    if (headSize != 0) {
        size_t remain = msgLength-idx;
        if (chunked) {
            remain = chunkSize < msgLength-idx ? chunkSize : msgLength-idx;
        }
        size_t inc = remain < headSize ? remain : headSize;
        headSize -= inc;
        idx += inc;
        if (chunked) {
            chunkSize -= inc;
        }
    } else {
        switch(flvStatus) {
            case 0: {
                typeId = *(pos+idx);
                idx++;
                if (chunked) {
                    chunkSize--;
                }
                flvStatus = 1;
                size = 0;
                break;
            }
            case 1: {
                unsigned char c = *(pos+idx);
                flvSize = (flvSize << 8) | c;
                idx++;
                size++;
                if (chunked) {
                    chunkSize--;
                }
                if (size == 3) {
                    size = 0;
                    flvStatus = 2;
                }
                break;
            }
            case 2: {
                if (size == 0) {
                    stream.currTag->push_back(typeId);
                    stream.currTag->push_back((flvSize>>16) & 0x0000ff);
                    stream.currTag->push_back((flvSize>>8) & 0x0000ff);
                    stream.currTag->push_back(flvSize & 0x0000ff);
                }
                size_t remain = msgLength-idx;
                if (chunked) {
                    remain = chunkSize < msgLength-idx ? chunkSize : msgLength-idx;
                }
                size_t inc = remain < flvSize-size+11 ? remain : flvSize-size+11;
                copy(pos+idx, inc, *stream.currTag);
                idx += inc;
                size += inc;
                if (chunked) {
                    chunkSize -= inc;
                }
                if (size == flvSize+11) {
                    size = 0;
                    flvSize = 0;
                    flvStatus = 0;
                    parseTag();
                }
                break;
            }
        }
    }
}
