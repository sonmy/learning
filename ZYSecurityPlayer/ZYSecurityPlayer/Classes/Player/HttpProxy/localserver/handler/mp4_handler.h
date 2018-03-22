//
//  mp4_handler.h
//  download_manager
//
//  Created by leonllhuang on 1/17/16.
//
//

#ifndef mp4_handler_h
#define mp4_handler_h



#include "mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    
    void ls_mp4_handler(struct mg_connection *conn, int ev, void *p);
    
#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* mp4_handler_h */
