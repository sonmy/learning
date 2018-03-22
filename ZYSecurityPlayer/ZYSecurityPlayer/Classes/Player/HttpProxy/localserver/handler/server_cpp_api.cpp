//
//  server_cpp_api.cpp
//  download_manager
//
//  Created by leonllhuang on 15/5/3.
//
//
#include <string.h>

#include "server_cpp_api.h"
#include "DownloadManager.h"
#include "GlobalError.h"
#include "DataCollect.h"

//#define LOCAL_SERVER_TEST_FILE

#ifndef LOCAL_SERVER_TEST_FILE

/// fopen file, return fileID
extern "C" int32_t dm_fopen(int32_t playDataID, int32_t clipNo, int64_t start, int64_t end, int32_t *fileID) {
    return dmOpenFile(playDataID, clipNo, start, end, *fileID) == eResult_Success ? 0 : -1;
}

/// get filesize
extern "C" int32_t dm_fsize(int32_t playDataID, int32_t clipNo, int64_t *fileSize) {
    return dmGetFileSize(playDataID, clipNo, *fileSize);
}

extern "C" int32_t dm_get_content_type(int32_t playDataID, int32_t clipNo, char * content_type, size_t content_type_size) {
    return dmGetContentType(playDataID, clipNo, content_type, content_type_size) == eResult_Success ? 0 : -1;
}
/// read data
extern "C" int32_t dm_fread(int32_t playDataID, int32_t clipNo, int64_t offset, char *buffer, const int32_t bufferLength, int32_t *readSize) {
    return dmReadData(playDataID, clipNo, offset, buffer, bufferLength, *readSize);
}

/// close file
extern "C" int32_t dm_fclose(int32_t fileID) {
    return dmCloseFile(fileID) == eResult_Success ? 0 : -1;
}

extern "C" int dm_get_service_type(int32_t fileID) {
    return dmGetServiceType(fileID);
}

#else
#define VIDEO_FILE "/Users/payneyang/code/v0015kd6an0.msd.tmv"

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <utility>

typedef std::map<int32_t, void* > MapFile;
static MapFile mf;

#define VIDEO_DATA_ID 15728641

struct test_data {
    int ref;
    char data[];
};
/// fopen file, return fileID
extern "C" int32_t dm_fopen(int32_t playDataID, int32_t clipNo, int64_t start, int64_t end, int32_t *fileID) {
//    return dmOpenFile(playDataID, start, end, *fileID) == eResult_Success ? 0 : 1;
    if (playDataID < VIDEO_DATA_ID) {
        *fileID = playDataID;
        MapFile::iterator it = mf.find(playDataID);
        struct test_data * data = NULL;
        if (it == mf.end()) {
            int filesize = playDataID * 8;
            data = (struct test_data*)malloc(sizeof(test_data) + filesize + 3);
            for (int i = 1; i <= playDataID; i++) {
                snprintf(data->data + (i-1)*8, 9, "%08x", i);
            }
            mf.insert( std::make_pair(playDataID, (void*)data) );
            data->ref = 1;
        } else {
            data = (struct test_data*)it->second;
            data->ref++;
        }
        
        return 0;
    }
    
    FILE * f = fopen(VIDEO_FILE, "rb");
    *fileID = playDataID;
    if (f != NULL) {
        mf.insert( std::make_pair(playDataID, (void*)f) );
        return 0;
    }
    return -1;
}

/// get filesize
extern "C" int32_t dm_fsize(int32_t playDataID, int32_t clipNo, int64_t *fileSize) {
    
    if (playDataID < VIDEO_DATA_ID) {
//        char buf[64] = {0};
//        *fileSize = snprintf(buf, sizeof(buf), "%d", playDataID);
        *fileSize = playDataID * 8;
        return 0;
    }
    
    MapFile::iterator it = mf.find(playDataID);
    if (it != mf.end()) {
        *fileSize = 68176031;
        return 0;
    }
    return -1;
}

