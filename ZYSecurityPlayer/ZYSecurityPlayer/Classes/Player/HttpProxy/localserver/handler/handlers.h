//
//  handlers.h
//  download_manager
//
//  Created by user_imac on 11/13/15.
//
//

#ifndef handlers_h
#define handlers_h

#include "mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
// 查找 HTTP Request 处理器
mg_event_handler_t ls_find_handler(const struct http_message *hm);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* handlers_h */
