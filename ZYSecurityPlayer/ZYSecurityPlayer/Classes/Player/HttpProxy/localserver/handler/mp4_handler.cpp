//
//  mp4_handler.c
//  http-server
//
//  Created by leonllhuang on 15/5/1.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifndef PRId64
#define PRId64 "lld"
#endif

#include "mp4_handler.h"
#include "../local_server.h"

#include "server_cpp_api.h"

#include "LogHelper.h"
#include "GlobalError.h"

//#define LS_SEND_BUF_LEN (2*1024*1024)
//#define SEND_LEN LS_SEND_BUF_LEN

struct session_t {
    int32_t data_id;
    int32_t clip_id;
    int32_t fileID; /// download_manager fileID
    int64_t cur, begin, end; // [begin, end]
    int64_t filesize;
    int64_t content_len;
    int64_t total_sent;
};

extern "C"
struct ls_mp4_handler_param {
    mg_event_handler_t handler; // 必须与 struct ls_handler_param 的第一个参数一样
    int is_keep_alive;
    struct session_t sess;
};

static void _mp4_destroy_handler_param(struct ls_mp4_handler_param **ptr) {
    if (ptr && *ptr != NULL) {
        (*ptr)->handler = NULL;
        struct session_t * sess = &(*ptr)->sess;
        struct mg_connection *conn = ((struct mg_connection *) ((char *) (ptr) - offsetof(struct mg_connection, user_data)));
        TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p close fileID:%d, data_id:%d clip_id:%d filesize:%lld range[%lld,%lld] total_sent:%lld", conn, sess->fileID, sess->data_id, sess->clip_id, sess->filesize, sess->begin, sess->end, sess->total_sent);
        dm_fclose(sess->fileID);
        free(*ptr);
        *ptr = NULL;
    }
}

/// M3U8 Handler
static void __mp4_handler_request(struct mg_connection *conn, struct http_message *hm);
static void __mp4_handler_poll(struct mg_connection *conn, time_t *p_now);
static void __mp4_handler_close(struct mg_connection *conn, void* p/*NULL*/);

static const char * ipv4ToStr(const struct in_addr *in, char *buf, socklen_t buflen)
{
    struct in_addr ia = *in;
    ia.s_addr = htonl(ia.s_addr);
    return inet_ntop(AF_INET, (void *)&ia, buf, buflen);
}

