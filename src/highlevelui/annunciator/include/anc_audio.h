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

#ifndef _ANC_AUDIO_H
#define _ANC_AUDIO_H

/**
 * @file
 * @ingroup highui_anc
 *
 * @brief Porting interface for playing Alert sound.
 *
 * ##include &lt;&gt;
 *
 * @{
 */
#include <kni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Possible sound types.
 */
typedef enum {
    ANC_SOUND_INFO          = 1, /**< Sound for informative alert */
    ANC_SOUND_WARNING       = 2, /**< Sound for warning alert */
    ANC_SOUND_ERROR         = 3, /**< Sound for error alert */
    ANC_SOUND_ALARM         = 4, /**< Sound for alarm alert */
    ANC_SOUND_CONFIRMATION  = 5  /**< Sound for confirmation alert */
} AncSoundType;

/**
 * Play a sound of the given type.
 *
 * @param type must be one of the constants from AncSoundType enumeration 
 * @return KNI_TRUE if a sound was actually emitted
 */
jboolean anc_play_sound(AncSoundType soundType);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* @} */

#endif /* _ANC_AUDIO_H */
