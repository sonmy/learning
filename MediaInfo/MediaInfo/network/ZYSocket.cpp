//
//  ZYSocket.cpp
//  MediaInfo
//
//  Created by mysong on 2023/2/15.
//

#include "ZYSocket.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int ZYSocket::SendData(void *data, size_t size) {
    return 0;
}

int ZYSocket::ConnetToIp(std::string ip, uint16_t port) {
    
    struct in6_addr s6;
    int ret6 = inet_pton(AF_INET6, ip.c_str(), (void *)&s6);
    if (ret6 > 0) {
        
    }
    
    
    
    this->socket_t = socket(AF_INET, SOCK_STREAM, 0);
    
    //向服务器（特定的IP和端口）发起请求
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  //具体的IP地址
    serv_addr.sin_port = htons(1234);  //端口
    //connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    return 0;
}
