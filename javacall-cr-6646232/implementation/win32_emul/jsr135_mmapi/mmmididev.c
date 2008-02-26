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
 */

#include "multimedia.h"
#include "mmmididev.h"

javacall_result javacall_open_midi_out(HMIDIOUT* pHmo, javacall_bool isSetFlute)
{
    javacall_result result = JAVACALL_FAIL;
    if (pHmo != NULL) {
        MMRESULT mmResult = midiOutOpen(pHmo, MIDI_MAPPER, 0, 0, 0);
        if (isSetFlute == JAVACALL_TRUE) {
            if (mmResult == MMSYSERR_NOERROR)
                mmResult = midiOutShortMsg(*pHmo, 0x00004bc0); /* set to flute */
            if (mmResult == MMSYSERR_NOERROR)
                mmResult = midiOutShortMsg(*pHmo, 0x00004bc1); /* set to flute */
        }
        if (mmResult == MMSYSERR_NOERROR)
            result = JAVACALL_OK;
    }
    return result;
}

javacall_result javacall_reset_midi_out(HMIDIOUT* pHmo) 
{
    javacall_result result = JAVACALL_FAIL;
    if (pHmo != NULL) {
        if (midiOutReset(*pHmo) == MMSYSERR_NOERROR)
            result = JAVACALL_OK;
    }
    return result;
}

javacall_result javacall_close_midi_out(HMIDIOUT* pHmo)
{
    javacall_result result = JAVACALL_FAIL;
    if (pHmo != NULL) {
        if (JAVACALL_SUCCEEDED(javacall_reset_midi_out(pHmo))) {
            MMRESULT mmResult = midiOutClose(*pHmo);
            if (mmResult == MMSYSERR_NOERROR)
                result = JAVACALL_OK;
        }
        *pHmo = NULL;
    }
    return result;
}

