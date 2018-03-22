//
//  LogHelper.h
//  download_manager
//
//  Created by vinowan on 15-5-12.
//
//

#ifndef __download_manager__LogHelper__
#define __download_manager__LogHelper__

#define LOG_VERBOSE 0
#define LOG_DEBUG   20
#define LOG_INFO    40
#define LOG_WARN    80
#define LOG_ERROR   100



#if 1
#define TVHttpProxyLOG_DEBUG(...) LogHelper_HttpProxy_Log(__FILE__, __LINE__, (LOG_DEBUG), "TencentVideoHttpProxy",__VA_ARGS__)
#define TVHttpProxyLOG(level,format, ...) LogHelper_HttpProxy_Log(__FILE__, __LINE__, (level), "TencentVideoHttpProxy",(format), __VA_ARGS__)
#else
#define TVHttpProxyLOG(level,format, ...) printf((format), __VA_ARGS__);
#endif


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//获取文件名(__FILE__宏获取到的是文件全路径)
const char* LogHelper_GetBaseFileName(const char* pszPath);

void LogHelper_HttpProxy_Log(const char* file,
                   int dLine,
                   int dLevel,
                   const char* tag,
                   const char* format,
                   ...);
#ifdef __cplusplus
    }
#endif // __cplusplus

#endif /* defined(__download_manager__LogHelper__) */
