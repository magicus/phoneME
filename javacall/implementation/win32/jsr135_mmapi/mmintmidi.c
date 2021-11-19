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
#include <windows.h>

/**********************************************************************************/

typedef struct javacall_interactive_midi_s {
    HMIDIOUT hmOut;
} javacall_interactive_midi_s;

/**
 * 
 */
static javacall_handle interactive_midi_create(javacall_int64 playerId, 
                                               javacall_media_type mediaType, 
                                               const javacall_utf16* URI, 
                                               long contentLength)
{
    javacall_interactive_midi_s* pIM = 
        javacall_malloc(sizeof(javacall_interactive_midi_s));
    if (pIM != NULL) {
        javacall_result res;
        pIM->hmOut = NULL;
        res = javacall_open_midi_out(&(pIM->hmOut), JAVACALL_TRUE);
        if (!JAVACALL_SUCCEEDED(res)) {
            javacall_free(pIM);
            pIM = NULL;
        }
    }
    return (javacall_handle)pIM;
}

/**
 * 
 */
static javacall_result interactive_midi_close(javacall_handle handle)
{
    if (handle) {
        javacall_interactive_midi_s* pIM = (javacall_interactive_midi_s*)handle;
        javacall_close_midi_out(&pIM->hmOut);
    }
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result interactive_midi_destroy(javacall_handle handle)
{
    if (handle) {
        interactive_midi_close(handle);
        javacall_free(handle);
    }
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result interactive_midi_acquire_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result interactive_midi_release_device(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result interactive_midi_start(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result interactive_midi_stop(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result interactive_midi_pause(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static javacall_result interactive_midi_resume(javacall_handle handle)
{
    return JAVACALL_OK;
}

/**
 * 
 */
static long interactive_midi_get_time(javacall_handle handle)
{
    return -1;
}

/**
 * 
 */
static long interactive_midi_set_time(javacall_handle handle, long ms)
{
    return -1;
}
 
/**
 * 
 */
static long interactive_midi_get_duration(javacall_handle handle)
{
    return -1;
}

/**********************************************************************************/

/**
 * 
 */
static
javacall_result interactive_midi_get_channel_volume(javacall_handle handle, 
                                                    long channel, long* volume) {
    return JAVACALL_FAIL;
}

/**
 * 
 */
static
javacall_result interactive_midi_set_channel_volume(javacall_handle handle, 
                                                    long channel, long volume) {
    return JAVACALL_FAIL;
}

/**
 * 
 */
static
javacall_result interactive_midi_set_program(javacall_handle handle, 
                                             long channel, long bank, long program) {
    return JAVACALL_FAIL;
}

/**
 * 
 */
static
javacall_result interactive_midi_short_midi_event(javacall_handle handle, 
                                                  long type, long data1, long data2) 
{
    javacall_interactive_midi_s* pIM = (javacall_interactive_midi_s*)handle;
    javacall_result ret = JAVACALL_FAIL;

    if ((pIM) && (pIM->hmOut)) {
        union { 
            DWORD dwData; 
            BYTE bData[4]; 
        } u; 

        u.bData[0] = (BYTE)type;
        u.bData[1] = (BYTE)data1;
        u.bData[2] = (BYTE)data2;
        u.bData[3] = 0; 
        
        midiOutShortMsg(pIM->hmOut, u.dwData);

        ret = JAVACALL_OK;
    }

    return ret;
}

/**
 * 
 */
static
javacall_result interactive_midi_long_midi_event(javacall_handle handle, 
                                                 const char* data, long offset, 
                                                 long* length) {
    return JAVACALL_FAIL;
}

/* VolumeControl Functions ************************************************/

/**
 * Get the current volume level set.
 * 
 * @param handle player handle
 * @return the current volume level or <code>-1</code>.
 */
static long interactive_midi_get_volume(javacall_handle handle) {
    return 0;
}

/**
 * Set the volume level using a linear point scale
 * with values between 0 and 100.
 * 
 * @param handle player handle
 * @param level the new volume specified in the level scale.
 * @return the level that was actually set.
 */
static long interactive_midi_set_volume(javacall_handle handle, long level) {
    return 0;
}

/**
 * Get the mute state of the signal
 * @return <code>JAVACALL_TRUE</code> if signal is mute,
 * <code>JAVACALL_FALSE</code> if signal is not mute.
 */
static javacall_bool interactive_midi_is_mute(javacall_handle handle) {
    return JAVACALL_FALSE;
}

/**
 * Mute or unmute the player
 * 
 * @param handle player handle
 * @param mute specify <code>true</code> to mute the signal,
 * <code>false</code> to unmute the signal
 * @return <code>JAVACALL_OK</code> on success
 */
static javacall_result interactive_midi_set_mute(javacall_handle handle, 
                                      javacall_bool mute) {
    return JAVACALL_OK;
}

/**********************************************************************************/

/**
 * Interactive MIDI basic javacall function interface
 */
static media_basic_interface _basic_itf = {
    interactive_midi_create,
    interactive_midi_close,
    interactive_midi_destroy,
    interactive_midi_acquire_device,
    interactive_midi_release_device,
    interactive_midi_start,
    interactive_midi_stop,
    interactive_midi_pause,
    interactive_midi_resume,
    NULL,
    NULL,
    interactive_midi_get_time,
    interactive_midi_set_time,
    interactive_midi_get_duration
};

/**
 * Interactive MIDI interactive_midi javacall function interface
 */
static media_midi_interface _interactive_midi_itf = {
    interactive_midi_get_channel_volume,
    interactive_midi_set_channel_volume,
    interactive_midi_set_program,
    interactive_midi_short_midi_event,
    interactive_midi_long_midi_event
};

/**
 * Interactive MIDI volume javacall function interface
 */
static media_volume_interface _interactive_midi_volume_itf = {
    interactive_midi_get_volume,
    interactive_midi_set_volume,
    interactive_midi_is_mute,
    interactive_midi_set_mute
};
/**********************************************************************************/
 
/* Global interactive_midi interface */
media_interface g_interactive_midi_itf = {
    &_basic_itf,
    &_interactive_midi_volume_itf,
    NULL,
    NULL,
    &_interactive_midi_itf
}; 

 

