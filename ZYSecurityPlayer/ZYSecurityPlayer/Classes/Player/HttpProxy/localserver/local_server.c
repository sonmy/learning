//
//  server.c
//  http-server
//
//  Created by leonllhuang on 15/4/29.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "local_server.h"
//#include "handler/handlers.h"
//#include "handler/mp4_handler.h"

//#include "LogHelper.h"

static void ls_counter_init(struct ls_counter *counter) {
    if (counter == NULL)
        return;
    counter->next = counter->speed = 0;
    counter->sum = 0;
    int i = 0;
    for (i = 0; i < sizeof(counter->cnt) / sizeof(counter->cnt[0]); i++) {
        counter->cnt[i] = -1;
    }
}

static void ls_counter_push(struct ls_counter *counter, int val) {
    unsigned int next = counter->next;
    unsigned int total = (sizeof(counter->cnt) / sizeof(counter->cnt[0]));
    if (counter->cnt[next] == -1) {
        counter->cnt[next] = val;
        counter->sum += val;
        counter->speed = counter->sum / (next + 1) + (counter->sum % (next + 1) > 0 ? 1 : 0);
        counter->next = (next + 1) % total;
    } else {
        counter->sum -= counter->cnt[next];
        counter->cnt[next] = val;
        counter->sum += val;
        counter->speed = counter->sum / total + (counter->sum % total > 0 ? 1 : 0);
        counter->next = (next + 1) % total;
    }
}

char * mg_copy_mg_str(char* dst, size_t dst_len, const struct mg_str * str)
{
    if (dst_len == 1) {
        dst[0] = '\0';
        return dst;
    } else if (dst_len > 0) {
        dst_len--;
        if (dst_len > str->len)
        dst_len = str->len;
        memcpy(dst, str->p, dst_len);
        dst[dst_len] = '\0';
        return dst;
    } else {
        return dst;
    }
}

int parse_range(const char *range, int64_t *begin, int64_t *end) {
    *begin = -1;
    *end = -1;
    if (range != NULL) {
        int ret = sscanf(range, "bytes=%" PRId64 "-%" PRId64, begin, end);
        if (ret == 1) *end = -1;
        return ret;
    }
    return 0;
}

int is_keep_alive(struct http_message *hm) {
    const struct mg_str *connection_header = mg_get_http_header(hm, "Connection");
    if (connection_header == NULL) {
        /* HTTP/1.1 connections are keep-alive by default. */
        if (mg_vcasecmp(&hm->proto, "HTTP/1.1") != 0) return 0;
    } else if (mg_vcasecmp(connection_header, "keep-alive") != 0) {
        return 0;
    }
    // We must also have Content-Length.
    //    return mg_get_http_header(hm, "Content-Length") != NULL;
    return 1;
}

//static void print_mg_connection(struct mg_connection *c) {
//    printf("method:%s uri:%s http_version:%s query:%s\nstatus_code:%d\n", c->request_method, c->uri, c->http_version, c->query_string, c->status_code);
//    int i;
//    for (i = 0; i < c->num_headers; i++) {
//        if (c->http_headers[i].name == NULL) {
//            break;
//        }
//        printf("%s: %s\n", c->http_headers[i].name, c->http_headers[i].value);
//    }
//}

static void ls_run_handler(struct mg_connection *nc, int ev, void *p) {
    struct ls_server *ls = (struct ls_server*)(nc->mgr->user_data);
    mg_event_handler_t handler = NULL;
    
    if (nc->user_data != NULL
        && NULL != (handler = ((struct ls_handler*)nc->user_data)->handler)) {
        handler(nc, ev, p);
        return;
    }
    
    switch (ev) {
        case MG_EV_HTTP_REQUEST :
        {
            ls->req_acc++;
            struct http_message* hm = (struct http_message*)p;
#ifdef USE_FIND_HANLDER
            if (NULL != (handler = ls_find_handler(hm))) {
                handler(nc, ev, p);
                return;
            }
#else
            if (0 == mg_vcasecmp(&(hm->uri), "/playmp4")) {
                //ls_mp4_handler(nc, ev, p);
                printf("%s", "playmp4");
                return;
            }
#endif
            char uri[128];
            mg_copy_mg_str(uri, sizeof(uri), &hm->uri);
            printf("handler not found! %s\n", uri);
            nc->flags |= MG_F_CLOSE_IMMEDIATELY;
        }
            break;
        
        case MG_EV_ACCEPT :
        {
            // 需要自己保存 IP/Port
            union socket_address *psa = (union socket_address*)p;
            nc->sa = *psa;
            //printf("MG_EV_ACCEPT nc:%p ev:%d sock:%d remote %s:%d\n", nc, ev, nc->sock, inet_ntoa(psa->sin.sin_addr), (int)ntohs(psa->sin.sin_port));
        }
            break;
        case MG_EV_POLL :
        case MG_EV_SEND :
        case MG_EV_RECV :
            break;
        case MG_EV_CLOSE :
        {
            struct sockaddr_in sin;
            socklen_t sl = (socklen_t)sizeof(sin);
            getpeername(nc->sock, (struct sockaddr*)&sin, &sl);
            //printf("MG_EV_CLOSE %p sock:%d remote %s:%d\n", nc, nc->sock, inet_ntoa(sin.sin_addr), (int)ntohs(sin.sin_port));
        }
            break;
        default:
            nc->flags |= MG_F_CLOSE_IMMEDIATELY;
            break;
    }
}

