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

#include "KNICommon.h"

/* MIDIControl **************************************************/

/*  private native void nSetChannelVolume ( int handle , int channel , int volume ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_mmedia_DirectMIDIControl_nSetChannelVolume() {

    jint handle = KNI_GetParameterAsInt(1);
    jint channel = KNI_GetParameterAsInt(2);
    jint volume = KNI_GetParameterAsInt(3);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        javacall_media_set_channel_volume(pKniInfo->pNativeHandle, 
                                          (long)channel, (long)volume);
    }

    KNI_ReturnVoid();
}

/*  private native int nGetChannelVolume ( int handle , int channel ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetChannelVolume() {

    jint handle = KNI_GetParameterAsInt(1);
    jint channel = KNI_GetParameterAsInt(2);
    jint returnValue = -1;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        long volume;

        if (JAVACALL_SUCCEEDED(
            javacall_media_get_channel_volume(pKniInfo->pNativeHandle, 
                                              (long)channel, &volume))
            ) {
            returnValue = (jint)volume;
        }
    }
    
    KNI_ReturnInt(returnValue);
}

/*  private native void nSetProgram ( int handle , int channel , int bank , int program ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_mmedia_DirectMIDIControl_nSetProgram() {

    jint handle = KNI_GetParameterAsInt(1);
    jint channel = KNI_GetParameterAsInt(2);
    jint bank = KNI_GetParameterAsInt(3);
    jint program = KNI_GetParameterAsInt(4);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    
    if (pKniInfo && pKniInfo->pNativeHandle) {
        javacall_media_set_program(pKniInfo->pNativeHandle, 
                                   (long)channel, (long)bank, (long)program);
    }

    KNI_ReturnVoid();
}

/*  private native void nShortMidiEvent ( int handle , int type , int data1 , int data2 ) ; */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_mmedia_DirectMIDIControl_nShortMidiEvent() {

    jint handle = KNI_GetParameterAsInt(1);
    jint type = KNI_GetParameterAsInt(2);
    jint data1 = KNI_GetParameterAsInt(3);
    jint data2 = KNI_GetParameterAsInt(4);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        javacall_media_short_midi_event(pKniInfo->pNativeHandle, 
                                        (long)type, (long)data1, (long)data2);
    }

    KNI_ReturnVoid();
}

/*  private native int nLongMidiEvent ( int handle , byte [ ] data , int offset , int length ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nLongMidiEvent() {

    jint handle = KNI_GetParameterAsInt(1);
    jint offset = KNI_GetParameterAsInt(3);
    jint length = KNI_GetParameterAsInt(4);
    jint returnValue = -1;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(dataHandle);
    KNI_GetParameterAsObject(2, dataHandle);
    
    if (pKniInfo && pKniInfo->pNativeHandle && length > 0) {
        char* pBuf = (char*)MMP_MALLOC((int)length);
        long inoutLength = length;

        if (pBuf) {
            KNI_GetRawArrayRegion(dataHandle, 0, (jsize)length, (void*)pBuf);
        }
        if (JAVACALL_SUCCEEDED(
            javacall_media_long_midi_event(pKniInfo->pNativeHandle, 
                                           pBuf, (long)offset, &inoutLength))
            ) {
            returnValue = (jint)inoutLength;
        }
        MMP_FREE(pBuf);
    }
    
    KNI_EndHandles();
    
    KNI_ReturnInt(returnValue);
}

/* RateControl **************************************************/

/*  private native int nGetMaxRate ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetMaxRate() {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        long rate = 0;
        if (JAVACALL_SUCCEEDED(javacall_media_get_max_rate(pKniInfo->pNativeHandle, &rate))) {
            returnValue = (jint)rate;
        }
    }

    KNI_ReturnInt(returnValue);
}

/*  private native int nGetMinRate ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetMinRate() {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        long rate = 0;
        if (JAVACALL_SUCCEEDED(javacall_media_get_min_rate(pKniInfo->pNativeHandle, &rate))) {
            returnValue = (jint)rate;
        }
    }

    KNI_ReturnInt(returnValue);
}

/*  private native int nSetRate ( int handle , int rate ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nSetRate() {

    jint handle = KNI_GetParameterAsInt(1);
    jint rate = KNI_GetParameterAsInt(2);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_SUCCEEDED(javacall_media_set_rate(pKniInfo->pNativeHandle, (long)rate))) {
            long realRate = 0;
            if (JAVACALL_SUCCEEDED(javacall_media_get_rate(pKniInfo->pNativeHandle, &realRate))) {
                returnValue = (jint)realRate;
            }
        }
    }

    KNI_ReturnInt(returnValue);
}

/*  private native int nGetRate ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetRate() {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        long rate = 0;
        if (JAVACALL_SUCCEEDED(javacall_media_get_rate(pKniInfo->pNativeHandle, &rate))) {
            returnValue = (jint)rate;
        }
    }

    KNI_ReturnInt(returnValue);
}

/* PitchControl **************************************************/