/// read data
extern "C" int32_t dm_fread(int32_t playDataID, int32_t clipNo, int64_t offset, char *buffer, const int32_t bufferLength, int32_t *readSize) {
    
    if (playDataID < VIDEO_DATA_ID) {
        int64_t filesize = playDataID * 8;
        if ((int)offset >= filesize || offset < 0) return -1;
        
        *readSize = (int)(filesize - offset);
        if (*readSize > bufferLength) *readSize = bufferLength;
        
        MapFile::iterator it = mf.find(playDataID);
        struct test_data* data = (struct test_data*)it->second;
        char * mem = (char*)data->data;
        memcpy(buffer, mem + offset, *readSize);
        
        return 0;
    }
    
    MapFile::iterator it = mf.find(playDataID);
    if (it != mf.end()) {
        FILE *f = (FILE*)it->second;
        fseek(f, (long)offset, SEEK_SET);
        
        size_t read = fread(buffer, 1, (size_t)bufferLength, f);
        if (read > 0) *readSize = (int32_t)read;
        else *readSize = 0;
        return 0;
    }
    return -1;
}

/// close file
extern "C" int32_t dm_fclose(int32_t fileID) {
    
    MapFile::iterator it = mf.find(fileID);
    
    if (fileID < VIDEO_DATA_ID) {
        if (it != mf.end() ) {
            struct test_data* data = (struct test_data*)it->second;
            data->ref--;
//            if (data->ref == 0) {
//                free(it->second);
//                mf.erase(it);
//            }
        }
        return 0;
    }
    if (it != mf.end()) {
        FILE *f = (FILE*)it->second;
        mf.erase(it);
        if (f != NULL) fclose(f);
        return 0;
    }
    return -1;
}

#endif

uint32_t dm_decode_dataid(const char *str)
{
#ifndef LOCAL_SERVER_TEST_FILE
    return dmDecodeDataId(str);
#else
    return atoi(str);
#endif
}

