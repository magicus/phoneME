/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 *
 * This software is provided "AS IS," without a warranty of any kind. ALL
 * EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES, INCLUDING
 * ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED. SUN AND ITS LICENSORS SHALL NOT
 * BE LIABLE FOR ANY DAMAGES OR LIABILITIES SUFFERED BY LICENSEE AS A RESULT
 * OF OR RELATING TO USE, MODIFICATION OR DISTRIBUTION OF THE SOFTWARE OR ITS
 * DERIVATIVES. IN NO EVENT WILL SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST
 * REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL,
 * INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY
 * OF LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE SOFTWARE, EVEN
 * IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 */ 

#ifndef __PORTING_MULTIMEDIA_H
#define __PORTING_MULTIMEDIA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <windows.h>
#include <mmsystem.h>
#include <vfw.h>
#include <string.h>  /* memcpy, memset */
#include <stdlib.h>  /* malloc */
#include <stdio.h>
#include <tchar.h>

#include "javacall_multimedia.h"
#include "javanotify_multimedia.h"
#include "mmdebug.h"

/*****************************************************************************
 *                    M E M O R Y   A L L O C A T I O N
 *****************************************************************************/

#define MALLOC(_size_)            malloc((_size_))
#define REALLOC(_ptr_, _size_)    realloc((_ptr_), (_size_))
#define FREE(_ptr_)               free((_ptr_))

#define MAX_MIMETYPE_LENGTH       0xFF
    
/*****************************************************************************
 *         I N T E R N A L   F O R M A T   P R E S E N T A T I O N
 *****************************************************************************/

typedef enum _jc_fmt {
    JC_FMT_MPEG1_LAYER2 = 0  ,
    JC_FMT_MPEG1_LAYER3      ,
    JC_FMT_MPEG1_LAYER3_PRO  ,
    JC_FMT_MPEG2_AAC         ,
    JC_FMT_MPEG4_HE_AAC      ,
    JC_FMT_ENHANCED_AAC_PLUS ,
    JC_FMT_AMR               ,
    JC_FMT_AMR_WB            ,
    JC_FMT_AMR_WB_PLUS       ,
    JC_FMT_GSM               ,
    JC_FMT_GSM_EFR           ,
    JC_FMT_QCELP             ,
    JC_FMT_MIDI              ,
    JC_FMT_SP_MIDI           ,
    JC_FMT_TONE              ,
    JC_FMT_MS_PCM            ,
    JC_FMT_MS_ADPCM          ,
    JC_FMT_YAMAHA_ADPCM      ,
    JC_FMT_AU                ,
    JC_FMT_OGG_VORBIS        ,
    JC_FMT_REALAUDIO_8       ,
    JC_FMT_AIFF              ,
    JC_FMT_WMA_9             ,
    JC_FMT_MJPEG_DEFAULT     ,
    JC_FMT_H263              ,
    JC_FMT_H264              ,
    JC_FMT_MPEG_1            ,
    JC_FMT_MPEG_2            ,
    JC_FMT_MPEG_4_SVP        ,
    JC_FMT_MPEG_4_AVC        ,
    JC_FMT_REALVIDEO_8       ,
    JC_FMT_WMV_9             ,
    JC_FMT_AUDIO_3GPP        ,
    JC_FMT_VIDEO_3GPP        ,
    JC_FMT_AVI               ,
    JC_FMT_MOV               ,
    JC_FMT_FLV               ,
    JC_FMT_JPEG              ,
    JC_FMT_JPEG2000          ,
    JC_FMT_TIFF              ,
    JC_FMT_PNG               ,
    JC_FMT_GIF               ,
    JC_FMT_RGB888            ,
    JC_FMT_RGBA8888          ,
    JC_FMT_GRAY1             ,
    JC_FMT_GRAY8             ,
    JC_FMT_DEVICE_TONE       ,
    JC_FMT_DEVICE_MIDI       ,
    JC_FMT_CAPTURE_AUDIO     ,
    JC_FMT_CAPTURE_RADIO     ,
    JC_FMT_CAPTURE_VIDEO     ,
    JC_FMT_RTP_DVI4          ,      
    JC_FMT_RTP_G722          ,
    JC_FMT_RTP_G723          ,
    JC_FMT_RTP_G726_16       ,
    JC_FMT_RTP_G726_24       ,
    JC_FMT_RTP_G726_32       ,
    JC_FMT_RTP_G726_40       ,
    JC_FMT_RTP_G728          ,
    JC_FMT_RTP_G729          ,
    JC_FMT_RTP_G729D         ,
    JC_FMT_RTP_G729E         ,
    JC_FMT_RTP_GSM           ,
    JC_FMT_RTP_GSM_EFR       ,
    JC_FMT_RTP_L8            ,
    JC_FMT_RTP_L16           ,
    JC_FMT_RTP_LPC           ,
    JC_FMT_RTP_MPA           ,
    JC_FMT_RTP_PCMA          ,
    JC_FMT_RTP_PCMU          ,
    JC_FMT_RTP_QCELP         ,
    JC_FMT_RTP_RED           ,
    JC_FMT_RTP_VDVI          ,
    JC_FMT_RTP_BT656         ,
    JC_FMT_RTP_CELB          ,
    JC_FMT_RTP_JPEG          ,
    JC_FMT_RTP_H261          ,
    JC_FMT_RTP_H263          ,
    JC_FMT_RTP_H263_1998     ,
    JC_FMT_RTP_H263_2000     ,
    JC_FMT_RTP_MPV           ,
    JC_FMT_RTP_MP2T          ,
    JC_FMT_RTP_MP1S          ,
    JC_FMT_RTP_MP2P          ,
    JC_FMT_RTP_BMPEG         ,
    JC_FMT_RTP_NV            ,
    /* JC_FMT_UNKNOWN excluded, it will be mapped to -1 */
    JC_FMT_UNSUPPORTED       ,

    JC_FMT_UNKNOWN = -1
} jc_fmt;

