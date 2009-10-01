/*
 *
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

#include <anc_audio.h>
#include <midp_logging.h>

/**
 * @file
 *
 * Common file to hold simple audio implementation.
 */

/**
 * Simple sound playing implementation for Alert.
 * On most of the ports, play a beeping sound for types:
 * ANC_SOUND_WARNING, ANC_SOUND_ERROR and ANC_SOUND_ALARM.
 */
jboolean anc_play_sound(AncSoundType soundType)
{
    REPORT_CALL_TRACE1(LC_HIGHUI, "LF:STUB:anc_play_sound(%d)\n", soundType);

    /* Suppress unused parameter warning */
    (void)soundType;

    /* Not yet implemented */
    return KNI_FALSE;
}
