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


/**
 * <code>Notice</code> is a small information note that can be
 * shown to the user from a headless application or a background
 * application. 
 * <p> 
 * <code>NoticeBase</code> contains a code shared between all 
 * target specific implementations. 
 * 
 */
public abstract class NoticeBase {

    /* --------- Notice state codes --------------*/
    /**
     * . A Notice becomes available it was posted.
     * 
     */
    public static final int AVAILABLE = 0;

    /**
     * Notice state. A Notice becomes removed after one of the 
     * following actions: 
     * <ul> 
     * <li>a Notice was dismissed or selected by the user</li> 
     * <li>a Notice was removed by the originator</li> 
     * <li>a Notice was fired by timer</li> 
     * </ul>
     * 
     */
    public static final int REMOVED = 1;


    /* --------- Notice removal reason codes -------- */

    /**
     * A notice was dismissed.
     * 
     */
    public static final int DISMISSED = 1;

    /**
     * A notice was selected.
     * 
     */
    public static final int SELECTED  = 2;

    /**
     * A notice was expired.
     * 
     */
    public static final int TIMEOUT   = 3;

    /**
     * A notice was removed by originator.
     * 
     */
    public static final int DELETED   = 4;

    /**
     * A time when the Notice was last posted.
     * 
     */
    protected long timestamp;

    /**
     * The notice image.
     * 
     */
    protected Image image;

    /**
     * The notice label.
     * 
     */
    protected String label;

    /**
     * The notice type.
     * 
     */
    protected NoticeType type;

    /**
     * Selectable status of posted notice.
     * 
     */
    protected boolean selectable;

    /**
     * The listener of the notice status.
     * 
     */
    protected NoticeListener listener;

    /**
     * The notice UID.
     * 
     */
    protected int uid;

    /**
     * The notice status.
     * 
     */
    protected int status;


    /**
     * The name of the originator application.
     * 
     */
    protected String originator;

    /**
     * The notice expiration time.
     * 
     */
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

    /**
     * Parameterless constructor. Used by MVM Notice implementation
     * when initialization is done through deserialization.
     * 
     */
    protected NoticeBase() {
        NoticeWatcher.init();
    }


    /**
     * Returns the notice timestamp.
     * 
     * 
     * @return the time the notice was last posted.
     */
    public long getTimestamp() {
        return timestamp;
    }

    /**
     * Returns the notice type.
     * 
     * 
     * @return NoticeType
     */
    public NoticeType getType() {
        return type;
    }

    /**
     * Changes the notice image.
     * 
     * 
     * @param newImage new notice image.
     */
    public void setImage(Image newImage) {
        image = newImage;
    }

    /**
     * Changes the notice label.
     * 
     * 
     * @param newLabel new notice label.
     */
    public void setLabel(String newLabel) {
        label = newLabel;
    }

    /**
     * Publishes the Notice. 
     * 
     * @param selectable <code>true</code> if the Notice must be 
     *                   presented in selectable way.
     * @param duration  the notice duration time.
     */
    public synchronized void post(boolean selectable, int duration) throws IOException {
        /* Replace duration value with more conform value*/
        int minDuration = 
            Configuration.getIntProperty("NOTIFICATION.MIN_DURATION", duration);
        if (duration < minDuration && 0 != duration) {
            duration = minDuration;
        }
        status = AVAILABLE;
        timestamp = System.currentTimeMillis();
        this.selectable = selectable;

        if (0 != duration) {
            timeout = timestamp + duration;
        } else {
            timeout = 0;
        }
    }

    /**
     * Checks the notice for selectable status.
     * 
     * 
     * @return boolean <code>true</code> is the Notice is 
     *         selectable.
     */
    public boolean isSelectable() {
        return selectable;
    }

    /** 
     * Returns the notice expiration time.
     *  <p>
     * Synchronized with post where the value is updated 
     * 
     * @return the notice expiration time or 0 if the notice can't 
     *         be expired
     */
    public synchronized long getTimeout() {
        return timeout;
    }

    /**
     * Removes the notice.
     * 
     */
    public void remove() {
        if (REMOVED == status) {
            throw new IllegalStateException("Already removed");
        }
        status = REMOVED;
    }

    /**
     * Returns the notice label.
     * 
     * 
     * @return notice label
     */
    public String getLabel() {
        return label;
    }

    /**
     * Returns the notice image.
     * 
     * 
     * @return notice image
     */
    public Image getImage() {
        return image;
    }

    /**
     * Assigns the notice status listener. 
     * <p> 
     * If the listener reference is <code>null</code> then the 
     * notice is removed as well. 
     * 
     * @param l new listener reference.
     */
    public void setListener(NoticeListener l) {
        if (null != listener && null == l) {
            remove();
        }
        listener = l;
    }


    /**
     * Returns the originator application name.
     * 
     * 
     * @return the application name.
     */
    public String getOriginator() {
        return originator;
    }

    /**
     * Returns unique ID of the notice.
     * 
     * 
     * @return UID
     */
    public int getUID() {
        return uid;
    }

    /**
     * Abstract method that removes the notice with notification of
     * the listeners through {@link NoticeListener#noticeDismissed}
     * call. 
     * <p> 
     * The function is called by the code that handles user action 
     * for this notice. 
     * <p>
     * The implementation depends on target environment.
     */
    public abstract void dismiss();

    /**
     * Abstract method that removes the notice with notification of
     * the listeners through {@link NoticeListener#noticeSelected}
     * call.
     * <p> 
     * The function is called by the code that handles user action 
     * for this notice. 
     * <p>
     * The implementation depends on target environment.
     */
    public abstract void select();

    /**
     * Abstract method that removes the notice with notification of
     * the listeners through {@link NoticeListener#noticeTimeout}
     * call.
     * <p> 
     * The function is called by {@link NoticeWatcher}
     * <p>
     * The implementation depends on target environment.
     */
    public abstract void timeout();


    /**
     * Creates unique ID for new <code>Notice</code> instance.
     * 
     * 
     * @return UID
     */
    private native int getUID0();
}
