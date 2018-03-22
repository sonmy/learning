//
//  server.h
//  http-server
//
//  Created by leonllhuang on 15/4/29.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __local_server__server__
#define __local_server__server__

#include <stdio.h>
#include <pthread.h>
#include "mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    
    /// first filed muse be 'mg_handler_t type'
    struct ls_handler {
        mg_event_handler_t handler; /// user-handler function address, same size as void*
        char data[0];
    };
    
    struct ls_counter {
        unsigned int next, speed;
        int sum, cnt[5];
    };
    
    struct ls_server {
        volatile short is_stop;
        volatile short is_running;
        unsigned short port;
        int select_time_out;
        struct mg_mgr mgr;
        struct mg_connection *conn;
        
        int time_now;
        unsigned int req_acc;
        struct ls_counter request_counter;
        
        pthread_t thread;
//        pthread_mutex_t mutex;
        
    };
    
    /// begin: LocalServer function
    struct ls_server * ls_create_server(unsigned short port, int select_time_out_ms);
    void ls_destroy_server(struct ls_server **server);
    int ls_start_server(struct ls_server *server);
    void* ls_run_server(struct ls_server *server);
    int ls_stop_server(struct ls_server *server);
    /// end: LocalServer function
    
    /// begin: utility tools
    int parse_range(const char *range, int64_t *begin, int64_t *end);
    int is_keep_alive(struct http_message *hm);
    char * mg_copy_mg_str(char* dst, size_t dst_len, const struct mg_str * str);
    /// end: utility tools
    
    /// begin localserve config
    int ls_get_request_speed(struct ls_server *server);
    
    /// end localser config
#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* defined(__local_server__server__) */
