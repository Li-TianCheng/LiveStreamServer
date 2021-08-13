//
// Created by ltc on 2021/8/12.
//

#ifndef LIVESTREAMSERVER_RTMPRELAYSESSION_H
#define LIVESTREAMSERVER_RTMPRELAYSESSION_H

#include "RtmpSession.h"

class RtmpRelaySession : public RtmpSession {
public:
    RtmpRelaySession(bool isRelaySource, const string& address, const string& vhost, const string& app, const string& streamName, int bufferChunkSize);
    void sessionInit() override;
private:
    void shakeHand(const char& c) override;
    void connect();
    void createStream(double num);
    void setChunkSize();
    void FCPublish(double num);
    void releaseStream(double num);
    void play();
    void publish();
    void parseCmdMsg() override;
private:
    bool isRelaySource;
    string address;
};


#endif //LIVESTREAMSERVER_RTMPRELAYSESSION_H
