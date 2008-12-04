/*
 * @(#)OffScreenImageSource.java	1.19 06/10/10
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

import java.util.Hashtable;
import java.awt.Component;
import java.awt.image.ImageConsumer;
import java.awt.image.ImageProducer;
import java.awt.image.ColorModel;

public class OffScreenImageSource implements ImageProducer {
    Component target;
    int width;
    int height;
    ImageRepresentation baseIR;
    public OffScreenImageSource(Component c, int w, int h) {
        target = c;
        width = w;
        height = h;
    }

    void setImageRep(ImageRepresentation ir) {
        baseIR = ir;
    }

    public ImageRepresentation getImageRep() {
        return baseIR;
    }
    // We can only have one consumer since we immediately return the data...
    private ImageConsumer theConsumer;
    public synchronized void addConsumer(ImageConsumer ic) {
        theConsumer = ic;
        produce();
    }

    public synchronized boolean isConsumer(ImageConsumer ic) {
        return (ic == theConsumer);
    }

    public synchronized void removeConsumer(ImageConsumer ic) {
        if (theConsumer == ic) {
            theConsumer = null;
        }
    }

    public void startProduction(ImageConsumer ic) {
        addConsumer(ic);
    }

    public void requestTopDownLeftRightResend(ImageConsumer ic) {}

    private native void sendPixels();

    private void produce() {
        if (theConsumer != null) {
            theConsumer.setDimensions(width, height);
        }
        if (theConsumer != null) {
            theConsumer.setProperties(new Hashtable());
        }
        sendPixels();
        if (theConsumer != null) {
            theConsumer.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
        }
    }
}
