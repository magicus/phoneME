/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 */ 

#ifndef __PORTING_MULTIMEDIA_H
#define __PORTING_MULTIMEDIA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <stdio.h>
#include "javacall_multimedia.h"
#include "javacall_memory.h"
#include "lcd.h"
#include "mmmididev.h"

#undef JAVA_DEBUG_PRINT
#undef JAVA_DEBUG_PRINT1
#undef JAVA_DEBUG_PRINT2
#undef JAVA_DEBUG_PRINT3

#define JAVA_DEBUG_PRINT(x)              printf((x))
#define JAVA_DEBUG_PRINT1(x, a)          printf((x), (a))
#define JAVA_DEBUG_PRINT2(x, a, b)       printf((x), (a), (b))
#define JAVA_DEBUG_PRINT3(x, a, b, c)    printf((x), (a), (b), (c))
#define JAVA_DEBUG_PRINT4(x, a, b, c, d) printf((x), (a), (b), (c), (d))

/*
 * - From midp_lcdui_md.c file -
 * This (x,y) coordinate pair refers to the offset of the upper
 * left corner of the display screen within the MIDP phone handset
 * graphic window
 */ 
#define TOP_BAR_HEIGHT      (11)
#define X_SCREEN_OFFSET     (30)
#define Y_SCREEN_OFFSET     (131 + TOP_BAR_HEIGHT)

#define MALLOC(_size_)            malloc((_size_))
#define REALLOC(_ptr_, _size_)    realloc((_ptr_), (_size_))
#define FREE(_ptr_)               free((_ptr_))

#define GET_MCIWND_HWND()         (midpGetWindowHandle())

extern HWND midpGetWindowHandle();

/**
 * Win32 native player's handle information
 */
typedef struct {
    HWND                hWnd;
    javacall_int64      playerId;
    UINT                timerId;
    long                offset;
    javacall_media_type mediaType;
    TCHAR               fileName[MAX_PATH];
} win32_native_info;

/**
 * function pointer vector table for basic media functions
 */
typedef struct {
    javacall_handle (*create)(javacall_int64 playerId, javacall_media_type mediaType, const javacall_utf16* URI, long contentLength);
    javacall_result (*close)(javacall_handle handle);
    javacall_result (*destroy)(javacall_handle handle);
    javacall_result (*acquire_device)(javacall_handle handle);
    javacall_result (*release_device)(javacall_handle handle);
    javacall_result (*start)(javacall_handle handle);
    javacall_result (*stop)(javacall_handle handle);
    javacall_result (*pause)(javacall_handle handle);
    javacall_result (*resume)(javacall_handle handle);
    long            (*do_buffering)(javacall_handle handle, const void* buffer, long length, long offset);
    javacall_result (*clear_buffer)(javacall_handle handle);
    long            (*get_time)(javacall_handle handle);
    long            (*set_time)(javacall_handle handle, long ms);
    long            (*get_duration)(javacall_handle handle);
    javacall_result (*protocol_handled_by_device)(javacall_handle handle);
    javacall_result (*switch_to_foreground)(javacall_handle handle, int options);
    javacall_result (*switch_to_background)(javacall_handle handle, int options);
} media_basic_interface;

/**
 * function pointer vector table for volume control
 */
typedef struct {
    long            (*get_volume)(javacall_handle handle); 
    long            (*set_volume)(javacall_handle handle, long level);
    javacall_bool   (*is_mute)(javacall_handle handle);
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
    javacall_result (*get_metadata_key)(javacall_handle handle, long index, long bufLength, char* buf);
    javacall_result (*get_metadata)(javacall_handle handle, const char* key, long bufLength, char *buf);
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
} media_video_interface;

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
} media_interface;

#ifdef __cplusplus
}
#endif

#endif

