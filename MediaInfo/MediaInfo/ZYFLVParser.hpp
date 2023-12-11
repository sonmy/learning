//
//  ZYFLVParser.hpp
//  MediaInfo
//
//  Created by mys on 2022/9/4.
//

#ifndef ZYFLVParser_hpp
#define ZYFLVParser_hpp

#include <stdint.h>
#include "ZYFileReader.hpp"

struct ZYFLVHeader {
    uint8_t signature[3]; //3字节"FLV"
    uint8_t version; //1字节 only 0x01 is valid
    uint8_t flags; //1字节 5bit：保留 1bit：是否有音频 1bit：保留 1bit是否有视频
    uint32_t header_size; //4字节
};

struct ZYFLVTagInfo {
    /// 1字节 1-2bit:必须为0，保留位；第3bit：0未加密 1加密 默认为0
    ///4-8bit：8-音频 9-视频 18-script数据
    uint8_t type;
    uint32_t dataSize; //3字节 tagData长度，不包括11字节的taghead
    uint32_t timestamp; // 3字节，毫秒，第一个tag为0
    uint32_t extTimestamp; // 1字节，当timestamp不够时，用来做高8位，很少使用
    uint32_t streamID; //流id，总是0 3字节;
    
    size_t file_offset;
};

enum FLVMetaDataType : uint8_t {
    FLVMetaDataTypeNumber = 0, //8字节double
    FLVMetaDataTypeBool = 1, //1字节bool
    FLVMetaDataTypeString = 2, //后面2个字节为长度
    FLVMetaDataTypeObject = 3, //
    FLVMetaDataTypeMovieClip = 4, //
    FLVMetaDataTypeNull = 5, //
    FLVMetaDataTypeUndefined = 6, //
    FLVMetaDataTypeReference = 7, //
    FLVMetaDataTypeECMAArray = 8, //数组，类似Map
    FLVMetaDataTypeStrictArray = 10, //
    FLVMetaDataTypeDate = 11, //
    FLVMetaDataTypeLongString = 12, //后面4字节为长度
};

enum ZYFLVTrackType : uint8_t {
    ZYFLVTrackTypeVideo = 0,
    ZYFLVTrackTypeAudio = 1,
};

enum ZYFLVCodecType : uint8_t {
    ZYFLVCodecTypeAVC = 1,
    ZYFLVCodecTypeHEVC = 2,
    ZYFLVCodecTypeAAC = 10,
    ZYFLVCodecTypeUnknow
};

struct ZYFLVTrackInfo {
    ZYFLVTrackType type;
    ZYFLVCodecType codec;
    float duration;
    float fps;
    float bitrate;
    int width;
    int height;
    int sampleSize;
    int sampleRate;
};



enum ZYFLVFrameType : uint32_t {
    ZYFLVFrameTypeKey = 1, //keyframe (for AVC, a seekable frame)
    ZYFLVFrameTypeInter = 2, //inter frame (for AVC, a nonseekable frame)
    ZYFLVFrameTypeDisposableInter = 3, //disposable inter frame (H.263 only)
    ZYFLVFrameTypeServerKey = 4, //generated keyframe (reserved forserver use only)
    ZYFLVFrameTypeInfo = 5, //video info/command frame
};
enum ZYFLVCodecID : uint32_t {
    ZYFLVCodecIDJPEG = 1, //JPEG (currently unused)
    ZYFLVCodecIDH263 = 2, //Sorenson H.263
    ZYFLVCodecIDScreen = 3, //Screen video
    ZYFLVCodecIDOn2 = 4, //On2 VP6
    ZYFLVCodecIDOn2Alple = 5, //On2 VP6 with alpha channel
    ZYFLVCodecIDScreenV2 = 6, //Screen video version 2
    ZYFLVCodecIDAVC = 7, //AVC
    ZYFLVCodecIDMayHEVC = 0x0c, //maybe hevc
};

enum ZYFLVAVCPacketType {
    ZYFLVAVCPacketTypeAVCConfig = 0, //AVCDecoderConfigurationRecord(AVC sequence header)
    ZYFLVAVCPacketTypeNALU = 1, //AVC NALU
    ZYFLVAVCPacketTypeEndOfAVCConfig = 2, //AVC end of sequence(lower level NALU sequence ender is not required or supported)
};

struct ZYFLVTagDataVideo {
    ZYFLVFrameType frameType; //4bit
    ZYFLVCodecID codecID;    //4bit
    ZYFLVAVCPacketType packetType; //1 byte
    int32_t compostionTimeOffset; // 3 bytes packetType为NALU时有效，其他场景为0
};

//https://en.wikipedia.org/wiki/Advanced_Video_Coding#Profiles
enum ZYAVCProfile {
    ZYAVCProfile_Baseline = 66, // 0x42
    ZYAVCProfile_Main = 77, // 0x4D
    ZYAVCProfile_High = 100, //0x64
    ZYAVCProfile_High10 = 110, //0x6E 10bit per sample
    ZYAVCProfile_High422 = 122, //0x7A
};

class ZYFLVParser {
private:
    ZYFileReader *_fileReader;
public:
    ZYFLVParser(const char *filePath);
    ~ZYFLVParser();
    void PrintTagInfo();
    void PrintSPS(size_t size);
};
#endif /* ZYFLVParser_hpp */
