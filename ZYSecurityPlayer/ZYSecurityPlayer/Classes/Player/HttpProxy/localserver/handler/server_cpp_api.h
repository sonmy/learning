//
//  server_cpp_api.h
//  download_manager
//
//  Created by leonllhuang on 15/5/3.
//
//

#ifndef __download_manager__server_cpp_api__
#define __download_manager__server_cpp_api__

#include <stdio.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    

    /// fopen file, return fileID
    int32_t dm_fopen(int32_t playDataID, int32_t clipNo, int64_t start, int64_t end, int32_t *fileID);
    
    /// get filesize
    int32_t dm_fsize(int32_t playDataID, int32_t clipNo, int64_t *fileSize);
    
    // get contetn_type
    int32_t dm_get_content_type(int32_t playDataID, int32_t clipNo, char * content_type, size_t content_type_size);
    
    /// read data
    int32_t dm_fread(int32_t playDataID, int32_t clipNo, int64_t offset, char *buffer, const int32_t bufferLength, int32_t *readSize);
    
    /// close file
    int32_t dm_fclose(int32_t fileID);
    
    int dm_get_service_type(int32_t fileID);
    
    uint32_t dm_decode_dataid(const char *str);

 #if defined (ANDROID)
    //获取http错误信息(http状态码，详细错误码，服务器详细错误码，HttpURL，错误描述)
    int32_t dm_get_http_error_info(int32_t fileID, int32_t *httpStatusCode, char* errorInfoBuffer, const int32_t bufferLength,
                                   int32_t *httpDetailErrorCode, int32_t* serverDetailErrorCode,
                                   char* httpURL, const int32_t httpURLLength);
 #endif  

    //映射content_type
    const char* dm_map_content_type(const char* sourceContentType);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* defined(__download_manager__server_cpp_api__) */
