/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.lcdui;

import javax.microedition.lcdui.Image;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

public class NoticeType {

    public static final int CALL_TYPE = 0;
    public static final int EMAIL_TYPE = 1;
    public static final int IM_TYPE = 2;
    public static final int MMS_TYPE = 3;
    public static final int REMINDER_TYPE = 4;
    public static final int SMS_TYPE = 5;
    public static final int USER_TYPE = 6;

    String label;
    Image image;
    int type;

    public NoticeType(int defaultType, String defaultLabel, Image defaultImage) {
        type = defaultType;
        label = defaultLabel;
        image = defaultImage;
        if (USER_TYPE == type) {
            /* user type is generating for every instance
               despite of label and image values */
            type += getUID0();
        }
    }

    NoticeType(int type) {
        this.type = type;
    }

    public boolean equals(Object obj) {
        if (obj instanceof NoticeType) {
            if (type == ((NoticeType)obj).type) {
                return true;
            }
        }
        return false;
    }

    public int getType() {
        return type;
    }

    private native int getUID0();
}
