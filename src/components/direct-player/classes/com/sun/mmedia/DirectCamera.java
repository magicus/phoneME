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
 
package com.sun.mmedia;

import javax.microedition.media.MediaException;

public class DirectCamera extends DirectVideo 
{
    /**
     * It does not need data source
     */
    public DirectCamera() {
        hasDataSource = false;
    }

    /**
     * Camera snapshot
     */
    public byte[] getSnapshot(String imageType) throws MediaException {
        checkState();
        checkPermission();

        if (null == imageType) {
            imageType = System.getProperty("video.snapshot.encodings");
            if (null == imageType) {
                throw new MediaException("Requested format is not supported");
            }
        }

        // Need revisit - Check encoding param validity (Not from Sprint PCS QVM based test case)

        byte[] data = null;
        if (hNative != 0) {
            data = nSnapShot(hNative, imageType.toLowerCase());
        }
        if (null == data) {
            throw new MediaException(imageType + " format is not supported");
        }
        return data;
    }
}