/*  private native int nGetMaxPitch ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetMaxPitch() {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        long pitch = 0;
        if (JAVACALL_SUCCEEDED(javacall_media_get_max_pitch(pKniInfo->pNativeHandle, &pitch))) {
            returnValue = (jint)pitch;
        }
    }

    KNI_ReturnInt(returnValue);
}

/*  private native int nGetMinPitch ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetMinPitch() {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        long pitch = 0;
        if (JAVACALL_SUCCEEDED(javacall_media_get_min_pitch(pKniInfo->pNativeHandle, &pitch))) {
            returnValue = (jint)pitch;
        }
    }

    KNI_ReturnInt(returnValue);
}

/*  private native int nSetPitch ( int handle , int pitch ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nSetPitch() {

    jint handle = KNI_GetParameterAsInt(1);
    jint pitch = KNI_GetParameterAsInt(2);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_SUCCEEDED(javacall_media_set_pitch(pKniInfo->pNativeHandle, (long)pitch))) {
            long realPitch = 0;
            if (JAVACALL_SUCCEEDED(javacall_media_get_pitch(pKniInfo->pNativeHandle, &realPitch))) {
                returnValue = (jint)realPitch;
            }
        }
    }

    KNI_ReturnInt(returnValue);
}

/*  private native int nGetPitch ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetPitch() {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        long pitch = 0;
        if (JAVACALL_SUCCEEDED(javacall_media_get_pitch(pKniInfo->pNativeHandle, &pitch))) {
            returnValue = (jint)pitch;
        }
    }

    KNI_ReturnInt(returnValue);
}

/* TempoControl **************************************************/

/*  private native int nGetTempo ( int handle ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetTempo() {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        long tempo = 0;
        if (JAVACALL_SUCCEEDED(javacall_media_get_tempo(pKniInfo->pNativeHandle, &tempo))) {
            returnValue = (jint)tempo;
        }
    }

    KNI_ReturnInt(returnValue);
}

/*  private native int nSetTempo ( int handle , int tempo ) ; */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nSetTempo() {

    jint handle = KNI_GetParameterAsInt(1);
    jint tempo = KNI_GetParameterAsInt(2);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jint returnValue = 0;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_SUCCEEDED(javacall_media_set_tempo(pKniInfo->pNativeHandle, (long)tempo))) {
            long realTempo = 0;
            if (JAVACALL_SUCCEEDED(javacall_media_get_tempo(pKniInfo->pNativeHandle, &realTempo))) {
                returnValue = (jint)realTempo;
            }
        }
    }

    KNI_ReturnInt(returnValue);
}


/*   private native boolean nIsBankQuerySupported(int handle) ; */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_mmedia_DirectMIDIControl_nIsBankQuerySupported() {

    jint handle = KNI_GetParameterAsInt(1);
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;
    jboolean returnValue = KNI_FALSE;
    long supported;

    if (pKniInfo && pKniInfo->pNativeHandle) {
        if (JAVACALL_SUCCEEDED(javacall_media_is_midibank_query_supported(pKniInfo->pNativeHandle, &supported))) {
            returnValue = supported ? KNI_TRUE : KNI_FALSE;
        }
    }

    KNI_ReturnBoolean(returnValue);
}

/* private native int nGetBankList(int handle, boolean custom, int[] list);  */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetBankList() {

    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)KNI_GetParameterAsInt(1);
    jint returnValue = -1;

    if (pKniInfo && pKniInfo->pNativeHandle) {

        jboolean custom;
        short *bl;
        long numBanks;
        int i, len;
        KNI_StartHandles(1);
        KNI_DeclareHandle(arrayHandle);

        custom = KNI_GetParameterAsBoolean(2);
        KNI_GetParameterAsObject(3, arrayHandle);
        len = (int)KNI_GetArrayLength(arrayHandle);

        /* IMPL NOTE: is there a calloc available?? */
        bl = (short*)MMP_MALLOC(len * sizeof(short));	
        numBanks = len;

        if (JAVACALL_SUCCEEDED(javacall_media_get_midibank_list(pKniInfo->pNativeHandle, custom, bl, &numBanks))) {
            if(numBanks < len) len = numBanks;

            for(i=0; i<len; i++)
                KNI_SetIntArrayElement(arrayHandle, i, (jint)bl[i]);

            returnValue = len;
        }

        MMP_FREE(bl);
        KNI_EndHandles();
    }

    KNI_ReturnInt(returnValue);
}