#if defined (ANDROID)
static int32_t getHttpErrorMsg(const int32_t httpStatusCode, char* errorInfoBuffer, const int32_t bufferLength)
{
    if(errorInfoBuffer == NULL || bufferLength <= 0)
    {
        return eResult_InvalidParam;
    }
//    static const char g_100[] = "HTTP/1.1 100 Continue\r\n";
//    static const char g_101[] = "HTTP/1.1 Switching Protocols\r\n";
//    static const char g_200[] = "HTTP/1.1 200 OK\r\n";
//    static const char g_201[] = "HTTP/1.1 201 Created\r\n";
//    static const char g_202[] = "HTTP/1.1 202 Accepted \r\n";
//    static const char g_203[] = "HTTP/1.1 203 Non-Authoritative Information\r\n";
//    static const char g_204[] = "HTTP/1.1 204 No Content\r\n";
//    static const char g_205[] = "HTTP/1.1 205 Reset Content\r\n";
//    static const char g_206[] = "HTTP/1.1 206 Partial Content\r\n";
//    static const char g_300[] = "HTTP/1.1 300 Multiple Choices\r\n";
//    static const char g_301[] = "HTTP/1.1 301 Moved Permanently\r\n";
//    static const char g_302[] = "HTTP/1.1 302 Found\r\n";
//    static const char g_303[] = "HTTP/1.1 303 See Other\r\n";
//    static const char g_304[] = "HTTP/1.1 304 Not Modified\r\n";
//    static const char g_305[] = "HTTP/1.1 305 Use Proxy\r\n";
//    static const char g_307[] = "HTTP/1.1 307 Temporary Redirect\r\n";
    static const char g_400[] = "Bad Request";
    static const char g_401[] = "Unauthorized";
    static const char g_402[] = "Payment Required";
    static const char g_403[] = "Forbidden";
    static const char g_404[] = "Not Found";
    static const char g_405[] = "Method Not Allowed";
    static const char g_406[] = "Not Acceptable";
    static const char g_407[] = "Proxy Authentication Required";
    static const char g_408[] = "Request Time-out";
    static const char g_409[] = "Conflict";
    static const char g_410[] = "Gone";
    static const char g_411[] = "Length Required";
    static const char g_412[] = "Precondition Failed";
    static const char g_413[] = "Request Entity Too Large";
    static const char g_414[] = "Request-URI Too Large";
    static const char g_415[] = "Unsupported Media Type";
    static const char g_416[] = "Requested range not satisfiable";
    static const char g_417[] = "Expectation Failed";
    static const char g_500[] = "Internal Server Error";
    static const char g_501[] = "Not Implemented";
    static const char g_502[] = "Bad Gateway";
    static const char g_503[] = "Service Unavailable";
    static const char g_504[] = "Gateway Time-out";
    static const char g_505[] = "HTTP Version not supported";

    
    switch (httpStatusCode) {
            case 400:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_400, bufferLength - 1);
            break;
            
            case 401:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_401, bufferLength - 1);
            break;
            
            case 402:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_402, bufferLength - 1);
            break;
            
            case 403:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_403, bufferLength - 1);
            break;
            
            case 404:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_404, bufferLength - 1);
            break;
            
            case 405:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_405, bufferLength - 1);
            break;
            
            case 406:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_406, bufferLength - 1);
            break;
            
            case 407:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_407, bufferLength - 1);
            break;
            
            case 408:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_408, bufferLength - 1);
            break;
            
            case 409:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_409, bufferLength - 1);
            break;
            
            case 410:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_410, bufferLength - 1);
            break;
            
            case 411:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_411, bufferLength - 1);
            break;
            
            case 412:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_412, bufferLength - 1);
            break;
            
            case 413:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_413, bufferLength - 1);
            break;
            
            case 414:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_414, bufferLength - 1);
            break;
            
            case 415:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_415, bufferLength - 1);
            break;
            
            case 416:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_416, bufferLength - 1);
            break;
            
            case 417:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_417, bufferLength - 1);
            break;
            
            case 500:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_500, bufferLength - 1);
            break;
            
            case 501:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_501, bufferLength - 1);
            break;
            
            case 502:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_502, bufferLength - 1);
            break;
            
            case 503:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_503, bufferLength - 1);
            break;
            
            case 504:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_504, bufferLength - 1);
            break;
            
            case 505:
            memset(errorInfoBuffer, 0, bufferLength);
            strncpy(errorInfoBuffer, g_505, bufferLength - 1);
            break;
            
        default:
        {
            return eResult_Unknown;
        }
    }
    
    return eResult_Success;
}

//获取http错误信息(http状态码，服务器详细错误码，错误描述)
int32_t dm_get_http_error_info(int32_t fileID, int32_t *httpStatusCode, char* errorInfoBuffer, const int32_t bufferLength,
                               int32_t *httpDetailErrorCode, int32_t* serverDetailErrorCode,
                               char* httpURL, const int32_t httpURLLength)
{
    if(httpStatusCode == NULL || errorInfoBuffer == NULL || bufferLength <= 0
       || httpDetailErrorCode == NULL || serverDetailErrorCode == NULL
       || httpURL == NULL || httpURLLength <= 0)
    {
        return eResult_InvalidParam;
    }
    
    int32_t hr = eResult_InvalidParam;
    int serviceType = dm_get_service_type(fileID);
    if (serviceType < 0) {
        return eResult_InvalidParam;
    }
    DataCollect::GetInstance(serviceType)->GetHttpErrorInfo(fileID, *httpStatusCode, *httpDetailErrorCode, *serverDetailErrorCode, httpURL, httpURLLength);
    
    if(hr != eResult_Success)
    {
        return hr;
    }
    
    return getHttpErrorMsg(*httpStatusCode, errorInfoBuffer, bufferLength);
}

#endif

//映射content_type
const char* dm_map_content_type(const char* sourceContentType)
{
    return dmGetMapContentType(sourceContentType);
}
