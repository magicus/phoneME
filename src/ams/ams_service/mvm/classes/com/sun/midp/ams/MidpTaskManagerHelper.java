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

package com.sun.midp.ams;

import java.util.Vector;

import com.sun.midp.log.LogChannels;
import com.sun.midp.log.Logging;
import com.sun.midp.main.*;
import com.sun.midp.security.*;

/**
 * Class that defines and implements midlet lifecycle management
 * functionality. Up to now is used only by TaskManager serivce.
 *
 */
public final class MidpTaskManagerHelper implements MidpTaskManagerInterface{

    /** Synchronizer to help midlet operation be atomic */
    private static final Obejct lock = new Object();
    /** MIDlet proxy list reference. */
    private MIDletProxyList midletProxyList;

    /** Contructor. */
    public MidpTaskManagerHelper() {
        /* can throw security exception if call by untrusted midlet */
        midletProxyList = MIDletProxyList.getMIDletProxyList();
    }

    public boolean startMidlet(int suiteId, String className) {
        MIDletProxy midlet =
            midletProxyList.findMIDletProxy(suiteId, className);
        if (null != midlet) {
            // causes MIDlet.startApp() call
            // TODO: do we need this?
            midlet.activateMidlet();
        } else {
            MIDletSuiteUtils.execute(suiteId, className, null);
        }

    }

    public boolean stopMidlet(int suiteId, String className) {
        MIDletProxy midlet =
            midletProxyList.findMIDletProxy(suiteId, className);
        if (null != midlet) {
            midlet.destroyMidlet();
            return true;
        } else {
            return false;
        }
    }

    public boolean pauseMidlet(int suiteId, String className) {
        MIDletProxy midlet =
            midletProxyList.findMIDletProxy(suiteId, className);
        if (null != midlet) {
            midlet.pauseMidlet();
            return true;
        } else {
            return false;
        }
    }

    public boolean setForegroundMidlet(int suiteId, String className) {
        return false;
    }

    public MidletInfo getForegroundMidlet() {
        return null;
    }

    public boolean setPriority(int suiteId, String className) {
        return false;
    }

    public RuntimeInfo getMidletInfo(int suiteId, String className) {
        return null;
    }

    public MidletInfo[] getMidletList() {
        MidletInfo[] result = null;
        Vector midletList = new Vector();
        Enumeration midletEnum = midletProxyList.getMIDlets();
        while (midletEnum.hasMoreElements()) {
            midletList.addElement(midletEnum.nextElement());
        }
        if (midletList.size() > 0) {
            result = new MidletInfo[midletList.size()];
            MIDletProxy proxy;
            for (int i = 0; i < midletList.size(); i++){
                proxy = (MIDletProxy)midletList.elementAt(i);
                result[i] = new MidletInfo(proxy.getSuiteId(), proxy.getClassName());
            }
        }
        return result;
    }
}