/*
static void ls_update(struct ls_server *server, time_t current_time) {
    static time_t last_time = 0;
    if (last_time != 0 && (current_time - last_time) < 10) {
        return;
    }
    last_time = current_time;
    struct mg_connection *c;
    //    char buf[20];
    //    int len = sprintf(buf, "%lu", (unsigned long) current_time);
    int total_conn = 0;
    size_t mem = 0;
    char buf[128] = {0};
    // Iterate over all connections, and push current time message to websocket ones.
    for (c = mg_next(&server->mgr, NULL); c != NULL; c = mg_next(&server->mgr, c)) {
        buf[0] = 0;
        mg_sock_to_str(c->sock, buf, sizeof(buf) - 1, MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT | MG_SOCK_STRINGIFY_REMOTE);
        printf("%7d: ls_update conn:%p sock:%d remote:%s %s\n", (int)time(NULL), c, c->sock, buf, c->listener ? "client" : "listen");
        total_conn++;
        mem += c->send_mbuf.size + c->recv_mbuf.size;
    }
    printf("total_conn %d mem:%" PRIdPTR "KB\n", total_conn, mem/1024);
}
*/

static inline void *ls_thread_serve(void *server) {
    return ls_run_server((struct ls_server*)server);
}

struct ls_server * ls_create_server(unsigned short port, int select_time_out_ms) {
    struct ls_server * ls = malloc(sizeof(struct ls_server));
    memset(ls, 0, sizeof(struct ls_server));
    char buf[64];
    snprintf(buf, sizeof(buf), "%u", (unsigned int)port);
    
    ls->port = port;
    ls->is_stop = 0;
    ls->is_running = 0;
    ls->select_time_out = select_time_out_ms; // ms
    ls->thread = 0;
//    pthread_mutex_init(&ls->mutex, NULL);
    ls_counter_init(&ls->request_counter);
    
    mg_mgr_init(&ls->mgr, (void*)ls);
    
    ls->conn = mg_bind(&ls->mgr, buf/*port*/, ls_run_handler);
    if (ls->conn) {
        // Set up HTTP server parameters
        mg_set_protocol_http_websocket(ls->conn);
    } else {
        ls_destroy_server(&ls);
    }
    return ls;
}

void ls_destroy_server(struct ls_server **server) {
    if (server == NULL || *server == NULL) return;
    (*server)->is_stop = 1;
//    pthread_mutex_destroy(&(*server)->mutex);
    mg_mgr_free(&(*server)->mgr);
    free(*server);
    *server = NULL;
}

int ls_start_server(struct ls_server *server) {
    
    if (server == NULL) {
        return -1;
    }
    
    pthread_t thread_id = (pthread_t) 0;
    pthread_attr_t attr;
    
    (void) pthread_attr_init(&attr);
//    (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
#if defined(NS_STACK_SIZE) && NS_STACK_SIZE > 1
    (void) pthread_attr_setstacksize(&attr, NS_STACK_SIZE);
#endif
    
    int ret = pthread_create(&thread_id, &attr, ls_thread_serve, server);
    pthread_attr_destroy(&attr);
    
    server->thread = thread_id;
    if (ret != 0)  {
        thread_id = 0;
        return -1;
    }
    return 0;
}

void* ls_run_server(struct ls_server *server) {
    //置server running标识
    server->is_running = 1;
    
    for ( ; !server->is_stop; ) {
//        pthread_mutex_lock(&server->mutex);
        mg_mgr_poll(&server->mgr, server->select_time_out);
        
        //如果检测到硬错误，那么置停止标识
        if(server->mgr.hard_error_flag == 1)
        {
            TVHttpProxyLOG(LOG_ERROR, "mg_mgr_poll occur hard error, just stop local server:%d", 1);
            server->is_stop = 1;
            break;
        }
        
//        ls_update(server, time(NULL));
        
        int now = (int)time(NULL);
        if (server->time_now == 0) {
            server->time_now = now;
        }
        int diff = now - server->time_now;
        if (diff > 0) {
            ls_counter_push(&server->request_counter, server->req_acc / diff + (server->req_acc % diff > 0 ? 1 : 0));
            server->req_acc = 0;
            server->time_now = now;
//            printf("request speed %d\n", ls_get_request_speed(server));
        }
//        pthread_mutex_unlock(&server->mutex);
    }
    
    //重置server running标识
    server->is_running = 0;
    
    return NULL;
}

int ls_stop_server(struct ls_server *server) {
    if (server == NULL) return -1;
    server->is_stop = 1;
    
//    mg_wakeup_server(server->mg_server); // 不要wakeup server，因为这里采用数据包形式发送，所以无法保证时序和server能够处理完wakeup消息，所以采用锁进行强制同步
    
//    // must lock, it needs wait for thread exit // 必须锁住，目的是等待server线程退出
//    pthread_mutex_lock(&server->mutex);
//    pthread_mutex_unlock(&server->mutex);
    
    //等待ls_run_server thread退出
    pthread_join(server->thread, NULL);
    
    //等待线程退出，最多等待500ms(主要是应对join失败的极端情况)
    int32_t          waitCounter = 0;

    while(server->is_running && (++waitCounter < 50))
    {
        //每次等待10ms
        usleep(10 * 1000);
    }


    return 0;
}

int ls_get_request_speed(struct ls_server *server)
{
    if (server == NULL)
        return 0;
    return server->request_counter.speed;
}


