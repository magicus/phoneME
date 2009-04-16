/*
 * @(#)ImageWatched.java	1.20 06/10/11
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
 *
 */

package sun.awt.image;

import java.util.Vector;
import java.util.Enumeration;
import java.util.NoSuchElementException;
import java.awt.Image;
import java.awt.image.ImageObserver;

public class ImageWatched {
    public ImageWatched() {}
    protected Vector watchers;
    public synchronized void addWatcher(ImageObserver iw) {
        if (iw != null && !isWatcher(iw)) {
            if (watchers == null) {
                watchers = new Vector();
            }
            watchers.addElement(iw);
        }
    }

    public synchronized boolean isWatcher(ImageObserver iw) {
        return (watchers != null && iw != null && watchers.contains(iw));
    }

    public synchronized void removeWatcher(ImageObserver iw) {
        if (iw != null && watchers != null) {
            watchers.removeElement(iw);
            if (watchers.size() == 0) {
                watchers = null;
            }
        }
    }

    public void newInfo(Image img, int info, int x, int y, int w, int h) {
        if (watchers != null) {
            Enumeration enum_ = watchers.elements();
            Vector uninterested = null;
            while (enum_.hasMoreElements()) {
                ImageObserver iw;
                try {
                    iw = (ImageObserver) enum_.nextElement();
                } catch (NoSuchElementException e) {
                    break;
                }
                if (!iw.imageUpdate(img, info, x, y, w, h)) {
                    if (uninterested == null) {
                        uninterested = new Vector();
                    }
                    uninterested.addElement(iw);
                }
            }
            if (uninterested != null) {
                enum_ = uninterested.elements();
                while (enum_.hasMoreElements()) {
                    ImageObserver iw = (ImageObserver) enum_.nextElement();
                    removeWatcher(iw);
                }
            }
        }
    }
}
