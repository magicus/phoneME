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

package com.sun.midp.main;
import javax.microedition.lcdui.Image;

import com.sun.midp.lcdui.Notice;

/**
 * {@link NoticeManagerListener} implementation.
 * Forwards incoming notifications to native AMS.
 * 
 */
class NativeNoticeListener implements NoticeManagerListener {

    /** Notice operation types */
    private static final int ADDED      = 1;
    private static final int UPDATED    = 2;
    private static final int REMOVED    = 3;

    /**
     * <code>NativeNoticeListener</code> singletone
     * 
     */
    private static NativeNoticeListener singleton;

    /**
     * <code>NativeNoticeListener</code> initializator
     * 
     */
    public static void init() {
        if (null == singleton) {
            singleton = new NativeNoticeListener();
        }
    }

    /**
     * Hidden Constructor.
     * 
     */
    private NativeNoticeListener() {
        NoticeManager.getInstance().addListener(this);
    }

    /**
     * Informs about new information note.
     *
     * @param notice new information note
     */
    public void notifyNotice(Notice notice) {
        noticeUpdate(notice, ADDED);
    }   
    /**
     * Informs about given information note need to be discarded
     *
     * @param notice information note
     */
    public void removeNotice(Notice notice) {
        noticeUpdate(notice, REMOVED);
    }
    /**
     * Informs that the notice was updated
     *
     * @param notice the notice was updated
     */
    public void updateNotice(Notice notice) {
        noticeUpdate(notice, UPDATED);
    }


    /**
     * Extracts nd forwards notification data to NAMS
     *  
     * @param notice incoming note
     * @param operation status
     */
    private void noticeUpdate(Notice notice, int operation) {
        int uid = notice.getUID();
        int type = notice.getType().getType();
        String label = notice.getLabel();
        Image image = notice.getImage();
        boolean selectable = notice.isSelectable();
        long timeout = notice.getTimeout();
        MIDletProxy proxy = MIDletProxyList.getMIDletProxyList().
            findFirstMIDletProxy(notice.getOriginatorID());
        if (null == proxy) {
            return;
        }
        int appID = proxy.getExternalAppId();
        noticeUpdate0(uid,type,label,image,selectable,
                      timeout, operation, appID);
    }

    /**
     * Puts notification data to native code.
     * 
     * @param uid           notification UID
     * @param type          notification type UID
     * @param label         notification label 
     * @param image         notification image
     * @param selectable    selectable status
     * @param timeout       expiration time
     * @param operation     type of oepration (ADDED, UPDATED, 
     *                      REMOVED)
     * @param externalAppID NAMS task ID of the originator
     *  
     * @see Notice 
     */
    private native void noticeUpdate0(int uid, int type, 
                                     String label, Image image, 
                                     boolean selectable, long timeout,
                                     int operation,
                                     int externalAppID);
}
