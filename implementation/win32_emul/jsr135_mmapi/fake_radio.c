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
 */ 

#include "multimedia.h"

typedef struct {
    unsigned char volume;
    javacall_bool is_mute;
} fake_radio_instance_t;

static javacall_result fake_radio_create(int appId, int playerId,
                                       jc_fmt mediaType,
                                       const javacall_utf16_string URI,
                                       javacall_handle *pHandle )
{
    fake_radio_instance_t *newHandle = NULL;
    newHandle = MALLOC( sizeof( fake_radio_instance_t ) );
    if( NULL == newHandle )
    {
        *pHandle = NULL;
        return JAVACALL_OUT_OF_MEMORY;
    }
    newHandle->volume = 50;
    newHandle->is_mute = JAVACALL_FALSE;
    *pHandle = ( javacall_handle )newHandle;
    return JAVACALL_OK;
}

static javacall_result fake_radio_get_format(javacall_handle handle, jc_fmt* fmt)
{
    *fmt = JC_FMT_CAPTURE_RADIO;
    return JAVACALL_OK;
}

static javacall_result fake_radio_get_player_controls(javacall_handle handle,
                                                    int* controls)
{
    *controls = JAVACALL_MEDIA_CTRL_VOLUME;
    return JAVACALL_OK;
}

static javacall_result fake_radio_close(javacall_handle handle){
    return JAVACALL_OK;
}

static javacall_result fake_radio_destroy(javacall_handle handle)
{
    if( NULL != handle )
    {
        FREE( handle );
    }
    return JAVACALL_OK;
}

static javacall_result fake_radio_acquire_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

static javacall_result fake_radio_release_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

static javacall_result fake_radio_start(javacall_handle handle){
    return JAVACALL_OK;
}

static javacall_result fake_radio_stop(javacall_handle handle){
    return JAVACALL_OK;
}

static javacall_result fake_radio_pause(javacall_handle handle){
    return JAVACALL_OK;
}

static javacall_result fake_radio_resume(javacall_handle handle){
    return JAVACALL_OK;
}

static javacall_result fake_radio_get_volume(javacall_handle handle, long* level)
{
    javacall_result res = JAVACALL_FAIL;
    if( NULL != handle )
    {
        fake_radio_instance_t *h = ( fake_radio_instance_t* )handle;
        *level = h->volume;
        res = JAVACALL_OK;
    }
    return res;
}

static javacall_result fake_radio_set_volume(javacall_handle handle, long* level)
{
    javacall_result res = JAVACALL_FAIL;
    if( NULL != handle )
    {
        fake_radio_instance_t *h = ( fake_radio_instance_t* )handle;
        long l = *level < 0 ? 0 : ( *level > 100 ? 100 : *level );
        h->volume = ( unsigned char )l;
        *level    = l;
        res = JAVACALL_OK;
    }
    return res;
}

static javacall_result fake_radio_is_mute( javacall_handle handle, 
                                         javacall_bool* mute ) {
    javacall_result res = JAVACALL_FAIL;
    if( NULL != handle )
    {
        fake_radio_instance_t *h = ( fake_radio_instance_t* )handle;
        *mute = h->is_mute;
        res = JAVACALL_OK;
    }
    return res;
}

static javacall_result fake_radio_set_mute(javacall_handle handle,
                                      javacall_bool mute){
    javacall_result res = JAVACALL_FAIL;
    if( NULL != handle )
    {
        fake_radio_instance_t *h = ( fake_radio_instance_t* )handle;
        h->is_mute = mute;
        res = JAVACALL_OK;
    }
    return res;
}

static media_basic_interface _fake_radio_basic_itf = {
    fake_radio_create,
    fake_radio_get_format,
    fake_radio_get_player_controls,
    fake_radio_close,
    fake_radio_destroy,
    fake_radio_acquire_device,
    fake_radio_release_device,
    NULL,
    NULL,
    fake_radio_start,
    fake_radio_stop,
    fake_radio_pause,
    fake_radio_resume,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static media_volume_interface _fake_radio_volume_itf = {
    fake_radio_get_volume,
    fake_radio_set_volume,
    fake_radio_is_mute,
    fake_radio_set_mute
};

media_interface g_fake_radio_itf = {
    &_fake_radio_basic_itf,
    &_fake_radio_volume_itf,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};