jc_fmt                     fmt_str2enum( javacall_media_format_type fmt );
javacall_media_format_type fmt_enum2str( jc_fmt                     fmt );
javacall_result            fmt_str2mime(
        javacall_media_format_type fmt, char *buf, int buf_len);

javacall_result get_int_param(javacall_const_utf16_string ptr, 
                              javacall_const_utf16_string paramName, 
                              int * value);

/*****************************************************************************
 *                     M E T A D A T A   K E Y S
 *****************************************************************************/

#define METADATA_KEY_AUTHOR    L"author"
#define METADATA_KEY_COPYRIGHT L"copyright"
#define METADATA_KEY_DATE      L"date"
#define METADATA_KEY_TITLE     L"title"
#define METADATA_KEY_COMMENT   L"comment"
#define METADATA_KEY_SOFTWARE  L"software"

/*****************************************************************************
 *                  M A I N   W I N D O W   A C C E S S
 *****************************************************************************/

#define GET_MCIWND_HWND()         (midpGetWindowHandle())

extern HWND midpGetWindowHandle();

/*****************************************************************************/

/*****************************************************************************
 *                      S T A T U S   M E S S A G E
 *****************************************************************************/

void mmSetStatusLine( const char* fmt, ... );

/*****************************************************************************/

/**
 * Win32 native player's handle information
 */
typedef struct {
    HWND                hWnd;
    javacall_int64      playerId;
    UINT                timerId;
    long                offset;
    jc_fmt              mediaType;
    TCHAR               fileName[MAX_PATH];
} win32_native_info;

/**
 * function pointer vector table for basic media functions
 */
