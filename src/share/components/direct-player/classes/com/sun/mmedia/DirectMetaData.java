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

package com.sun.mmedia;

import javax.microedition.media.control.MetaDataControl;


public class DirectMetaData implements MetaDataControl {    
    private int hNative;    
    private String [] keys;
    
    private native int nGetKeyCount(int hNative);
    private native String nGetKey(int hNative, int index);
    private native String nGetKeyValue(int hNative, String key);
    
    private void updateKeys() {
        int newKeys = 0;
        if (hNative != 0)
            newKeys = nGetKeyCount(hNative);

        if (newKeys > 0) {
            if (newKeys != keys.length) {
                keys = new String[newKeys];
                for (int i = 0; i < newKeys; i++) {
                    keys[i] = nGetKey(hNative, i);
                }
            }
        } else {
            keys = null;
        }
    }    

    DirectMetaData(int hNative) {
        this.hNative = hNative;
    }

    void playerClosed() {
        hNative = 0;
        keys = null;
    }

    public String[] getKeys() {
        updateKeys();
        return keys;
    }

    public String getKeyValue(String key) {
        if (key == null) {
            throw new IllegalArgumentException("Key is null");
        updateKeys();
        for (int i = 0; i < keys.length; i++) {
            if (key.equals(keys[i])) {
                return nGetKeyValue(hNative, key);
            }
        }
        throw new IllegalArgumentException("Key is invalid");
    }
}