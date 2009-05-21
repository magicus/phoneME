/*
 * 
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

#include "PPCBrush.h"

void AwtGDIObject::ReleaseObject(void* obj) {
    ((AwtGDIObject*)obj)->Release();
}

void AwtGDIObject::ForceGarbageCollection()
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }
    
    env->CallStaticVoidMethod(WCachedIDs.java_lang_SystemClass,
			      WCachedIDs.java_lang_System_gcMID);
    env->CallStaticVoidMethod(WCachedIDs.java_lang_SystemClass,
			      WCachedIDs.java_lang_System_runFinalizationMID);
}

Hashtable AwtBrush::cache("Brush cache", ReleaseObject);

AwtBrush::AwtBrush(COLORREF color) : AwtGDIObject(color, SafeCreateSolidBrush(color)) {
}

AwtBrush* AwtBrush::Get(COLORREF color) {
    AwtBrush* obj = (AwtBrush*)cache.get((void*)color);
    if (obj == NULL) {
        obj = new AwtBrush(color);
        VERIFY(cache.put((void*)color, obj) == NULL);
    }
    obj->IncrRefCount();
    return obj;
}

HBRUSH AwtBrush::SafeCreateSolidBrush(COLORREF cr) {
    //!CQ could do partial flushes more often, based on LRU or refcount == 1, etc
    HBRUSH hbr = ::CreateSolidBrush(cr);
    if (!hbr) {
        // Flush cache (it calls back to Release each Brush in it). Any current references to those
        // brushes will remain valid until the last Release is called
        cache.clear();
        hbr = ::CreateSolidBrush(cr);
        // if that didn't work, then try a gc to cleanup potential java objects that
        // are hanging onto our objects
        if (!hbr) {
            ForceGarbageCollection();
            hbr = ::CreateSolidBrush(cr);
            ASSERT(hbr != NULL);
        }
    }
    return hbr;
}