typedef struct {
    javacall_result (*create)(int appId, int playerId, jc_fmt mediaType,
                              const javacall_utf16_string URI,
                              /*OUT*/ javacall_handle *pHandle);
    javacall_result (*get_format)(javacall_handle handle, jc_fmt* fmt);
    javacall_result (*get_player_controls)(javacall_handle handle, int* controls);
    javacall_result (*close)(javacall_handle handle);
    javacall_result (*destroy)(javacall_handle handle);
    javacall_result (*acquire_device)(javacall_handle handle);
    javacall_result (*release_device)(javacall_handle handle);
    javacall_result (*realize)(javacall_handle handle, javacall_const_utf16_string mime, long mimeLength);
    javacall_result (*prefetch)(javacall_handle handle);
    javacall_result (*start)(javacall_handle handle);
    javacall_result (*stop)(javacall_handle handle);
    javacall_result (*pause)(javacall_handle handle);
    javacall_result (*resume)(javacall_handle handle);
    javacall_result (*get_java_buffer_size)(javacall_handle handle,long* java_buffer_size,long* first_data_size);
    javacall_result (*set_whole_content_size)(javacall_handle handle, long whole_content_size);
    javacall_result (*get_buffer_address)(javacall_handle handle,const void** buffer,long* max_size);
    javacall_result (*do_buffering)(javacall_handle handle, const void* buffer, long *length, javacall_bool *need_more_data, long *min_data_size);
    javacall_result (*clear_buffer)(javacall_handle handle);
    javacall_result (*get_time)(javacall_handle handle, long* ms);
    javacall_result (*set_time)(javacall_handle handle, long* ms);
    javacall_result (*get_duration)(javacall_handle handle, long* ms);
    javacall_result (*switch_to_foreground)(javacall_handle handle, int options);
    javacall_result (*switch_to_background)(javacall_handle handle, int options);
} media_basic_interface;

/**
 * function pointer vector table for volume control
 */
typedef struct {
    javacall_result (*get_volume)(javacall_handle handle, long* level); 
    javacall_result (*set_volume)(javacall_handle handle, long* level);
    javacall_result (*is_mute)(javacall_handle handle, javacall_bool* mute);
    javacall_result (*set_mute)(javacall_handle handle, javacall_bool mute);
} media_volume_interface;

/**
 * function pointer vector table for MIDI control
 */
typedef struct {
    javacall_result (*get_channel_volume)(javacall_handle handle, long channel, long* volume); 
    javacall_result (*set_channel_volume)(javacall_handle handle, long channel, long volume);
    javacall_result (*set_program)(javacall_handle handle, long channel, long bank, long program);
    javacall_result (*short_midi_event)(javacall_handle handle, long type, long data1, long data2);
    javacall_result (*long_midi_event)(javacall_handle handle, const char* data, long offset, long* length);

    javacall_result (*is_bank_query_supported)(javacall_handle handle, long* supported);
    javacall_result (*get_bank_list)(javacall_handle handle, long custom, short* banklist, long* numlist);
    javacall_result (*get_key_name)(javacall_handle handle, long bank, long program, long key, char* keyname, long* keynameLen);
    javacall_result (*get_program_name)(javacall_handle handle, long bank, long program, char* progname, long* prognameLen);
    javacall_result (*get_program_list)(javacall_handle handle, long bank, char* proglist, long* proglistLen);
    javacall_result (*get_program)(javacall_handle, long channel, long* prog);

} media_midi_interface;

/**
 * function pointer vector table for MetaData control
 */
typedef struct {
    javacall_result (*get_metadata_key_counts)(javacall_handle handle, long* keyCounts); 
    javacall_result (*get_metadata_key)(javacall_handle handle, long index, long bufLength, javacall_utf16* buf);
    javacall_result (*get_metadata)(javacall_handle handle, javacall_const_utf16_string key, long bufLength, javacall_utf16* buf);
} media_metadata_interface;

/**
 * function pointer vector table for Rate control
 */
typedef struct {
    javacall_result (*get_max_rate)(javacall_handle handle, long* maxRate); 
    javacall_result (*get_min_rate)(javacall_handle handle, long* minRate);
    javacall_result (*set_rate)(javacall_handle handle, long rate);
    javacall_result (*get_rate)(javacall_handle handle, long* rate);
} media_rate_interface;

/**
 * function pointer vector table for Tempo control
 */
typedef struct {
    javacall_result (*get_tempo)(javacall_handle handle, long* tempo); 
    javacall_result (*set_tempo)(javacall_handle handle, long tempo);
} media_tempo_interface;

/**
 * function pointer vector table for Pitch control
 */
