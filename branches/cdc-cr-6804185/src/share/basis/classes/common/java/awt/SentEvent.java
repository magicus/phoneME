/*
 * @(#)SentEvent.java	1.8 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package java.awt;

import java.awt.AWTEvent;
import java.awt.ActiveEvent;
import sun.awt.AppContext;
import sun.awt.SunToolkit;

/**
 * A wrapping tag for a nested AWTEvent which indicates that the event was
 * sent from another AppContext. The destination AppContext should handle the
 * event even if it is currently blocked waiting for a SequencedEvent or
 * another SentEvent to be handled.
 *
 * @version 1.8, 10/10/06
 * @author David Mendenhall
 */
class SentEvent extends AWTEvent implements ActiveEvent {
    static final int ID =
	java.awt.event.FocusEvent.FOCUS_LAST + 2;

    boolean dispatched;
    private AWTEvent nested;
    private AppContext toNotify;

    SentEvent() {
	this(null);
    }
    SentEvent(AWTEvent nested) {
	this(nested, null);
    }
    SentEvent(AWTEvent nested, AppContext toNotify) {
	super((nested != null)
	          ? nested.getSource()
	          : Toolkit.getDefaultToolkit(),
	      ID);
	this.nested = nested;
	this.toNotify = toNotify;
    }

    public void dispatch() {
        try {
            if (nested != null) {
                Toolkit.getEventQueue().dispatchEvent(nested);
            }
        } finally {
            dispatched = true;
            if (toNotify != null) {
                SunToolkit.postEvent(toNotify, new SentEvent());
            }
            synchronized (this) {
                notifyAll();
            }
        }
    }
    final void dispose() {
        dispatched = true;
        if (toNotify != null) {
            SunToolkit.postEvent(toNotify, new SentEvent());
        }
        synchronized (this) {
            notifyAll();
        }        
    }
}
