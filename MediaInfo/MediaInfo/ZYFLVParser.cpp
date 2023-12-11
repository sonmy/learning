//
//  ZYFLVParser.cpp
//  MediaInfo
//
//  Created by mys on 2022/9/4.
//

#include "ZYFLVParser.hpp"
#include "ZYBufferItem.hpp"
#include <vector>

std::string GetProfileName(enum ZYAVCProfile profile)
{
    switch (profile) {
        case ZYAVCProfile_Baseline:
            return "BaseLine";
        case ZYAVCProfile_Main:
            return "Main";
        case ZYAVCProfile_High:
            return "High";
        default:
            return "unkown";
    }
}

ZYFLVParser::ZYFLVParser(const char *filePath)
{
    _fileReader = new ZYFileReader(filePath);
}

ZYFLVParser::~ZYFLVParser()
{
    if (_fileReader) {
        delete _fileReader;
    }
}

void ZYFLVParser::PrintTagInfo()
{
    ZYFLVHeader head = {0};
    _fileReader->ReadBytes(head.signature, sizeof(head.signature));
    head.version = _fileReader->ReadUInt8();
    head.flags = _fileReader->ReadUInt8();
    head.header_size = _fileReader->ReadUInt32();
    bool hasAudio = (head.flags & 0x4) != 0 ? 1 : 0;
    bool hasVideo = (head.flags & 0x1) != 0 ? 1 : 0;
    
    printf("文件类型:%c%c%c Ver:%d 音频:%d 视频:%d\n", head.signature[0], head.signature[1], head.signature[2], head.version, hasAudio, hasVideo);
    std::vector<struct ZYFLVTagInfo> tagList;
    ZYFLVTagInfo tagInfo;
    
    int tagCount = 0;
    int scriptCount = 0;
    int audioCount = 0;
    int videoCount = 0;
    while (!_fileReader->isEOF()) {
        memset(&tagInfo, 0, sizeof(ZYFLVTagInfo));
        int32_t preTagSize = _fileReader->ReadUInt32();
        tagInfo.file_offset = _fileReader->CurPos();
        tagInfo.type = _fileReader->ReadUInt8();
        tagInfo.dataSize = _fileReader->Read3BytesUInt();
        tagInfo.timestamp = _fileReader->Read3BytesUInt();
        tagInfo.extTimestamp = _fileReader->ReadUInt8();
        tagInfo.streamID = _fileReader->Read3BytesUInt();
        //printf("tagType->%d preSize(%d) file_offset(%ld) size(%u)\n", tagInfo.type, preTagSize, tagInfo.file_offset, tagInfo.dataSize+11);
        if (tagInfo.type == 18) {
            //脚本tag
            scriptCount++;
        } else if (tagInfo.type == 8) {
            //audio
            audioCount++;
        } else if (tagInfo.type == 9) {
            //video
            videoCount++;
        }
        if (_fileReader->isEOF()) {
            break;
        }
        _fileReader->Seek(tagInfo.dataSize);
        tagCount++;
        tagList.push_back(tagInfo);
    }
    printf("tag数量:%d script:%d audio:%d video:%d\n", tagCount, scriptCount, audioCount, videoCount);
    
    ZYFLVTagInfo scriptTagInfo = tagList.at(0);
    for(ZYFLVTagInfo &tag:tagList) {
        
    }
    
//    _fileReader->SeekTo(scriptTagInfo.offset);
//    _fileReader->Seek(11);
//    enum FLVMetaDataType amfType = (enum FLVMetaDataType)_fileReader->ReadUInt8();
//    if (amfType == FLVMetaDataTypeString) {
//        int len = _fileReader->ReadUInt16();
//        int error = 0;
//        std::shared_ptr<struct ZYBufferItem> buffer = _fileReader->CopyNextBuffer(len, &error);
//        std::string value = std::string(buffer->buf, buffer->len);
//        printf("%s\n", value.c_str());
//    }
//    amfType = (enum FLVMetaDataType)_fileReader->ReadUInt8();
//    if (amfType == FLVMetaDataTypeECMAArray) {
//        int metaCount = _fileReader->ReadUInt32();
//        printf("metaData数量:%d\n", metaCount);
//z
//        enum FLVMetaDataType valueType;
//        int error = 0;
//        for (int i = 0; i < metaCount; i++) {
//            int len = _fileReader->ReadUInt16();
//            std::shared_ptr<ZYBufferItem> buf = _fileReader->CopyNextBuffer(len, &error);
//            std::string key = std::string(buf->buf, buf->len);
//            printf("%s:", key.c_str());
//            valueType = (enum FLVMetaDataType)_fileReader->ReadUInt8();
//            if (valueType == FLVMetaDataTypeNumber) {
//                double value = (double)_fileReader->ReadFloat64();
//                if (key == "videocodecid") {
//                    if ((int)value == 7) {
//                        printf("%s\n", "H264");
//                        continue;
//                    }
//                }
//                if (key == "audiocodecid") {
//                    if ((int)value == 10) {
//                        printf("%s\n", "AAC");
//                        continue;
//                    }
//                }
//                printf("%.2f\n", value);
//            } else if (valueType == FLVMetaDataTypeBool) {
//                bool value = (double)_fileReader->ReadUInt8();
//                printf("%d\n", value);
//            } else if (valueType == FLVMetaDataTypeString) {
//                int len = _fileReader->ReadUInt16();
//                std::shared_ptr<ZYBufferItem> buf = _fileReader->CopyNextBuffer(len, &error);
//                std::string value = std::string(buf->buf, buf->len);
//                printf("%s\n", value.c_str());
//            }
//        }
//    } else if (amfType == FLVMetaDataTypeObject) {
//
//    }
//
//    for (int i = 1; i < tagList.size(); i++) {
//        ZYFLVTagInfo tagInfo = tagList.at(i);
//        if (tagInfo.head.type == ZYFLVTagVideo) {
//            _fileReader->SeekTo(tagInfo.offset + 11);
//            struct ZYFLVTagDataVideo videoData;
//            uint8_t videoflag = _fileReader->ReadUInt8();
//            videoData.frameType = (enum ZYFLVFrameType)(videoflag>>4);
//            videoData.codecID = (enum ZYFLVCodecID)(videoflag&0x0f);
//            ZYFLVAVCPacketType type = (enum ZYFLVAVCPacketType)_fileReader->ReadUInt8();
//            int32_t timeOffset = _fileReader->Read3BytesUInt();
//
//            if (type == ZYFLVAVCPacketTypeAVCConfig) {
//                //sps解析
//                int8_t configurationVersion = _fileReader->ReadUInt8();
//                enum ZYAVCProfile avcProfileIndication = (enum ZYAVCProfile)_fileReader->ReadUInt8(); //sps[1]
//                int8_t profile_compatibility = _fileReader->ReadUInt8(); //sps[2]
//                int8_t avcLevelIndication = _fileReader->ReadUInt8(); //sps[3]
//
//                int8_t bytes = _fileReader->ReadUInt8(); //6bit reserved 2bit NALUnitLength的长度-1,一般为3
//                int lengthSizeMinusOne = bytes & 0x3;
//                bytes = _fileReader->ReadUInt8(); // 3bit reserved 5bit sps个数, 一般为1
//                int numberOfSequenceParameterSets = bytes & 0x1F;
//
//                for (int i = 0; i < numberOfSequenceParameterSets; i++) {
//                    uint16_t sequenceParameterSetLength = _fileReader->ReadUInt16();
//                    PrintSPS(sequenceParameterSetLength);
//
//                }
//                printf("profile(%s@%d.%d)", GetProfileName(avcProfileIndication).c_str(), avcLevelIndication/10,avcLevelIndication%10);
//                break;
//            }
//
//
//            printf("");
//        }
//    }
    printf("");
}