static void __mp4_handler_request(struct mg_connection *conn, struct http_message *hm)
{
    struct ls_server *ls = (struct ls_server*)conn->mgr->user_data;
    char buf[128] = {0};
    const size_t buf_size = sizeof(buf) - 1;
    int keep_alive = is_keep_alive(hm);
    int status_code = 0;
    int var_len;
    const char *http_info = NULL; /**/
    char ip[32] = {0};
    
    mg_copy_mg_str(buf, sizeof(buf), &hm->uri);
    TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]__mp4_handler_request uri:%s conn:%p remote %s:%u",
                   buf, conn, ipv4ToStr(&conn->sa.sin.sin_addr, ip, sizeof(ip)-1), (int)ntohs(conn->sa.sin.sin_port));
    
    do { /// new request
        if (conn->user_data != NULL) {
            TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]__mp4_handler_request conn:%p  destroy old connection",
                           conn);
            _mp4_destroy_handler_param((struct ls_mp4_handler_param**)&conn->user_data);
        }
        
        if (ls->is_stop == 1) {
            status_code = 500;
            http_info = "Internal Server Error";
            snprintf(buf, buf_size, "__mp4_handler_request serve close");
            goto __mp4_handler_request_ERROR;
        }
        struct ls_mp4_handler_param param;
        memset(&param, 0, sizeof(param));
        
        // save callback
        param.handler = ls_mp4_handler;
        struct session_t *sess = &param.sess;
        
        ///// data_id
        var_len = mg_get_http_var(&hm->query_string, "data_id", buf, buf_size);
        if (var_len <= 0) {  // Not Found
            status_code = 400;
            http_info = "Bad Request";
            mg_copy_mg_str(buf, buf_size, &hm->query_string);
            goto __mp4_handler_request_ERROR;
        }
        buf[var_len]  = '\0';
        sess->data_id = dm_decode_dataid(buf);
        
        ///// clip_id
        var_len = mg_get_http_var(&hm->query_string, "clip_id", buf, buf_size);
        if (var_len <= 0) {  // Not Found
            status_code = 400;
            http_info = "Bad Request";
            mg_copy_mg_str(buf, buf_size, &hm->query_string);
            goto __mp4_handler_request_ERROR;
        }
        buf[var_len]  = '\0';
        sess->clip_id = (int32_t)strtoll(buf, NULL, 10);
        
        ///// Range
        const mg_str *range = mg_get_http_header(hm, "Range");
        if (NULL != range) {
            mg_copy_mg_str(buf, buf_size, range);
            // Parse Range
            int n = parse_range(buf, &sess->begin, &sess->end);
            if (n == 0) {
                sess->begin = 0;
                sess->end = -1;
            } else if ((n == 2 && sess->begin > sess->end) || (sess->begin < 0)) {
                status_code = 400;
                http_info = "Bad Request";
                /// now buf is http range
                goto __mp4_handler_request_ERROR;
            }
        } else {
            sess->begin = 0;
            sess->end = -1;
        }
        sess->filesize = -1;
        sess->cur = sess->begin;
        if (sess->end != -1 ) {
            sess->content_len = sess->end - sess->begin + 1;
        }
        sess->total_sent = 0;
        
        int ret = dm_fopen(sess->data_id, sess->clip_id, sess->begin, sess->end, &sess->fileID);
        TVHttpProxyLOG(LOG_INFO,
                       "[TVDownloadProxy_LocalProxy]__mp4_hanlder_request conn:%p accept request, dataid:%d,fileID:%d, %lld-%lld Connection:%s",
                       conn, sess->data_id, sess->fileID, sess->begin, sess->end, keep_alive ? "keep-alive" : "close");
        
        if (ret != 0) {
            // alloc failed
            status_code = 500;
            http_info = "Internal Server Error";
            snprintf(buf, buf_size, "__mp4_handler_request open file failed! data_id:%d clip_id:%d ret:%d", sess->data_id, sess->clip_id, ret);
            goto __mp4_handler_request_ERROR;
        }
        /// save connection_param for next call
        struct ls_mp4_handler_param *p = (struct ls_mp4_handler_param*)malloc(sizeof(struct ls_mp4_handler_param));
        if (NULL == p) {
            // alloc failed
            status_code = 500;
            http_info = "Internal Server Error";
            snprintf(buf, buf_size, "ts_handler_new alloc memory failed! bytes:%d", (int)sizeof(struct ls_mp4_handler_param));
            goto __mp4_handler_request_ERROR;
        }
        *p = param; /// copy param
        p->is_keep_alive = keep_alive;
        conn->user_data = (void*)p;
        time_t now = time(NULL);
        __mp4_handler_poll(conn, &now);
        return;
        
    } while (0);
    return; // 必须返回，后面的代码只是作为出错处理
    
__mp4_handler_request_ERROR: // output error
    if (status_code != 0) {
        size_t len = strlen(buf);
        mg_printf(conn,
                  "HTTP/1.1 %d %s\r\n"
                  "Content-Type: %s\r\n"
                  "Connection: %s\r\n"
                  "Content-Range: bytes %d-%d/%d\r\n"
                  "Accept-Ranges: bytes\r\n"
                  "Content-Length: %d\r\n"
                  "\r\n",
                  status_code, http_info,
                  "text/plain",
                  keep_alive ? "keep-alive" : "close",
                  0, (int)(len - 1), (int)len,
                  (int)len);
        mg_send(conn, buf, (int)len);
    }

//__mp4_handler_request_END: // serve end or failed!
    if (!keep_alive) {
        conn->flags |= MG_F_SEND_AND_CLOSE;
    }
}


