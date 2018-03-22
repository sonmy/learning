//
//  ZYAVDecoder.c
//  ZYSecurityPlayer
//
//  Created by mysong on 2017/5/5.
//  Copyright © 2017年 tencent. All rights reserved.
//

#include "ZYAVDecoder.h"
#include <VideoToolbox/VideoToolbox.h>
#include "libavcodec/avcodec.h"

int zyavdecode(int session)
{
    //VTDecompressionSessionDecodeFrame;
    avcodec_register_all();
    return 1;
}


