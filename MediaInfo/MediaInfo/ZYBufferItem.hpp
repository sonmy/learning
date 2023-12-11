//
//  ZYBufferItem.hpp
//  MediaInfo
//
//  Created by mys on 2022/8/18.
//

#ifndef ZYBufferItem_hpp
#define ZYBufferItem_hpp

#include <memory>

struct ZYBufferItem {
    char *buf;
    size_t len;
    
    ZYBufferItem(char *input, size_t size) {
        buf = input;
        len = size;
    };
    
    ZYBufferItem(size_t size) {
        buf = (char*)malloc(size);
        len = size;
    };
    
    ~ZYBufferItem() {
        if (buf) {
            free(buf);
        }
    };
};

#endif /* ZYBufferItem_hpp */
