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
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Stack;
import java.util.Vector;

import com.sun.midp.ams.VMUtils;
import com.sun.midp.events.Event;
import com.sun.midp.events.EventListener;
import com.sun.midp.events.EventQueue;
import com.sun.midp.events.EventTypes;
import com.sun.midp.events.NativeEvent;
import com.sun.midp.lcdui.Notice;
import com.sun.midp.log.LogChannels;
import com.sun.midp.log.Logging;


/**
 * Notice manager implements {@link Notice} posting and removing
 * functionality as well as notification of listeners about
 * notices status.
 * 
 */
public class NoticeManager implements EventListener, MIDletProxyListListener {

    /**
     * An error message that is displayed if specified
     * initialization sequence was violated.
     * 
     */
    private static final String errorMsg = 
    "Incorrect usage: NoticeManager must be initialized first";

    /**
     * The singleton.
     * 
     */
    private static NoticeManager singleton;

    /**
     * Current task event queue.
     * 
     */
    private EventQueue queue;

    /**
     * Registered notice status listeners.
     * 
     */
    private Vector listeners;

    /**
     * Registered notices.
     * 
     */
    private Vector notices;


    /**
     * Common initialization function. Registers 
     * NOTIFICATION_ANNOUNCEMENT_EVENT handler. Used by every task 
     * initializers. 
     * 
     * 
     * @param queue event queue.
     */
    public static void initCommon(EventQueue queue) {
        if (null == singleton) {
            singleton = new NoticeManager(queue);
        }
    }

    /**
     * AMS specific initialization. Registers the manager as {@link 
     * MIDletProxyList} listener. Need to cleanup registered notice 
     * table if the originator MIDlet exits unexpectedly.
     * 
     * 
     * @param proxyList 
     */
    public static void initWithAMS(MIDletProxyList proxyList) {
        if (null == singleton) {
            if (Logging.REPORT_LEVEL <= Logging.CRITICAL) {
                Logging.report(Logging.CRITICAL, LogChannels.LC_AMS,
                               errorMsg);
            }
            throw new RuntimeException(errorMsg);
        }
        proxyList.addListener(singleton);
    }


    /**
     * Private constructor assure the single instance of 
     * NoticeManager. 
     * 
     * 
     * @param queue current task event queue.
     */
    private NoticeManager(EventQueue queue) {
        queue.registerEventListener(EventTypes.NOTIFICATION_ANNOUNCEMENT_EVENT, this);
        // home indicator and notice visualizer
        listeners = new Vector(2);

        notices = new Vector();
        this.queue = queue;
    }

    /**
     * Returns NoticeManager singleton.
     * 
     * 
     * @return NoticeManager singleton.
     */
    public static NoticeManager getInstance() {
        if (null == singleton) {
            if (Logging.REPORT_LEVEL <= Logging.CRITICAL) {
                Logging.report(Logging.CRITICAL, LogChannels.LC_AMS,
                               errorMsg);
            }
            throw new RuntimeException(errorMsg);
        }
        return singleton;
    }

