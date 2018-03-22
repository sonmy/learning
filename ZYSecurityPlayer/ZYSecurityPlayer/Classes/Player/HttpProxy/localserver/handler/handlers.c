//
//  handlers.c
//  download_manager
//
//  Created by user_imac on 11/13/15.
//
//

#include <string.h>
#include "handlers.h"

// 在这里声明使用的 handler ，然后在其它位置写好 c 文件，作为库链接进来，编译器会自动寻找函数定义
/// begin: server handlers, work part
// mg_handler
// handler example : void ls_hls_handler(struct mg_connection *conn, int ev, void *p)

#include "mp4_handler.h"

enum URIMatchType { URI_MATCH_ALL, URI_MATCH_PREFIX };
struct _HandlerPair {
    enum URIMatchType type; // 0 match, 1 prefix match
    const char * requset_name;
    mg_event_handler_t handler;
};

static struct _HandlerPair st_handlers[] = {
#ifdef PLUGIN_P2PLIVE
    {URI_MATCH_PREFIX,  "/playhls/", ls_hls_handler},
#endif
    {URI_MATCH_ALL,  "/playmp4", ls_mp4_handler},
//    {URI_MATCH_ALL,  "/test_keepalive", ls_test_keepalive},
//    {"router_get_router_type", ls_router_get_router_type},
//    {"router_check_router_password", ls_router_check_router_password},
//    {"router_encode_mac", ls_router_encode_mac},
//    {"router_has_storage", ls_router_has_storage},
    {URI_MATCH_ALL, NULL}
};

mg_event_handler_t ls_find_handler(const struct http_message* hm)
{
    if (NULL == hm) {
        return NULL;
    }
    /// find handler
    const struct mg_str *uri = &(hm->uri);
    if (uri != NULL) {
        const int cnt = sizeof(st_handlers) / sizeof(struct _HandlerPair);
        int i ;
        for (i = 0; i < cnt; i++) {
            if (NULL == st_handlers[i].requset_name)
                continue;
            const char* const requset_name = st_handlers[i].requset_name;
            switch (st_handlers[i].type) {
                case URI_MATCH_ALL:
                    if ( NULL != requset_name && 0 == mg_vcasecmp(uri, requset_name)) {
                        return st_handlers[i].handler;
                    }
                    break;
                case URI_MATCH_PREFIX:
                {
                    size_t len = strlen(requset_name);
                    len = len < uri->len ? len : uri->len;
                    if ( NULL != requset_name && 0 == strncasecmp(uri->p, requset_name, len)) {
                        return st_handlers[i].handler;
                    }
                }
                default:
                    break;
            }
        }
    }
    return NULL;
}