static void __mp4_handler_poll(struct mg_connection *conn, time_t *p_now)
{
    struct ls_server *ls = (struct ls_server*)conn->mgr->user_data;
    struct ls_mp4_handler_param *param = (ls_mp4_handler_param*)conn->user_data;
    int keep_alive = 0;
    int status_code         = 0;
    int error_code          = 0;
    int server_error_code   = 0;
    const char *http_url    = "";
    const char *http_info   = "";
    
    //记录播放ID，用于日志输出
    int playDataID        = -1;
    
    //是否已经是非法播放Session
    int isInvalidSession  = 0;

#if defined (ANDROID)
    //Http错误响应
    int32_t httpStatusCode          = 0;
    int32_t httpDetailErrorCode     = 0;
    int32_t serverDetailErrorCode   = 0;
    char    httpErrorMsg[128]       = {0};
    char    httpURL[1024]           = {0};
#endif
    
    char buf[64 * 1024] = {0};
//    const size_t buf_size = sizeof(buf);
    
    do {
        if (NULL == param) {
            return;
//            status_code = 500;
//            http_info = "Internal Server Error";
//            snprintf(buf, buf_size, "__mp4_handler_poll param is NULL!");
//            goto __mp4_handler_poll_ERROR;
        }
        
        //如果已经stop，那么直接关闭连接, add by hualiangyan, 2015.07.22
        //DeInit里面DestroyServer 调用到这里，下面调用dmReadData里面dm_get_play_data里面的Mutex已经被析构了，会导致ANR
        if (ls->is_stop == 1) {
            isInvalidSession = 1;
            TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p , is_stop=1", conn);
            goto __mp4_handler_poll_END;
        }
        
        keep_alive = param->is_keep_alive;
        
        struct session_t *sess = &param->sess;
        
        playDataID = sess->data_id;
        
        if (sess->filesize == -1) {
            int32_t ret = dm_fsize(sess->data_id, sess->clip_id, &sess->filesize);
            if (ret != 0) {
                sess->filesize = -1;
                
                //如果播放Session非法，那么关闭连接
                if(ret == eResult_InvalidPlaySession)
                {
                    isInvalidSession = 1;
                    TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p , dm_fsize return invalid session", conn);
                    goto __mp4_handler_poll_END;
                }
                
#if defined (ANDROID)
                if(eResult_Success == dm_get_http_error_info(sess->fileID, &httpStatusCode, httpErrorMsg, sizeof(httpErrorMsg),
                                                             &httpDetailErrorCode, &serverDetailErrorCode, httpURL, sizeof(httpURL)))
                {
                    status_code         = httpStatusCode;
                    error_code          = httpDetailErrorCode;
                    server_error_code   = serverDetailErrorCode;
                    http_info           = httpErrorMsg;
                    http_url            = httpURL;
                    
                    TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p ,fileID:%d, occur error:http status code:%d, detailErrorCode:%d,serverErrorCode:%d", conn, sess->fileID, status_code, error_code, server_error_code);
                    
                    goto __mp4_handler_poll_ERROR;
                }
 #endif
                
                return;
            } else {
                if (sess->end == -1) sess->end = sess->filesize - 1;
                sess->content_len = sess->end - sess->begin + 1;
                
                const char *msg = "OK";
                int status_code = 200;
                
                const char * content_type = "video/mp4";
                char buf_ct[64] = {0};
                if (0 == dm_get_content_type(sess->data_id, sess->clip_id, buf_ct, sizeof(buf_ct)) && buf_ct[0] != '\0') {
                    content_type = buf_ct;
                    TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p set header Content-Type from CDN:%s", conn, content_type);
                }
                else{
                    TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p get header Content-Type from CDN failed", conn);
                }
                
                //需要进行ContentType的映射转换，有些视频的URL会返回ContentType:application/octet-stream,这种直接透传给播放器会直接报错
                const char* mapContentType = dm_map_content_type(content_type);
                if(mapContentType != NULL) {
                    content_type = mapContentType;
                }
                
                
                if (sess->content_len < sess->filesize) {
                    status_code = 206;
                    msg = "Partial Content";
                } else {
                    status_code = 200;
                    msg = "OK";
                }
                
                mg_printf(conn,
                          "HTTP/1.1 %d %s\r\n"
                          "Content-Type: %s\r\n"
                          "Connection: %s\r\n"
                          "Content-Range: bytes %" PRId64 "-%" PRId64 "/%" PRId64 "\r\n"
                          "Accept-Ranges: bytes\r\n"
                          "Content-Length: %" PRId64 "\r\n"
                          "\r\n",
                          status_code, msg,
                          content_type,
                          keep_alive ? "keep-alive" : "close",
                          sess->begin, sess->end, sess->filesize,
                          sess->content_len);
                
                TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p send header Content-Range: %lld-%lld/%lld, fileID:%d, now:%d", conn, sess->begin, sess->end, sess->filesize, sess->fileID, p_now ? (int)*p_now : -1);
            }
            
            sess->cur = sess->begin;
            
        }
        
        // Send Data
        if (sess->filesize != -1) {
            int64_t sent = 0;
            int64_t orginal_cur = sess->cur;
            static int64_t SEND_LEN_LIMIT = (2 * 1024 * 1024);
            while (sent <  SEND_LEN_LIMIT && sess->cur <= sess->end && conn->send_mbuf.len < SEND_LEN_LIMIT) {
                size_t need = (size_t)(sess->end - sess->cur + 1);
                need = need < sizeof(buf) ? need : sizeof(buf);
                size_t remain = (size_t)SEND_LEN_LIMIT - conn->send_mbuf.len;
                need = need < remain ? need : remain;
                
                int32_t read = 0;
                int32_t ret = dm_fread(sess->data_id, sess->clip_id, sess->cur, buf, (int32_t)need, &read);
                if (ret != 0) {
                    //如果播放Session非法，那么关闭连接
                    if(ret == eResult_InvalidPlaySession)
                    {
                        isInvalidSession = 1;
                        TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p , dm_fread return invalid session, now:%d", conn, p_now ? (int)*p_now : -1);
                        goto __mp4_handler_poll_END;
                    }

#if defined (ANDROID)
                    if(eResult_Success == dm_get_http_error_info(sess->fileID, &httpStatusCode, httpErrorMsg, sizeof(httpErrorMsg),
                                                                 &httpDetailErrorCode, &serverDetailErrorCode, httpURL, sizeof(httpURL)))
                    {
                        status_code         = httpStatusCode;
                        error_code          = httpDetailErrorCode;
                        server_error_code   = serverDetailErrorCode;
                        http_info           = httpErrorMsg;
                        http_url            = httpURL;
                        
                        TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p ,fileID:%d, occur error:http status code:%d, detailErrorCode:%d,serverErrorCode:%d", conn, sess->fileID, status_code, error_code, server_error_code);
                        
                        goto __mp4_handler_poll_ERROR;
                    }
#endif
                    break;
                } else { // read success
                    size_t real_sent = mg_send(conn, buf, (int)read);
                    if (0 == real_sent) {
                        TVHttpProxyLOG(LOG_ERROR, "[TVDownloadProxy_LocalProxy]__mp4_handler_poll conn:%p mg_send return 0, need_send:%d now:%d send_buflen:%d", conn, read, p_now ? (int)*p_now : -1, (int)conn->send_mbuf.len);
                        break;
                    } else {
                        sent += real_sent;
                        sess->cur += real_sent;
                    }
                }
            }
            
            if(sent > 0) {
                sess->total_sent += sent;
                TVHttpProxyLOG(LOG_DEBUG, "[TVDownloadProxy_LocalProxy]__mp4_handler_poll conn:%p send response data:%lld,%lld, now:%d send_buflen:%d total_sent:%lld", conn, orginal_cur, sent, p_now ? (int)*p_now : -1, (int)conn->send_mbuf.len, sess->total_sent);
            }
            if (sess->cur > sess->end) {
                //如果数据发送完毕
                TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]localserver conn:%p , send data finish. now:%d total_sent:%lld", conn, p_now ? (int)*p_now : -1, sess->total_sent);
                goto __mp4_handler_poll_END;
            }
            // 需要返回
            return;
        }
    } while (0);
    
    return; // 必须返回，后面的代码只是作为出错处理
    