    /* -------------------- EventListener --------------------------- */
    /**
     * Preprocess an event that is being posted to the event queue.
     * This method will get called in the thread that posted the event.
     *
     * @param event event being posted
     *
     * @param waitingEvent previous event of this type waiting in the
     *     queue to be processed
     *
     * @return true to allow the post to continue, false to not post the
     *     event to the queue
     */
    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }
    /**
     * Process an event.
     * This method will get called in the event queue processing thread.
     *
     * @param event event to process
     */
    public void process(Event event) {
        NativeEvent ev = (NativeEvent)event;
        Notice notice = findNotice(ev.intParam1);
        if (null == notice) {
            if (ev.intParam2 != Notice.REMOVED) {
                try {
                    /* We specify class name for the case when custom Notice based class was posted.*/
                    notice = (Notice)Class.forName(ev.stringParam1).newInstance();
                } catch (Exception ee) {
                    if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                        Logging.report(Logging.ERROR, LogChannels.LC_HIGHUI, 
                                       "Can't instantiate " + ev.stringParam1);
                    }
                    return;
                }
                /* String as container is used temporary.*/
                ByteArrayInputStream byteStream = new ByteArrayInputStream(ev.stringParam2.getBytes());
                DataInputStream inData = new DataInputStream(byteStream);
                try {
                    notice.deserialize(inData);
                } catch (IOException e) {
                    if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                        Logging.report(Logging.ERROR, LogChannels.LC_HIGHUI, 
                                       "Can't deserialize " + ev.stringParam1);
                    }
                    return;
                }
                notifyNew(notice);
            }
        } else if (Notice.REMOVED == ev.intParam2) {
            notifyRemoved(notice, ev.intParam3);
        } else {
            /* String as container is used temporary.*/
            ByteArrayInputStream byteStream = new ByteArrayInputStream(ev.stringParam2.getBytes());
            DataInputStream inData = new DataInputStream(byteStream);
            try {
                notice.deserialize(inData);
            } catch (IOException e) {
                if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                    Logging.report(Logging.ERROR, LogChannels.LC_HIGHUI, 
                                   "Can't deserialize " + notice.getClass().getName());
                }
                return;
            }
            notifyUpdated(notice);
        }

    }

    /*
       ----------------------- MIDletProxyListListener ----------------
       Does nothing for non-AMS isolates,
       but memory overhead is too small for refactoring
    */

    /**
     * Called when a MIDlet is added to the list.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
    }
    /**
     * Called when a MIDlet is removed from the list. 
     * <p> 
     * Cleanups all related  notices.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
        int is = midlet.getIsolateId();
        Enumeration enm = notices.elements();
        while (enm.hasMoreElements()) {
            Notice notice = (Notice)enm.nextElement();
            if (notice.getOriginatorID() == is) {
                notices.removeElement(notice);
                notifyRemoved(notice, Notice.DELETED);
            }
        }
    }
    /**
     * Called when error occurred while starting a MIDlet object.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error details
     */
    public void midletStartError(int externalAppId, int suiteId, String className, int errorCode, String errorDetails) {
    }
    /**
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated,
     * see constants above
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
    }


    /* ----------------------- NoticeManagerUI ----------------- */

    /** 
     * Registers notice status listener. 
     * <p> 
     * Synchronized to prevent listeners notification.
     * 
     * @param listener {@link NoticeManagerListener} implemented 
     *                 object.
     */
    public synchronized void addListener(NoticeManagerListener listener) {
        if (!listeners.contains(listener)) {
            listeners.addElement(listener);
        }
    }

    /**
     * Removes notice status listener. 
     * <p> 
     * Synchronized to prevent listeners notification.
     * 
    * @param listener {@link NoticeManagerListener} implemented 
    *                 object.
     */
    public synchronized void removeListener(NoticeManagerListener listener) {
        listeners.removeElement(listener);
    }


    /** 
     * Notifies listeners about new {@link notice}. 
     * <p> 
     * Synchronized to prevent listeners registration/removal.
     * 
     * 
     * @param notice new notice
     */
    private synchronized void notifyNew(Notice notice) {
        notices.addElement(notice);
        if (listeners.size() > 0) {
            Enumeration enm = listeners.elements();
            while (enm.hasMoreElements()) {
                NoticeManagerListener l = (NoticeManagerListener)enm.nextElement();
                l.notifyNotice(notice);
            }
        }
    }

    /** 
     * Notifies listeners about updated {@link notice}. 
     * <p> 
     * Synchronized to prevent listeners registration/removal.
     * 
     * 
     * @param notice updated notice
     */
    private synchronized void notifyUpdated(Notice notice) {
        if (listeners.size() > 0) {
            Enumeration enm = listeners.elements();
            while (enm.hasMoreElements()) {
                NoticeManagerListener l = (NoticeManagerListener)enm.nextElement();
                l.updateNotice(notice);
            }
        }
    }

    /** 
     * Notifies listeners about removed {@link notice}. 
     * <p> 
     * Synchronized to prevent listeners registration/removal.
     * 
     * 
     * @param notice removed notice 
     * @param reason the code of the reason the notice was removed.
     */
    private synchronized void notifyRemoved(Notice notice, int reason) {
        notices.removeElement(notice);
        notice.removed(reason);
        if (listeners.size() > 0) {
            Enumeration enm = listeners.elements();
            while (enm.hasMoreElements()) {
                NoticeManagerListener l = (NoticeManagerListener)enm.nextElement();
                l.removeNotice(notice);
            }
        }
    }

    /* --------------------------- Notice management ----------------- */

    /**
     * Publishes the {@link Notice} across running tasks.
     * <p> 
     * IMPL_NOTE: need to think how to avoid message flood and large 
     * image transfer 
     * 
     * @param notice the <code>Notice</code> to be published. 
     * @see NoticeBase for reason codes. 
     */
    public void post(Notice notice) throws IOException {
        NativeEvent event = new NativeEvent(EventTypes.NOTIFICATION_ANNOUNCEMENT_EVENT);
        event.intParam1 = notice.getUID();
        event.intParam2 = Notice.AVAILABLE;
        event.stringParam1 = notice.getClass().getName();
        ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
        DataOutputStream dataOut = new DataOutputStream(byteStream);
        try {
            notice.serialize(dataOut);
            dataOut.close();// closes byteStream also
            // IMPL_NOTE: will be redesigned when new NativeEvent is arrived
            event.stringParam2 = new String(byteStream.toByteArray());
            // IMPL_NOTE: replace to broadcast address
            queue.sendNativeEventToIsolate(event, VMUtils.getAmsIsolateId());
        } catch (IOException e) {
            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_HIGHUI, 
                               "Can't serialize " + notice.getClass().getName());
            }
            throw e;
        }
        // repost (if broadcast doesn't include current task)
        notifyNew(notice);
    }

    /**
     * Make previously published {@link Notice} be {@link 
     * NoticeBase#REMOVED}. 
     * 
     * 
     * @param notice the notice to remove
     * @param reason the reason of removal 
     * @see NoticeBase for reason codes. 
     */
    public void remove(Notice notice, int reason) {
        notifyRemoved(notice, reason);
        if (Notice.TIMEOUT == reason) {
            // every task handles timer separately
            return;
        }
        NativeEvent event = new NativeEvent(EventTypes.NOTIFICATION_ANNOUNCEMENT_EVENT);
        event.intParam1 = notice.getUID();
        event.intParam2 = Notice.REMOVED;
        event.intParam3 = reason;
        // no need to send whole data, the result will notice discarding
        queue.sendNativeEventToIsolate(event, notice.getOriginatorID());
    }

    /**
     * Checks for the notice with given UID.
     * 
     * 
     * @param uid unique ID of the notice.
     * 
     * @return Notice the <Notice> instance or null of no notice was
     *         found.
     */
    private Notice findNotice(int uid) {
        Enumeration enm = notices.elements();
        while (enm.hasMoreElements()) {
            Notice notice = (Notice)enm.nextElement();
            if (notice.getUID() == uid) {
                return notice;
            }
        }
        return null;
    }

    /**
     * Retruns all registered notices array. 
     * 
     * @return registered notices array
     */
    public synchronized Notice[] getNotices() {
        Notice[] n = new Notice[notices.size()];
        notices.copyInto(n);
        return n;
    }

    /**
     * Finds first notices from given originator.
     * 
     * 
     * @param origID the originator ID.
     * 
     * @return null if no notice was found.
     */
    public Notice pop(int origID) {
        if (notices.size() > 0) {
            Enumeration enm = notices.elements();
            while (enm.hasMoreElements()) {
                Notice n = (Notice)enm.nextElement();
                if (n.getOriginatorID() == origID) {
                    return n;
                }
            }
        }
        return null;
    }

}

