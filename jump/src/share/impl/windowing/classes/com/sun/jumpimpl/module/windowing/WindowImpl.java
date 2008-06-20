/*
 * %W% %E%
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jumpimpl.module.windowing;

import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.common.JUMPIsolate;
import com.sun.jump.common.JUMPWindow;

import com.sun.jumpimpl.process.JUMPIsolateProxyImpl;

import java.util.TreeMap;
import java.util.Comparator;


public class WindowImpl extends JUMPWindow {
    public static int executiveId = JUMPExecutive.getInstance().getProcessId(); 

    private String      state;
    private int         id;
    private int         isolateId;

    private static long
    getKey(Object o) {
        if(o == null || !(o instanceof WindowImpl)) {
            return 0;
        }

        WindowImpl w = (WindowImpl)o;

        long res = w.id;
        return (res << 32 | w.isolateId);
    }

    // all known windows
    // use TreeMap and not HashMap or something similar to enforce use of
    // custom comparator that ignores WindowImpl#state member field in key
    // generation
    private static TreeMap windows =
        new TreeMap(
            new Comparator() {
                public int
                compare(Object o1, Object o2) {
                    long res = (getKey(o1) - getKey(o2));
                    if(res < 0) {
                        return -1;
                    }
                    if(res > 0) {
                        return 1;
                    }
                    return 0;
                }
            });
    // key to reuse in window look up
    private static WindowImpl key = new WindowImpl(0, 0);

    private WindowImpl(int isolateId, int id) {
        this.id         = id;
        this.isolateId  = isolateId;

        JUMPIsolateProxyImpl isolateProxy = (JUMPIsolateProxyImpl)getIsolate();
        if(isolateProxy != null) {
            isolateProxy.registerWindow(this);
        }
    }

    static synchronized WindowImpl
    getWindow(int isolateId, int id) {
        // don't create key for look up, instead initialize precreated one
        key.id          = id;
        key.isolateId   = isolateId;

        WindowImpl w = (WindowImpl)windows.get(key);
        if(w == null) {
            w = new WindowImpl(isolateId, id);
            windows.put(w, w);
        }

        return w;
    }

    void
    setState(String state) {
        this.state = state;
    }

    public String
    getState() {
        return this.state;
    }

    public int
    getId() {
        return this.id;
    }

    public JUMPIsolate
    getIsolate() {
        if(isolateId == executiveId) {
            // this WindowImpl was created in executive's VM
            return null;
        }

        return JUMPExecutive.getInstance(
            ).getIsolateFactory().getIsolate(isolateId);
    }
}
