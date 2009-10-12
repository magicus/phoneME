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


/**
 * A class that represents the {@link Notice} type.
 * 
 */
public class NoticeType {

    /**
     * Default type for missed calls.
     * 
     */
    public static final int CALL_TYPE = 0;

    /**
     * Default type for email notices.
     * 
     */
    public static final int EMAIL_TYPE = 1;

    /**
     * Default type for instant message notice.
     * 
     */
    public static final int IM_TYPE = 2;

    /**
     * Default notice type for MMS message.
     * 
     */
    public static final int MMS_TYPE = 3;

    /**
     * Default type for reminder notices.
     * 
     */
    public static final int REMINDER_TYPE = 4;

    /**
     * Default type for SMS messages.
     * 
     */
    public static final int SMS_TYPE = 5;

    /**
     * Basic type for user defined notices.
     * All user defined types are equals or greater this value.
     * 
     */
    public static final int USER_TYPE = 6;

    /**
     * Default label for the notice of such type.
     * 
     */
    String label;

    /**
     * Default image for the notice of such type.
     * 
     */
    Image image;

    /**
     * Notice type UID.
     * 
     */
    int type;

    /**
     * Default constructor.
     * <p> 
     *  
     * 
     * @param defaultType   specifies group the NoticeType belongs 
     *                      to.
     * @param defaultLabel  specifies the NoticeType default label. 
     *                      Can't be null.
     * @param defaultImage  specifies the NoticeType default image. 
     *                      As opposed to <code>defaultLabel</code>
     *                      can be NULL
     */
    public NoticeType(int defaultType, String defaultLabel, Image defaultImage) {
        if (defaultType > USER_TYPE) {
            throw new IllegalArgumentException("Invalid notice type: " + defaultType);
        }
        type = defaultType;
        if (null == defaultLabel) {
            throw new NullPointerException("Default label can't be null");
        }
        label = defaultLabel;
        image = defaultImage;
        if (USER_TYPE == type) {
            /* user type is generating for every instance
               despite of label and image values */
            type += getUID0();
        }
    }

    /**
     * Package protected class is used only for <code>Notice</code>
     * deserialization. 
     * 
     * @param type NoticeType UID extracted from serialed stream.
     */
    NoticeType(int type) {
        this.type = type;
    }

    /**
     * Compares two object
     * 
     * 
     * @param obj an object the NoticeType is compared with.
     * 
     * @return boolean true is <code>obj</code> is instance of 
     *         NoticeType and has the same UID.
     */
    public boolean equals(Object obj) {
        if (obj instanceof NoticeType) {
            if (type == ((NoticeType)obj).type) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns notice type UID.
     * 
     * 
     * @return notice type UID.
     */
    public int getType() {
        return type;
    }

    /**
     * Creates unique ID across all tasks. 
     *  
     * @return UID
     */
    private native int getUID0();
}
