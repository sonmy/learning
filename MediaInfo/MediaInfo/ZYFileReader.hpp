//
//  ZYFileReader.hpp
//  MediaInfo
//
//  Created by mys on 2022/8/18.
//

#ifndef ZYFileReader_hpp
#define ZYFileReader_hpp

#include <stdio.h>
#include <string>


#include "ZYBufferItem.hpp"

class ZYFileReader {
private:
    FILE *_fileHandle;
public:
    int errCode;
    ZYFileReader(std::string filePath);
    std::shared_ptr<struct ZYBufferItem> CopyNextBuffer(size_t size, int *error);
    size_t ReadBytes(void *buf, size_t size);
    double ReadFloat64(void);
    float  ReadFloat32(void);
    uint64_t ReadUInt64(void);
    uint32_t ReadUInt32(void);
    uint16_t ReadUInt16(void);
    uint8_t ReadUInt8(void);
    uint32_t Read3BytesUInt(void);
    size_t CurPos(void);
    
    bool Seek(long offset);
    bool SeekTo(long offset);
    
    bool isEOF(void);
    
};
#endif /* ZYFileReader_hpp */
