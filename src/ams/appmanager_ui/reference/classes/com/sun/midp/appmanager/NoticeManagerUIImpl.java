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
import java.util.Hashtable;
import java.util.Timer;
import java.util.Vector;

import javax.microedition.lcdui.Alert;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.Resource.ResourceConstants;


/**
 * Displays a small informational note unobtrusive way. 
 * <p> 
 * A notice is used by MIDlet post small portion of information 
 * to the user without taking screen control.
 * 
 */
class NoticeManagerUIImpl implements NoticeManagerUI, CommandListener {
    Alert alert;
    Timer timer;
    Command dissmiss;
    Command select;
    Notice notice;
    
    NoticeManagerUIImpl() {
        NoticeManager.getInstance().addListener(this);
    }

    /**
     * Informs about new information note.
     * 
     * @param notice new information note
     */
    public void notifyNotice(Notice note) {
        if (null != alert) {
            return;
        }
        notice = note;
        alert = new Alert(Resource.getString(ResourceConstants.NOTICE_POPUP_TITLE)+
                          notice.getOrigonator(),
                          notice.getLabel(),
                          notice.getImage(),
                          AlertType.INFO);
        int duration = notice.getDuration();
        if (duration > 0) {
            alert.setTimeout(notice.getDuration());
        }
        alert.setCommandListener(this);
        Display.getDisplay(this).setCurrent(alert);
    }

    /**
     * Informs about given information note need to be discarded
     * 
     * @param notice information note
     */
    public void removeNotice(Notice note) {
        if (note.equals(notice) && null != alert) {
            alert.removeCommand(select);
        }
    }

    /**
     * Informs that the notice was updated
     * 
     * @param notice the notice was updated
     */
    public void updateNotice(Notice note) {
        if (notice.equals(note) && null != alert) {
            notice = note;
            alert.setImage(note.getImage());
            alert.setString(note.getLabel());
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
        if (c == dissmiss) {
            if (null != notice) {
                notice.dismiss();
                alert = null;
            }
        }
        if (c == select) {
            if (null != notice) {
                notice.select();
                alert = null;
            }
        }
    }

}
