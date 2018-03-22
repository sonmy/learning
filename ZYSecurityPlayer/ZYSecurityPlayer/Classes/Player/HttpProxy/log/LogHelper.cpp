//
//  LogHelper.cpp
//  download_manager
//
//  Created by vinowan on 15-5-12.
//
//

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "LogHelper.h"

#if defined (ANDROID)
#include "JNIHelper.h"
using namespace nspi;
#endif


extern "C" const char* LogHelper_GetBaseFileName(const char* pszPath)
{
    if(pszPath == NULL)
    {
        return "";
    }
    
    char *baseFileName = strrchr(pszPath, '/');
    
    if(baseFileName)
    {
        return baseFileName + 1;
    }
    else
    {
        return "";
    }
}


extern "C" void LogHelper_HttpProxy_Log(const char* file,
                              int dLine,
                              int dLevel,
                              const char* tag,
                              const char* format,
                              ...)
{
    if(file == NULL || tag == NULL || format == NULL)
    {
        return;
    }
    
    //日志级别控制
//    if(dLevel < GlobalConfig::MinLogLevel)
//    {
//        return;
//    }
//#define LOG_VERBOSE 0
//#define LOG_DEBUG   20
//#define LOG_INFO    40
//#define LOG_WARN    80
//#define LOG_ERROR   100
    
    char levelStrBuf[32] = {0};
    const char * levelStr = "Unknown";
    switch (dLevel) {
        case LOG_VERBOSE:
            levelStr = "LOG_VERBOSE";
            break;
        case LOG_DEBUG:
            levelStr = "LOG_DEBUG";
            break;
        case LOG_INFO:
            levelStr = "LOG_INFO";
            break;
        case LOG_WARN:
            levelStr = "LOG_WARN";
            break;
        case LOG_ERROR:
            levelStr = "LOG_ERROR";
            break;
        default:
            snprintf(levelStrBuf, sizeof(levelStrBuf), "LOG_UNKNOWN:%d", dLevel);
            levelStr = levelStrBuf;
            break;
    }
    
    time_t now = 0;
    struct timeval tv_now = {0};
    if (0 == gettimeofday(&tv_now, NULL)) {
        now = (time_t)tv_now.tv_sec;
    } else {
        now = time(NULL);
        tv_now.tv_sec = now;
        tv_now.tv_usec = 0;
    }
    
    struct tm ltime;
    localtime_r(&now, &ltime);
    
    const size_t kLogStrLen = 2048;
    char logStr[kLogStrLen];
    size_t offset       = 0;
    
    size_t size_LevelStr = snprintf(logStr + offset, kLogStrLen - offset, "#%s", levelStr);
    offset += size_LevelStr;
    
    size_t size_time    = strftime(logStr + offset, kLogStrLen - offset, " %F %T.", &ltime);
    offset += size_time;
    
    int size_file_name  = snprintf(logStr + offset, kLogStrLen - offset,  "%03d[%s:%d] ", (int)(tv_now.tv_usec / 1000.0 + 0.5), LogHelper_GetBaseFileName(file), dLine);
    offset += size_file_name;
    
    va_list args;
    va_start(args, format);
    vsnprintf(logStr + offset, kLogStrLen - offset, format, args);
    va_end(args);


    //dmLogUseLogFunc(dLevel, logStr);
}
