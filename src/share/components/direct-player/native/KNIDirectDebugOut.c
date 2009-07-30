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

#include <windows.h>
#include "KNICommon.h"
#include "sni.h"
#include "javautil_unicode.h"

#define MAXDEBUGMSG_LEN 250

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_DirectDebugOut_nDebugPrint) {
    jchar jStr[ MAXDEBUGMSG_LEN ];
    char str[ MAXDEBUGMSG_LEN ];
    int len = 0;
    
    KNI_StartHandles( 1 );
    KNI_DeclareHandle( javaStringObj );
    KNI_GetParameterAsObject( 1, javaStringObj );
    
    len = KNI_GetStringLength( javaStringObj );
    
    if( MAXDEBUGMSG_LEN - 1 < len )
    {
        len = MAXDEBUGMSG_LEN - 1;
    }
    
    if( len > 0 )
    {
        KNI_GetStringRegion( javaStringObj, 0, len, jStr );
        javautil_unicode_utf16_to_utf8(jStr, len, str, 
                MAXDEBUGMSG_LEN - 1, &len);
        str[ len ] = '\0';
        OutputDebugString( str );
    }
    
    KNI_EndHandles();
    KNI_ReturnVoid();
}
