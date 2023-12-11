//
//  QuikTimeDefine.h
//  MP4File
//
//  Created by mysong on 15/3/27.
//  Copyright (c) 2015年 mysong. All rights reserved.
//

#ifndef MP4File_QuikTimeDefine_h
#define MP4File_QuikTimeDefine_h
#include "QuickTimeAtomTypeDefine.h"
#include <arpa/inet.h>

typedef struct _Mp4BoxHeader
{
    uint32_t size;
    uint64_t largeSize;
    uint64_t offset;
    char     type[4];
    
    uint32_t UIntType()  {
        return htonl(*(uint32_t *)(type));
    }
    uint64_t RealSize() {
        return largeSize > 0 ? largeSize : size;
    }
}Mp4BoxHeader;

typedef struct _Mp4FtypBox
{
    Mp4BoxHeader head;
    char majorBrand[4];//A 32-bit unsigned integer that represents a file format code.
    int32_t IntMajorBrand()  {
        return htonl(*(uint32_t *)(majorBrand));
    }
    uint32_t minorVersion; //A 32-bit field that indicates the file format specification version.
    char compatibleBrands[20]; //A series of unsigned 32-bit integers listing compatible file formats.
}Mp4FtypBox;


typedef struct _Mp4Mvhd
{
    //https://developer.apple.com/library/content/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html
    Mp4BoxHeader head;
    uint8_t  ver; //A 1-byte specification of the version of this movie header atom.
    char     flag[3]; //Three bytes of space for future movie header flags.
    uint32_t createTime; //Movie atom的起始时间。基准时间是1904-1-1 0:00 AM
    uint32_t modifyTime; //Movie atom的修订时间。基准时间是1904-1-1 0:00 AM
    
    uint32_t timeScale; //A time value that indicates the time scale for this movie—that is, the number of time units that pass per second in its time coordinate system. A time coordinate system that measures time in sixtieths of a second, for example, has a time scale of 60.
    
    uint32_t duraion; //A time value that indicates the duration of the movie in time scale units.Note that this property is derived from the movie’s tracks. The value of this field corresponds to the duration of the longest track in the movie.
    int32_t preferredRate; //A 32-bit fixed-point number that specifies the rate at which to play this movie. A value of 1.0 indicates normal rate.
    int16_t volume; //A 16-bit fixed-point number that specifies how loud to play this movie’s sound. A value of 1.0 indicates full volume.
    
    char    reserved[10];//Ten bytes reserved for use by Apple. Set to 0.
    
    //abucdvxyw
    // a b u
    // c d v
    // x y w
    //Byte transform[36];
    int32_t  transform[3][3]; //The matrix structure associated with this movie. A matrix shows how to map points from one coordinate space into another. See Matrices for a discussion of how display matrices are used in QuickTime.
    
    uint32_t previewTime; //The time value in the movie at which the preview begins.
    uint32_t previewDuration; //The duration of the movie preview in movie time scale units.
    
    uint32_t posterTime;  //The time value of the time of the movie poster.
    uint32_t selectionTime; //The time value for the start time of the current selection.
    uint32_t selectionDuration; //The time value for the start time of the current selection.
    uint32_t currentTime; //The time value for current time position within the movie.
    uint32_t nextTrackId; //A 32-bit integer that indicates a value to use for the track ID number of the next track added to this movie. Note that 0 is not a valid track ID value.
}Mp4MvhdBox;


typedef struct _MP4Tkhd
{
    Mp4BoxHeader head;
    
    /*
     * A 1-byte specification of the version of this movie header atom.
     */
    uint8_t  ver;
    
    /*
     Three bytes that are reserved for the track header flags. These flags indicate how the track is used in the movie. The following flags are valid (all flags are enabled when set to 1).
     Track enabled
     Indicates that the track is enabled. Flag value is 0x0001.
     Track in movie
     Indicates that the track is used in the movie. Flag value is 0x0002.
     Track in preview
     Indicates that the track is used in the movie’s preview. Flag value is 0x0004.
     Track in poster
     Indicates that the track is used in the movie’s poster. Flag value is 0x0008.
     */
    char     flag[3];
    
    /*
     A 32-bit integer that indicates the calendar date and time (expressed in seconds since midnight, January 1, 1904) when the track header was created. It is strongly recommended that this value should be specified using coordinated universal time (UTC).
     */
    uint32_t createTime;
    /*
     A 32-bit integer that indicates the calendar date and time (expressed in seconds since midnight, January 1, 1904) when the track header was changed. It is strongly recommended that this value should be specified using coordinated universal time (UTC).
     */
    uint32_t modifyTime; //Movie atom的修订时间。基准时间是1904-1-1 0:00 AM
    
    /*
     * A 32-bit integer that uniquely identifies the track. The value 0 cannot be used.
     */
    uint32_t trackId;
    
    /*
     * A 32-bit integer that is reserved for use by Apple. Set this field to 0.
     */
    uint32_t reserved;
    
    /*
     * A time value that indicates the duration of this track (in the movie’s time coordinate system). Note that this property is derived from the track’s edits. The value of this field is equal to the sum of the durations of all of the track’s edits. If there is no edit list, then the duration is the sum of the sample durations, converted into the movie timescale.
     */
    uint32_t duraion;
    
    /*
     * An 8-byte value that is reserved for use by Apple. Set this field to 0.
     */
    uint8_t   reserved2[8];
    
    /*
     * A 16-bit integer that indicates this track’s spatial priority in its movie. The QuickTime Movie Toolbox uses this value to determine how tracks overlay one another. Tracks with lower layer values are displayed in front of tracks with higher layer values.
     */
    int16_t layer;
    
    /*
     A 16-bit integer that identifies a collection of movie tracks that contain alternate data for one another. This same identifier appears in each 'tkhd' atom of the other tracks in the group. QuickTime chooses one track from the group to be used when the movie is played. The choice may be based on such considerations as playback quality, language, or the capabilities of the computer.
     A value of zero indicates that the track is not in an alternate track group.
     The most common reason for having alternate tracks is to provide versions of the same track in different languages. Figure 2-8 shows an example of several tracks. The video track’s Alternate Group ID is 0, which means that it is not in an alternate group (and its language codes are empty; normally, video tracks should have the appropriate language tags). The three sound tracks have the same Group ID, so they form one alternate group, and the subtitle tracks have a different Group ID, so they form another alternate group. The tracks would not be adjacent in an actual QuickTime file; this is just a list of example track field values.
     
     Example of alternate tracks in two alternate groups
     Track Type  |  Alternate Group Id | Extened Language Tag | Language Code
     Video(vide) |  0
     sound(soun) |  1                  | en-US                | eng
     sound       |  1                  | fr-FR                | fra
     sound       |  1                  | jp-JP                | jpn
     subtitle(subt)|2                  | en-US                | eng
     subtitile   |  2                  | fr-FR                | fra
     */
    int16_t alternateGroup;
    
    /*
     * A 16-bit fixed-point value that indicates how loudly this track’s sound is to be played. A value of 1.0 indicates normal volume.
     */
    int16_t volume;
    
    /*
     * An 2-byte value that is reserved for use by Apple. Set this field to 0.
     */
    int16_t   reserved3;
    
    int32_t  matrix[3][3];
    
    /*
     * A 32-bit fixed-point number that specifies the width of this track in pixels.
     */
    int32_t  trackWidth;
    
    /*
     * A 32-bit fixed-point number that specifies the height of this track in pixels.
     */
    int32_t  trackHeight;
}MP4TkhdBox;

#endif
