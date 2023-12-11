//
//  ParseFLV.c
//  MediaInfo
//
//  Created by mysong on 2023/3/29.
//

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned char signature[3];
    unsigned char version;
    unsigned char flags;
    unsigned int header_size;
} FLVHeader;

typedef struct {
    unsigned char type;
    unsigned int data_size;
    unsigned int timestamp;
    unsigned char timestamp_extended;
    unsigned int stream_id;
} FLVTagHeader;

typedef struct {
    unsigned char frame_type;
    unsigned char codec_id;
    unsigned int composition_time;
} FLVVideoTagHeader;

typedef struct {
    unsigned char format;
    unsigned char rate;
    unsigned char size;
    unsigned char type;
} FLVAudioTagHeader;

typedef struct {
    unsigned char frame_type;
    unsigned char codec_id;
    unsigned int composition_time;
    unsigned char video_data[5];
} FLVVideoTagHeaderExtended;

int test(int argc, char *argv[]) {
    FILE *fp;
    FLVHeader header;
    FLVTagHeader tag_header;
    FLVVideoTagHeader video_tag_header;
    FLVAudioTagHeader audio_tag_header;
    FLVVideoTagHeaderExtended video_tag_header_extended;
    unsigned char tag_type;
    unsigned int tag_data_size;
    unsigned int tag_timestamp;
    unsigned char tag_timestamp_extended;
    unsigned int tag_stream_id;
    unsigned char video_frame_type;
    unsigned char video_codec_id;
    unsigned int video_composition_time;
    unsigned char video_data[5];
    unsigned int video_width;
    unsigned int video_height;
    unsigned int frame_offset = 0;
    unsigned int frame_size = 0;
    unsigned int dts = 0;
    unsigned int pts = 0;
    
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("Error: could not open file %s\n", argv[1]);
        return 1;
    }
    
    fread(&header, sizeof(header), 1, fp);
    
    printf("Signature: %c%c%c\n", header.signature[0], header.signature[1], header.signature[2]);
    printf("Version: %d\n", header.version);
    printf("Flags: %d\n", header.flags);
    printf("Header Size: %d\n", header.header_size);
    
    while (!feof(fp)) {
        fread(&tag_type, sizeof(tag_type), 1, fp);
        fread(&tag_data_size, sizeof(tag_data_size), 1, fp);
        tag_data_size = (tag_data_size & 0x00FFFFFF);
        fread(&tag_timestamp, sizeof(tag_timestamp), 1, fp);
        fread(&tag_timestamp_extended, sizeof(tag_timestamp_extended), 1, fp);
        fread(&tag_stream_id, sizeof(tag_stream_id), 1, fp);
        printf("Tag Type: %d\n", tag_type);
        printf("Tag Data Size: %d\n", tag_data_size);
        printf("Tag Timestamp: %d\n", tag_timestamp);
        printf("Tag Timestamp Extended: %d\n", tag_timestamp_extended);
        printf("Tag Stream ID: %d\n", tag_stream_id);
        
        if (tag_type == 8) { // Audio Tag
            fread(&audio_tag_header, sizeof(audio_tag_header), 1, fp);
            fseek(fp, tag_data_size - sizeof(audio_tag_header), SEEK_CUR);
        } else if (tag_type == 9) { // Video Tag
            fread(&video_tag_header, sizeof(video_tag_header), 1, fp);
            video_frame_type = (video_tag_header.frame_type >> 4) & 0x0F;
            video_codec_id = video_tag_header.codec_id & 0x0F;
            video_composition_time = video_tag_header.composition_time;
            if (video_codec_id == 7) { // AVC Video
                fread(&video_tag_header_extended, sizeof(video_tag_header_extended), 1, fp);
                video_width = ((video_tag_header_extended.video_data[0] << 8) | video_tag_header_extended.video_data[1]);
                video_height = ((video_tag_header_extended.video_data[2] << 8) | video_tag_header_extended.video_data[3]);
                printf("Video Width: %d\n", video_width);
                printf("Video Height: %d\n", video_height);
                fseek(fp, tag_data_size - sizeof(video_tag_header) - sizeof(video_tag_header_extended), SEEK_CUR);
            } else {
                fseek(fp, tag_data_size - sizeof(video_tag_header), SEEK_CUR);
            }
        } else {
            fseek(fp, tag_data_size, SEEK_CUR);
        }
        
        if (tag_type == 9 && video_codec_id == 7) { // AVC Video
            frame_offset = ftell(fp);
            frame_size = tag_data_size - sizeof(video_tag_header) - sizeof(video_tag_header_extended);
            dts = tag_timestamp;
            pts = dts + video_composition_time;
            printf("Frame Offset: %d\n", frame_offset);
            printf("Frame Size: %d\n", frame_size);
            printf("DTS: %d\n", dts);
            printf("PTS: %d\n", pts);
            fseek(fp, frame_size, SEEK_CUR);
        }
    }
    
    fclose(fp);
    return 0;
}
