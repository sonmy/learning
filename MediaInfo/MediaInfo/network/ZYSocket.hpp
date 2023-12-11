//
//  ZYSocket.hpp
//  MediaInfo
//
//  Created by mysong on 2023/2/15.
//

#ifndef ZYSocket_hpp
#define ZYSocket_hpp

#include <stdio.h>

#include <string>
#include <memory>


class ZYSocket;

class ZYSocketEventDelegate {
public:
    virtual void OnConnected(ZYSocket *socket);
    virtual void OnConnectFailed(ZYSocket *socket, int errNo);
    virtual void OnReceivedData(void *buf, size_t size);
    virtual void OnDisConnect(int errNo);
};

class ZYSocket {
    std::weak_ptr<ZYSocketEventDelegate> delegate;
    int SendData(void *data, size_t size);
    int ConnetToIp(std::string ip, uint16_t port);
private:
    int socket_t;
};
#endif /* ZYSocket_hpp */