__mp4_handler_poll_ERROR: // output error
    TVHttpProxyLOG(LOG_ERROR, "[TVDownloadProxy_LocalProxy]localserver conn:%p , occur error:http status code:%d", conn, status_code);
    if (status_code != 0) {
        int len = (int)strlen(buf);
        mg_printf(conn,
                  "HTTP/1.1 %d %s\r\n"
                  "Content-Type: %s\r\n"
                  "Connection: %s\r\n"
                  "Content-Range: bytes %d-%d/%d\r\n"
                  "Accept-Ranges: bytes\r\n"
                  "Content-Length: %d\r\n"
                  /*自定义的错误信息:详细错误码，服务器返回的详细错误码,下载URL*/
                  "x-proxy-error:%d\r\n"
                  "x-server-error:%d\r\n"
                  "x-source-url:%s\r\n"
                  "\r\n",
                  status_code, http_info,
                  "text/plain",
                  keep_alive ? "keep-alive" : "close",
                  0, (len - 1), len,
                  len,
                  error_code, server_error_code, http_url);
        mg_send(conn, buf, len);
    }
    
__mp4_handler_poll_END:
    _mp4_destroy_handler_param((struct ls_mp4_handler_param**)&conn->user_data);
    //非长连接或者已经是非法播放Session，那么关闭连接
    if (!keep_alive || (isInvalidSession == 1))
    {
        // MG_F_CLOSE_IMMEDIATELY 不要与 MG_F_SEND_AND_CLOSE 一起出现
        if (isInvalidSession == 1)
            conn->flags |= MG_F_CLOSE_IMMEDIATELY;
        else
            conn->flags = (conn->flags | MG_F_SEND_AND_CLOSE) & (~MG_F_CLOSE_IMMEDIATELY);
        
        TVHttpProxyLOG(LOG_INFO, "__mp4_handler_poll conn:%p close by localserver, dataid:%d", conn, playDataID);
    }
    
}

