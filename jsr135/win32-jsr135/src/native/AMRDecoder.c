/*
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
/**
 * @file AMRDecoder.c
 * This is the JNI/KNI porting layer for AMRDecoder.
 */


#include "mni.h"

/**
 * This function opens the native AMR decoder.
 * @param jtype 1 for narrow band, 2 for wideband
 * @return  pointer to native decoder structure
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_AMRDecoder_nOpen(
MNI_FUNCTION_PARAMS_2(jint jtype, jint jchannels)
)
{
    MNI_RET_VALUE_INT(0);
}


MNI_RET_TYPE_INT
Java_com_sun_mmedia_AMRDecoder_nDecode(
MNI_FUNCTION_PARAMS_6(jint jpeer,
		      jbyteArray jamrData,
		      jint joffset,
		      jint jlength,
		      jint jchannels,
		      jbyteArray jpcmData)
)
{
    MNI_RET_VALUE_INT(0);
}

MNI_RET_TYPE_VOID
Java_com_sun_mmedia_AMRDecoder_nClose(
MNI_FUNCTION_PARAMS_1(jint jpeer)
)
{
}
