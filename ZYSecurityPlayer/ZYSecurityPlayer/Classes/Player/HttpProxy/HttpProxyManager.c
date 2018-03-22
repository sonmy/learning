//
//  HttpProxyManager.c
//  ZYSecurityPlayer
//
//  Created by mysong on 2017/6/15.
//  Copyright © 2017年 tencent. All rights reserved.
//

#include "HttpProxyManager.h"
#include "local_server.h"

struct ls_server *gLocalServer = NULL;


//生成随机端口
static int32_t __randomPort()
{
    uint32_t i = arc4random();
    return i % 10000 + 10000;
}

int HttpProxyStart()
{
    int port = 0;
    if (gLocalServer == NULL) {
        port = __randomPort();
        gLocalServer = ls_create_server(port, 50);
        
        if (gLocalServer == NULL) {
            printf("HttpProxy init failed\n");
            port = 0;
        } else {
            ls_start_server(gLocalServer);
        }
    }
    
    return port;
}
