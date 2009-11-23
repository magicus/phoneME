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

package com.sun.midp.appmanager;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import javax.microedition.lcdui.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.lcdui.Notice;
import com.sun.midp.main.MIDletProxy;
import com.sun.midp.main.NoticeManager;
import com.sun.midp.main.NoticeManagerListener;


/**
 * Displays a small informational note unobtrusive way. 
 * <p> 
 * A {@link Notice} is used by MIDlet to post small portion of 
 * information to the user without taking screen control. 
 *
 */
class NoticeManagerUIImpl extends Form implements CommandListener, ItemCommandListener, NoticeManagerListener  {
    /**
     * <i>Select</i> command for Alert.
     * 
     */
    Command dismiss;

    /**
     * <i>Dismiss</i> command for Alert.
     * 
     */
    Command select;

    /**
     * Notices from given running suite.
     * 
     */
    Vector notices;

    /**
     * The display the form is displayed on.
     * 
     */
    Display display;

    /**
     * The notice manager.
     * 
     */
    NoticeManager manager;

    /**
     * Running midlet suite info.
     * 
     */
    RunningMIDletSuiteInfo rmi;

    /**
     * The form exit command.
     * 
     */
    Command exit;

    /**
     * The form show command.
     * 
     */
    Command show;

    /**
     * Parent displayable
     * 
     */
    Displayable parent;

    /**
     * An arrays of notices display form for different suites.
     * 
     */
    static Hashtable actionCenter;

    /**
     * Private constructor.
     * 
     * 
     * @param d the <code>Display</code> this Form is displayed on.
     * @param p the parent displayable 
     * @param s suite info the notices belong to.
     */
    private NoticeManagerUIImpl(Display d, Displayable p, RunningMIDletSuiteInfo s) {
        super(Resource.getString(ResourceConstants.NOTICE_LIST_WINDOW_TITLE)
                  + s.displayName);
        manager = NoticeManager.getInstance();
        manager.addListener(this);
        display = d;
        parent = p;
        rmi = s;
        setupForm();
    }

    /**
     * Creates and returns Displayable for given suite.
     * 
     * 
     * @param s suite info the notices belong to.
     * @param d the <code>Display</code> this Form is displayed on.
     * @param p the parent displayable 
     * 
     * @return Form 
     */
    static Form getNoticeManagerFor(RunningMIDletSuiteInfo s, Display d, Displayable p) {
        if (null == actionCenter) {
            actionCenter = new Hashtable();
        }
        Form form = (Form)actionCenter.get(s);
        if (null == form) {
            form  = new NoticeManagerUIImpl(d, p, s);
            actionCenter.put(s, form);
        }

        return form;
    }

    /**
     * Creates form.
     * 
     */
    private void setupForm() {
        Notice[] n = manager.getNotices();
        if (null != n) {
            notices = new Vector();
            MIDletProxy[] proxies = rmi.getProxies();
            for (int i = 0; i < n.length; i++) {
                for (int j = 0; j < proxies.length; j++) {
                    if (n[i].getOriginatorID() == proxies[j].getIsolateId()) {
                        notices.addElement(n[i]);
                    }
                }
            }
        }
        exit = new Command(Resource.getString(ResourceConstants.EXIT), Command.EXIT, 1);
        dismiss = new Command(Resource.getString(ResourceConstants.DISMISS), Command.ITEM, 1);
        select = new Command(Resource.getString(ResourceConstants.SELECT), Command.ITEM, 2);
        addCommand(exit);
        setCommandListener(this);
        Enumeration enm = notices.elements();
        while (enm.hasMoreElements()) {
            Notice note = (Notice)enm.nextElement();
            addItem(note);
        }
    }

    /**
     * Adds item for given notice.
     * 
     * 
     * @param n <code>Notice</code>
     */
    private void addItem(Notice n) {
        CustomImageItem item = new CustomImageItem(n);
        item.setItemCommandListener(this);
        append(item);
    }

    /**
     * Informs about new information note.
     *
     * @param notice new information note
     */
    public void notifyNotice(Notice note) {
        MIDletProxy[] proxies = rmi.getProxies();
        for (int j = 0; j < proxies.length; j++) {
            if (note.getOriginatorID() == proxies[j].getIsolateId()) {
                notices.addElement(note);
                addItem(note);
            }
        }
    }

    /**
     * Informs about given information note need to be discarded
     *
     * @param notice information note
     */
    public void removeNotice(Notice note) {
        if (notices.contains(note)) {
            int i = notices.indexOf(note);
            delete(i);
            notices.removeElement(note);
            if (0 == notices.size() && display.getCurrent() != this) {
                // ready for GC
                manager.removeListener(this);
                actionCenter.remove(rmi);
            }
        }
    }

    /**
     * Informs that the notice was updated
     *
     * @param notice the notice was updated
     */
    public void updateNotice(Notice note) {
        if (notices.contains(note)) {
            int i = notices.indexOf(note);
            ImageItem item = (ImageItem)get(i);
            item.setImage(note.getImage());
            item.setLabel(note.getLabel());
            item.removeCommand(select);
            if (note.isSelectable()) {
                item.addCommand(select);
            }
        }
    }

    /**
     * Indicates that a command event has occurred on
     * <code>Displayable d</code>.
     *
     * @param c a <code>Command</code> object identifying the
     * command. This is either one of the
     * applications have been added to <code>Displayable</code> with
     * {@link Displayable#addCommand(Command)
     * addCommand(Command)} or is the implicit
     * {@link List#SELECT_COMMAND SELECT_COMMAND} of
     * <code>List</code>.
     * @param d the <code>Displayable</code> on which this event
     *  has occurred
     */
    public void commandAction(Command c, Displayable d) {
        if (c == exit) {
            manager.removeListener(this);
            actionCenter.remove(rmi);
            // ready for GC
            display.setCurrent(parent);
        }
    }

    /**
     * Called by the system to indicate that a command has been invoked on a 
     * particular item.
     * 
     * @param c the <code>Command</code> that was invoked
     * @param item the <code>Item</code> on which the command was invoked
     */
    public void commandAction(Command c, Item item) {
        if (select == c) {
            ((CustomImageItem)item).notice.select();
        } else {
            ((CustomImageItem)item).notice.dismiss();
        }
    }

    private class CustomImageItem extends ImageItem {
        Notice notice;
        CustomImageItem(Notice n) {
            super(n.getLabel(), n.getImage(), ImageItem.LAYOUT_LEFT, 
                  null, ImageItem.PLAIN);
            if (n.isSelectable()) {
                super.addCommand(select);
            }
            super.addCommand(dismiss);
            notice = n;
        }
    }
}
