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
package com.sun.mmedia;

import javax.microedition.media.control.MetaDataControl;

public class QSoundMetaDataCtrl implements MetaDataControl
{
    private int peer;

    public QSoundMetaDataCtrl(int mc)
    {
        peer = mc;
    }

    private native int nGetKeyValue(int peer, byte[] key, byte[] value);
    
    public String getKeyValue(String key)
    {
        if(key == null) throw new IllegalArgumentException("null key");
        if(key.length() == 0) throw new IllegalArgumentException("empty string");
        
        byte[] val = new byte[250];
        byte[] bk = key.getBytes();
        
        int n = nGetKeyValue(peer, bk, val);
        
        if(n <= 0)
            throw new IllegalArgumentException();
        
        return new String(val, 0, n);
    }

    private native int nGetKeys(int peer, byte[] pn, int cnt);
    
    public String[] getKeys()
    {
        int cnt = 0;
        byte[] pn = new byte[40];
        String[] names = new String[50];   // Hopefully enough...
        int n;
        
        while((n = nGetKeys(peer, pn, cnt)) > 0)
            names[cnt++] = new String(pn, 0, n);
        
        String[] ret = new String[cnt];

        for(int i=0; i<cnt; i++)
            ret[i] = names[i];

        return ret;
    }


}






