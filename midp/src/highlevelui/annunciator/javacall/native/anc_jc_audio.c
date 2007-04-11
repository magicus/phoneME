/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include <anc_audio.h>
#include <midp_logging.h>
#include <javacall_annunciator.h>

/**
 * @file
 *
 * Common file to hold simple audio implementation.
 */

/**
 * Simple sound playing implementation for Alert.
 * On most of the ports, play a beeping sound for types:
 * SOUND_WARNING, SOUND_ERROR and SOUND_ALARM.
 */
jboolean anc_play_sound(int soundType)
{
    javacall_audible_tone_type jatt;

    switch (soundType) {
    case SOUND_INFO:         jatt = JAVACALL_AUDIBLE_TONE_INFO;         break;
    case SOUND_WARNING:      jatt = JAVACALL_AUDIBLE_TONE_WARNING;      break;
    case SOUND_ERROR:        jatt = JAVACALL_AUDIBLE_TONE_ERROR;        break;
    case SOUND_ALARM:        jatt = JAVACALL_AUDIBLE_TONE_ALARM;        break;
    case SOUND_CONFIRMATION: jatt = JAVACALL_AUDIBLE_TONE_CONFIRMATION; break;
    default:
        return KNI_FALSE;
    };

    if (JAVACALL_SUCCEEDED(javacall_annunciator_play_audible_tone (jatt)))
        return KNI_TRUE;
    else
        return KNI_FALSE;
}