typedef struct {
    javacall_result (*get_max_pitch)(javacall_handle handle, long* maxPitch); 
    javacall_result (*get_min_pitch)(javacall_handle handle, long* minPitch);
    javacall_result (*set_pitch)(javacall_handle handle, long pitch);
    javacall_result (*get_pitch)(javacall_handle handle, long* pitch);
} media_pitch_interface;


/**
 * function pointer vector table for video playing
 */
typedef struct {
    javacall_result (*get_video_size)(javacall_handle handle, long* width, long* height);
    javacall_result (*set_video_visible)(javacall_handle handle, javacall_bool visible);
    javacall_result (*set_video_location)(javacall_handle handle, long x, long y, long w, long h);
    javacall_result (*set_video_alpha)(javacall_handle handle, javacall_bool on, javacall_pixel color);
    javacall_result (*set_video_fullscreenmode)(javacall_handle handle, javacall_bool fullScreenMode);
} media_video_interface;

/**
 * function pointer vector table for frame position control
 */
typedef struct {
    javacall_result (*map_frame_to_time)(javacall_handle handle, long frameNum, long* ms);
    javacall_result (*map_time_to_frame)(javacall_handle handle, long ms, long* frameNum);
    javacall_result (*seek_to_frame)(javacall_handle handle, long frameNum, long* actualFrameNum);
    javacall_result (*skip_frames)(javacall_handle handle, long* nFrames);
} media_fposition_interface;

/**
 * function pointer vector table for video snapshot
 */
typedef struct {
    javacall_result (*start_video_snapshot)(javacall_handle handle, const javacall_utf16* imageType, long length);
    javacall_result (*get_video_snapshot_data_size)(javacall_handle handle, long* size);
    javacall_result (*get_video_snapshot_data)(javacall_handle handle, char* buffer, long size);
} media_snapshot_interface;

/**
 * function pointer to vector table for record control
 */
typedef struct {
    javacall_result (*set_recordsize_limit)(javacall_handle handle, /*INOUT*/ long* size);
    javacall_result (*recording_handled_by_native)(javacall_handle handle, const javacall_utf16* locator);
    javacall_result (*start_recording)(javacall_handle handle);
    javacall_result (*pause_recording)(javacall_handle handle);
    javacall_result (*stop_recording)(javacall_handle handle);
    javacall_result (*reset_recording)(javacall_handle handle);
    javacall_result (*commit_recording)(javacall_handle handle);
    javacall_result (*get_recorded_data_size)(javacall_handle handle, /*OUT*/ long* size);
    javacall_result (*get_recorded_data)(javacall_handle handle, /*OUT*/ char* buffer, long offset, long size);
    javacall_result (*close_recording)(javacall_handle handle);
} media_record_interface;

/**
 * Interface to javacall implementation
 */
typedef struct {
    media_basic_interface*      vptrBasic;
    media_volume_interface*     vptrVolume;
    media_video_interface*      vptrVideo;
    media_snapshot_interface*   vptrSnapshot;
    media_midi_interface*       vptrMidi;
    media_metadata_interface*   vptrMetaData;
    media_rate_interface*       vptrRate;
    media_tempo_interface*      vptrTempo;
    media_pitch_interface*      vptrPitch;
    media_record_interface*     vptrRecord;
    media_fposition_interface*  vptrFposition;
} media_interface;

typedef struct {
    int                        appId;
    int                        playerId;
    javacall_utf16_string      uri;
    javacall_media_format_type mediaType;
    javacall_handle            mediaHandle;
    media_interface*           mediaItfPtr;
    javacall_bool              downloadByDevice;
} javacall_impl_player;

typedef struct {
    long                hWnd;
    int                 isolateId;
    int                 playerId;
    UINT                timerId;
    long                duration;
    long                curTime;
    long                offset;
    jc_fmt              mediaType;
    javacall_bool       isForeground;
    TCHAR               fileName[MAX_PATH * 2];
    long                volume;
    BOOL                mute;
    long                wholeContentSize;
    void *              buffer;
    javacall_bool       isBuffered;

#ifdef ENABLE_EXTRA_CAMERA_CONTROLS
    void *              pExtraCC;
#endif //ENABLE_EXTRA_CAMERA_CONTROLS
    void *              mutex;

} audio_handle;

#ifdef __cplusplus
}
#endif

#endif