void ZYFLVParser::PrintSPS(size_t size) {
    int8_t nalType = _fileReader->ReadUInt8(); //0x67 pps
    enum ZYAVCProfile avcProfileIndication = (enum ZYAVCProfile)_fileReader->ReadUInt8();
    uint8_t constraint_set = _fileReader->ReadUInt8();
    int8_t avcLevelIndication = _fileReader->ReadUInt8();
    printf("");
}


int read_ue(unsigned char *data, int *index) {
    int leading_zeros = -1;
    int i = *index;

    while (!data[i++]) {
        leading_zeros++;
    }

    int value = 0;
    for (int j = 0; j < leading_zeros; j++) {
        value <<= 1;
        value |= data[i++];
    }
    value |= 1 << leading_zeros;

    *index = i;

    return value - 1;
}

int read_bit(unsigned char *data, int *i) {
    int byte_offset = *i / 8;
    int bit_offset = 7 - (*i % 8);
    int bit = (data[byte_offset] >> bit_offset) & 1;
    (*i)++;
    return bit;
}

int read_bits(unsigned char *data, int *i, int n) {
    int value = 0;
    for (int j = 0; j < n; j++) {
        value = (value << 1) | read_bit(data, i);
    }
    return value;
}

void parse_sps(unsigned char *sps_data, int sps_size, int *width, int *height) {
    int i = 0;
    int profile_idc, level_idc, chroma_format_idc, bit_depth_luma_minus8, bit_depth_chroma_minus8, qpprime_y_zero_transform_bypass_flag, seq_scaling_matrix_present_flag;
    int seq_parameter_set_id, log2_max_frame_num_minus4, pic_order_cnt_type, log2_max_pic_order_cnt_lsb_minus4, delta_pic_order_always_zero_flag, frame_mbs_only_flag;
    int pic_width_in_mbs_minus1, pic_height_in_map_units_minus1, frame_crop_left_offset, frame_crop_right_offset, frame_crop_top_offset, frame_crop_bottom_offset;
    int vui_parameters_present_flag, aspect_ratio_info_present_flag, aspect_ratio_idc, sar_width, sar_height, overscan_info_present_flag, video_signal_type_present_flag;
    int chroma_loc_info_present_flag, chroma_sample_loc_type_top_field, chroma_sample_loc_type_bottom_field, timing_info_present_flag, num_units_in_tick, time_scale;
    int fixed_frame_rate_flag, nal_hrd_parameters_present_flag, vcl_hrd_parameters_present_flag, low_delay_hrd_flag, pic_struct_present_flag, bitstream_restriction_flag;
    int motion_vectors_over_pic_boundaries_flag, max_bytes_per_pic_denom, max_bits_per_mb_denom, log2_max_mv_length_horizontal, log2_max_mv_length_vertical, num_reorder_frames;
    int max_dec_frame_buffering;

    profile_idc = sps_data[1];
    level_idc = sps_data[3];
    seq_parameter_set_id = read_ue(sps_data + 4, &i);
    if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128 || profile_idc == 138 || profile_idc == 139 || profile_idc == 134) {
        chroma_format_idc = read_ue(sps_data + 4, &i);
        if (chroma_format_idc == 3) {
            bit_depth_luma_minus8 = read_ue(sps_data + 4, &i);
            bit_depth_chroma_minus8 = read_ue(sps_data + 4, &i);
            qpprime_y_zero_transform_bypass_flag = read_bit(sps_data + 4, &i);
            seq_scaling_matrix_present_flag = read_bit(sps_data + 4, &i);
            if (seq_scaling_matrix_present_flag) {
                for (int j = 0; j < 8; j++) {
                    if (read_bit(sps_data + 4, &i)) {
                        // TODO: read scaling list
                    }
                }
            }
        }
    }
    log2_max_frame_num_minus4 = read_ue(sps_data + 4, &i);
    pic_order_cnt_type = read_ue(sps_data + 4, &i);
    if (pic_order_cnt_type == 0) {
        log2_max_pic_order_cnt_lsb_minus4 = read_ue(sps_data + 4, &i);
    } else if (pic_order_cnt_type == 1) {
        delta_pic_order_always_zero_flag = read_bit(sps_data + 4, &i);
        // TODO: read other fields
    }
    frame_mbs_only_flag = read_bit(sps_data + 4, &i);
    if (!frame_mbs_only_flag) {
        // TODO: read other fields
    }
    pic_width_in_mbs_minus1 = read_ue(sps_data + 4, &i);
    pic_height_in_map_units_minus1 = read_ue(sps_data + 4, &i);
    frame_crop_left_offset = read_ue(sps_data + 4, &i);
    frame_crop_right_offset = read_ue(sps_data + 4, &i);
    frame_crop_top_offset = read_ue(sps_data + 4, &i);
    frame_crop_bottom_offset = read_ue(sps_data + 4, &i);
    vui_parameters_present_flag = read_bit(sps_data + 4, &i);
    if (vui_parameters_present_flag) {
        aspect_ratio_info_present_flag = read_bit(sps_data + 4, &i);
        if (aspect_ratio_info_present_flag) {
            aspect_ratio_idc = read_ue(sps_data + 4, &i);
            if (aspect_ratio_idc == 255) {
                sar_width = read_ue(sps_data + 4, &i);
                sar_height = read_ue(sps_data + 4, &i);
            }
        }
        overscan_info_present_flag = read_bit(sps_data + 4, &i);
        if (overscan_info_present_flag) {
            // TODO: read other fields
        }
        video_signal_type_present_flag = read_bit(sps_data + 4, &i);
        if (video_signal_type_present_flag) {
            // TODO: read other fields
        }
        chroma_loc_info_present_flag = read_bit(sps_data + 4, &i);
        if (chroma_loc_info_present_flag) {
            // TODO: read other fields
        }
        timing_info_present_flag = read_bit(sps_data + 4, &i);
        if (timing_info_present_flag) {
            num_units_in_tick = read_bits(sps_data + 4, &i, 32);
            time_scale = read_bits(sps_data + 4, &i, 32);
            fixed_frame_rate_flag = read_bit(sps_data + 4, &i);
            if (fixed_frame_rate_flag) {
                // TODO: read other fields
            }
        }
        nal_hrd_parameters_present_flag = read_bit(sps_data + 4, &i);
        if (nal_hrd_parameters_present_flag) {
            // TODO: read other fields
        }
        vcl_hrd_parameters_present_flag = read_bit(sps_data + 4, &i);
        if (vcl_hrd_parameters_present_flag) {
            // TODO: read other fields
        }
        if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
            low_delay_hrd_flag = read_bit(sps_data + 4, &i);
        }
        pic_struct_present_flag = read_bit(sps_data + 4, &i);
        if (pic_struct_present_flag) {
            // TODO: read other fields
        }
        bitstream_restriction_flag = read_bit(sps_data + 4, &i);
        if (bitstream_restriction_flag) {
            motion_vectors_over_pic_boundaries_flag = read_bit(sps_data + 4, &i);
            max_bytes_per_pic_denom = read_ue(sps_data + 4, &i);
            max_bits_per_mb_denom = read_ue(sps_data + 4, &i);
            log2_max_mv_length_horizontal = read_ue(sps_data + 4, &i);
            log2_max_mv_length_vertical = read_ue(sps_data + 4, &i);
            num_reorder_frames = read_ue(sps_data + 4, &i);
            max_dec_frame_buffering = read_ue(sps_data + 4, &i);
        }
    }

    // Calculate the width and height of the video frame
    int pic_width_in_mbs = pic_width_in_mbs_minus1 + 1;
    int pic_height_in_map_units = pic_height_in_map_units_minus1 + 1;
    int frame_crop_left = frame_crop_left_offset * 2;
    int frame_crop_right = frame_crop_right_offset * 2;
    int frame_crop_top = frame_crop_top_offset * 2;
    int frame_crop_bottom = frame_crop_bottom_offset * 2;
    int crop_unit_x = 1;
    int crop_unit_y = 2 - frame_mbs_only_flag;
    int width_in_pixels = (pic_width_in_mbs * 16 - frame_crop_left - frame_crop_right) * crop_unit_x;
    int height_in_pixels = (pic_height_in_map_units * 16 - frame_crop_top - frame_crop_bottom) * crop_unit_y;

    // Set the output parameters
    *width = width_in_pixels;
    *height = height_in_pixels;
}