static void __mp4_handler_close(struct mg_connection *conn, void* p)
{
    char buf[32] = {0};
    TVHttpProxyLOG(LOG_INFO, "[TVDownloadProxy_LocalProxy]__mp4_handler_close conn:%p remote %s:%d", conn, ipv4ToStr(&conn->sa.sin.sin_addr, buf, sizeof(buf)-1), (int)ntohs(conn->sa.sin.sin_port));
    _mp4_destroy_handler_param((struct ls_mp4_handler_param**)&conn->user_data);
}

extern "C"
void ls_mp4_handler(struct mg_connection *conn, int ev, void *p)
{
    struct ls_server *ls = (struct ls_server*)conn->mgr->user_data;
    switch (ev) {
        case MG_EV_POLL :
            //            printf(" MG_EV_POLL p:%p now:%d\n", p, (int)*(time_t*)p);
            __mp4_handler_poll(conn, (time_t*)p);
            break;
        case MG_EV_CLOSE :
            __mp4_handler_close(conn, p);
            break;
            
        case MG_EV_HTTP_REQUEST :
            __mp4_handler_request(conn, (struct http_message*)p);
            break;
        
        case MG_EV_ERROR :
        {
            int err = *(int*)p;
            char str[256] = {0};
            strerror_r(err, str, sizeof(str) - 1);
            TVHttpProxyLOG(LOG_ERROR, "[TVDownloadProxy_LocalProxy] ls_mp4_handler socket error! errno:%d, %s", err, str);
            __mp4_handler_close(conn, p);
            conn->flags |= MG_F_CLOSE_IMMEDIATELY;
            break;
        }
        default:
            break;
    }
    
    if (ls->is_stop) {
        conn->flags = (conn->flags | MG_F_CLOSE_IMMEDIATELY) & (~MG_F_SEND_AND_CLOSE);
    }
}