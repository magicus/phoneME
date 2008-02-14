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
 
#include "multimedia.h"

/**********************************************************************************/

/**
 * 
 */
static javacall_handle video_create(javacall_int64 playerId, 
                                    javacall_media_type mediaType, 
                                    const javacall_utf16* URI, 
                                    long contentLength)
{
    return (javacall_handle)1;
}

/**
 * 
 */
static javacall_result video_close(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_destroy(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_acquire_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_release_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_start(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_stop(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_pause(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_resume(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * Store media data to temp file (except JTS type)
 */
static long video_do_buffering(javacall_handle handle, const void* buffer, 
                        long length, long offset){
    return length;
}

static javacall_result video_clear_buffer(javacall_handle handle) {
    return JAVACALL_OK;
}

/**
 * 
 */
static long video_get_time(javacall_handle handle)
{
    return -1;
}

/**
 * 
 */
static long video_set_time(javacall_handle handle, long ms)
{
    return -1;
}
 
/**
 * 
 */
static long video_get_duration(javacall_handle handle)
{
    return -1;
}

/**********************************************************************************/

/**
 * 
 */
static javacall_result video_get_video_size(javacall_handle handle, 
                                            long* width, long* height)
{
    *width = 100;
    *height = 100;
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_set_video_visible(javacall_handle handle, 
                                               javacall_bool visible)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_set_video_location(javacall_handle handle, 
                                                long x, long y, long w, long h)
{
    return JAVACALL_OK;
}

/**********************************************************************************/

/**
 * 
 */
static javacall_result video_start_video_snapshot(javacall_handle handle, 
                                                  const javacall_utf16* imageType, 
                                                  long length)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_get_video_snapshot_data_size(javacall_handle handle, 
                                                          long* size)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result video_get_video_snapshot_data(javacall_handle handle, 
                                                     char* buffer, long size)
{
    return JAVACALL_OK;
}

/**********************************************************************************/

/**
 * Video basic javacall function interface
 */
static media_basic_interface _video_basic_itf = {
    video_create,
    video_close,
    video_destroy,
    video_acquire_device,
    video_release_device,
    video_start,
    video_stop,
    video_pause,
    video_resume,
    video_do_buffering,
    video_clear_buffer,
    video_get_time,
    video_set_time,
    video_get_duration
};

/**
 * Video video javacall function interface
 */
static media_video_interface _video_video_itf = {
    video_get_video_size,
    video_set_video_visible,
    video_set_video_location
};

/**
 * Video snapshot javacall function interface
 */
static media_snapshot_interface _video_snapshot_itf = {
    video_start_video_snapshot,
    video_get_video_snapshot_data_size,
    video_get_video_snapshot_data
};

/**********************************************************************************/
 
/* Global video interface */
media_interface g_video_itf = {
    &_video_basic_itf,
    NULL,
    &_video_video_itf,
    &_video_snapshot_itf
}; 

 
