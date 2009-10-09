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

import java.io.ByteArrayOutputStream;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;

import javax.microedition.lcdui.Image;

import com.sun.midp.events.NativeEvent;
import com.sun.midp.main.Configuration;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;


public abstract class NoticeBase {

    public static final int AVAILABLE = 0;
    public static final int REMOVED = 1;

    public static final int DISMISSED = 1;
    public static final int SELECTED  = 2;
    public static final int DELETED   = 3;
    public static final int TIMEOUT   = 4;

    protected long timestamp;

    protected Image image;

    protected String label;

    protected NoticeType type;

    protected boolean selectable;

    protected NoticeListener listener;

    protected int uid;

    protected int status;

    protected String originator;

    protected long timeout;


    /**
     * Creates a new Notification with the given type, label and
     * image.
     *
     * @param type  the notification type
     * @param label the notification's label
     * @param image the notification's image
     */
    protected NoticeBase(NoticeType newType, String newLabel, Image newImage) {
        if (null == newType) {
            throw new NullPointerException();
        }
        type = newType;
        if (null == newLabel) {
            label = type.label;
        } else {
            label = newLabel;
        }
        if (null == newImage) {
            image = type.image;
        } else {
            image = newImage;
        }

        MIDletStateHandler handler = MIDletStateHandler.getMidletStateHandler();
        MIDletSuite suite = handler.getMIDletSuite();
        originator = suite.getMIDletName(handler.getFirstRunningMidlet());

        /**
         * It is necessary to distinguish between different
         * notifications with the same type. Timestamp is not robust
         * because windows has time quantum around 10ms
         */
        uid = getUID0();

        status = REMOVED;

        NoticeWatcher.init();
    }

    protected NoticeBase() {
        NoticeWatcher.init();
    }


    public long getTimestamp() {
        return timestamp;
    }

    public NoticeType getType() {
        return type;
    }

    public void setImage(Image newImage) {
        image = newImage;
    }

    public void setLabel(String newLabel) {
        label = newLabel;
    }

    public synchronized void post(boolean selectable, int duration) throws IOException {
        int minDuration = 
            Configuration.getIntProperty("NOTIFICATION.MIN_DURATION", duration);
        if (duration < minDuration) {
            duration = minDuration;
        }
        status = AVAILABLE;
        timestamp = System.currentTimeMillis();
        this.selectable = selectable;

        timeout = timestamp + duration;
    }

    public boolean isSelectable() {
        return selectable;
    }

    /**
     * Synchronized with post where the value is updated
     * 
     * 
     * @return int 
     */
    public synchronized long getTimeout() {
        return timeout;
    }

    public void remove() {
        if (REMOVED == status) {
            throw new IllegalStateException("Already removed");
        }
        status = REMOVED;
    }

    public String getLabel() {
        return label;
    }

    public Image getImage() {
        return image;
    }

    public void setListener(NoticeListener l) {
        if (null != listener && null == l) {
            remove();
        }
        listener = l;
    }


    public String getOriginator() {
        return originator;
    }

    public int getUID() {
        return uid;
    }

    public abstract void dismiss();

    public abstract void select();

    public abstract void timeout();

    public void removed(int reason) {
        if (REMOVED == status) {
            return;
        }
        status = REMOVED;
        if (null != listener) {
            if (SELECTED == reason) {
                listener.noticeSelected((Notice)this);
            } else if (DISMISSED == reason) {
                listener.noticeDismissed((Notice)this);
            } else if (TIMEOUT == reason) {
                listener.noticeTimeout((Notice)this);
            }
        }
    }

    private native int getUID0();
}