void parse_pps(unsigned char *pps_data, int pps_size, int *slice_group_count) {
    int i = 0;
    int pic_parameter_set_id, seq_parameter_set_id, entropy_coding_mode_flag, bottom_field_pic_order_in_frame_present_flag;
    int num_slice_groups_minus1, slice_group_map_type, slice_group_change_rate_minus1;

    // Skip the first byte (for NAL unit header)
    i++;

    // Parse the pic_parameter_set_id and seq_parameter_set_id
    pic_parameter_set_id = read_ue(pps_data, &i);
    seq_parameter_set_id = read_ue(pps_data, &i);

    // Parse the entropy_coding_mode_flag
    entropy_coding_mode_flag = pps_data[i++];

    // Parse the bottom_field_pic_order_in_frame_present_flag
    bottom_field_pic_order_in_frame_present_flag = pps_data[i++];

    // Parse the num_slice_groups_minus1 and slice_group_map_type
    num_slice_groups_minus1 = read_ue(pps_data, &i);
    if (num_slice_groups_minus1 > 0) {
        slice_group_map_type = read_ue(pps_data, &i);
        if (slice_group_map_type == 0) {
            for (int j = 0; j <= num_slice_groups_minus1; j++) {
                read_ue(pps_data, &i); // Run length
            }
        } else if (slice_group_map_type == 2) {
            for (int j = 0; j < num_slice_groups_minus1; j++) {
                read_ue(pps_data, &i); // Top left
                read_ue(pps_data, &i); // Bottom right
            }
        } else if (slice_group_map_type == 3 || slice_group_map_type == 4 || slice_group_map_type == 5) {
            i++; // Slice group change direction flag
            slice_group_change_rate_minus1 = read_ue(pps_data, &i);
        } else if (slice_group_map_type == 6) {
            int pic_size_in_map_units_minus1 = read_ue(pps_data, &i);
            for (int j = 0; j <= pic_size_in_map_units_minus1; j++) {
                read_ue(pps_data, &i); // Slice group ID
            }
        }
    }

    // Store the slice group count
    *slice_group_count = num_slice_groups_minus1 + 1;
}