/* private native int nGetKeyName(int handle, int bank, int prog, int key, byte[] keyname);  */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetKeyName() {

    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)KNI_GetParameterAsInt(1);
    jint returnValue = -1;

    if (pKniInfo && pKniInfo->pNativeHandle) {

        long bank, prog, key;
        char *kname;
        long len;
        KNI_StartHandles(1);
        KNI_DeclareHandle(arrayHandle);

        bank = (long)KNI_GetParameterAsInt(2);
        prog = (long)KNI_GetParameterAsInt(3);
        key = (long)KNI_GetParameterAsInt(4);

        KNI_GetParameterAsObject(5, arrayHandle);
        len = (long)KNI_GetArrayLength(arrayHandle);

        /* IMPL NOTE: is there a calloc available?? */
        kname = (char*)MMP_MALLOC((len+1) * sizeof(jbyte));	
        kname[len] = '\0';

        if (JAVACALL_SUCCEEDED(javacall_media_get_midibank_key_name(pKniInfo->pNativeHandle, bank, prog, key, kname, &len))) {
            KNI_SetRawArrayRegion(arrayHandle, 0, (jsize)len, ( jbyte* )kname);

            returnValue = ( jint )len;
        }

        MMP_FREE(kname);
        KNI_EndHandles();
    }

    KNI_ReturnInt(returnValue);
}

/*  private native boolean nGetProgramName(int handle, int bank, int prog, byte[] progname);  */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetProgramName() {

    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)KNI_GetParameterAsInt(1);
    jint returnValue = -1;

    if (pKniInfo && pKniInfo->pNativeHandle) {

        long bank, prog;
        char *pname;
        long len;
        KNI_StartHandles(1);
        KNI_DeclareHandle(arrayHandle);

        bank = (long)KNI_GetParameterAsInt(2);
        prog = (long)KNI_GetParameterAsInt(3);

        KNI_GetParameterAsObject(4, arrayHandle);
        len = ( long )KNI_GetArrayLength(arrayHandle);

        /* IMPL NOTE: is there a calloc available?? */
        pname = (char*)MMP_MALLOC((len+1) * sizeof(jbyte));
        pname[len] = '\0';

        if (JAVACALL_SUCCEEDED(javacall_media_get_midibank_program_name(pKniInfo->pNativeHandle, bank, prog, pname, &len))) {
            KNI_SetRawArrayRegion(arrayHandle, 0, (jsize)len, ( jbyte* )pname);

            returnValue = ( jint )len;
        }

        MMP_FREE(pname);
        KNI_EndHandles();
    }

    KNI_ReturnInt(returnValue);
}


/*  private native boolean nGetProgramList(int handle, int bank, int[] proglist);  */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetProgramList() {

    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)KNI_GetParameterAsInt(1);
    jint returnValue = -1;

    if (pKniInfo && pKniInfo->pNativeHandle) {

        long bank;
        char *plist;
        int i;
        long len;
        KNI_StartHandles(1);
        KNI_DeclareHandle(arrayHandle);

        bank = (long)KNI_GetParameterAsInt(2);

        KNI_GetParameterAsObject(3, arrayHandle);
        len = ( long )KNI_GetArrayLength(arrayHandle);

        /* IMPL NOTE: is there a calloc available?? */
        plist = (char*)MMP_MALLOC((len) * sizeof(char));	

        if (JAVACALL_SUCCEEDED(javacall_media_get_midibank_program_list(pKniInfo->pNativeHandle, bank, plist, &len))) {

            for(i=0; i<len; i++)
                KNI_SetIntArrayElement(arrayHandle, i, (jint)plist[i]);

            returnValue = ( jint )len;
        }

        MMP_FREE(plist);
        KNI_EndHandles();
    }

    KNI_ReturnInt(returnValue);
}

/* private native int nGetProgram(int handle, int[] program);  */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_mmedia_DirectMIDIControl_nGetProgram() {

    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)KNI_GetParameterAsInt(1);
    jint returnValue = -1;

    if (pKniInfo && pKniInfo->pNativeHandle) {

        jboolean custom;
        long *p;
        int channel;
        KNI_StartHandles(1);
        KNI_DeclareHandle(arrayHandle);

        KNI_GetParameterAsObject(3, arrayHandle);
        channel = (int)KNI_GetParameterAsInt(2);

        /* IMPL NOTE: is there a calloc available?? */
        p = (long*)MMP_MALLOC(2 * sizeof(long));	

        if (JAVACALL_SUCCEEDED(javacall_media_get_midibank_program(pKniInfo->pNativeHandle, channel, p))) {

            KNI_SetIntArrayElement(arrayHandle, 0, (jint)p[0]);
            KNI_SetIntArrayElement(arrayHandle, 1, (jint)p[1]);

            returnValue = 0;
        }

        MMP_FREE(p);
        KNI_EndHandles();
    }

    KNI_ReturnInt(returnValue);
}

