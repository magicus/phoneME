/*
 * @(#)PixelStore.java	1.31 06/10/10
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

import java.awt.Image;
import java.awt.image.*;
import java.util.Hashtable;
import sun.misc.Ref;

/**
 * A memory cache object for pixel data.  The actual pixels are stored
 * in a Ref so that they can be freed when necessary.
 *
 * @version 	1.27 08/19/02
 * @author 	Jim Graham
 */
public abstract class PixelStore extends Ref {
    int width;
    int height;
    ColorModel colormodel;
    Hashtable properties;
    boolean seen[];
    int numlines;
    int availinfo;
    int hints;
    public PixelStore() {}

    public PixelStore(int w, int h) {
        setDimensions(w, h);
    }

    public PixelStore(int w, int h, ColorModel cm) {
        setDimensions(w, h);
        setColorModel(cm);
    }

    public synchronized void setDimensions(int w, int h) {
        width = w;
        height = h;
        availinfo |= ImageObserver.WIDTH | ImageObserver.HEIGHT;
    }

    public synchronized void setProperties(Hashtable props) {
        properties = props;
        availinfo |= ImageObserver.PROPERTIES;
    }

    public synchronized void setColorModel(ColorModel cm) {
        colormodel = cm;
    }

    public synchronized void setHints(int h) {
        hints = h;
    }

    protected void recordPixels(int x, int y, int w, int h) {
        numlines = Math.max(numlines, y + h);
        if ((hints & ImageConsumer.TOPDOWNLEFTRIGHT) == 0) {
            if (seen == null) {
                seen = new boolean[height];
            }
            for (int i = 0; i < h; i++) {
                seen[y + i] = true;
            }
        }
    }

    public synchronized boolean setPixels(int x, int y, int w, int h,
        byte pix[], int srcoff, int scansize) {
        recordPixels(x, y, w, h);
        Object[] lines = (Object[]) get();
        if (lines != null) {
            int i = srcoff;
            for (int Y = y; Y < y + h; Y++) {
                byte ras[] = (byte[]) lines[Y];
                int dstoff = offsets[Y] + x;
                System.arraycopy(pix, i, ras, dstoff, w);
                i += scansize;
            }
            availinfo |= ImageObserver.SOMEBITS;
        }
        return (lines != null);
    }

    public synchronized boolean setPixels(int x, int y, int w, int h,
        int pix[], int srcoff, int scansize) {
        recordPixels(x, y, w, h);
        Object[] lines = (Object[]) get();
        if (lines != null) {
            int i = srcoff;
            for (int Y = y; Y < y + h; Y++) {
                int ras[] = (int[]) lines[Y];
                int dstoff = offsets[Y] + x;
                System.arraycopy(pix, i, ras, dstoff, w);
                i += scansize;
            }
            availinfo |= ImageObserver.SOMEBITS;
        }
        return (lines != null);
    }

    public synchronized void imageComplete() {
        if (get() != null && bit_state == BITS_ALLOCATED) {
            hints = (ImageConsumer.TOPDOWNLEFTRIGHT |
                        ImageConsumer.COMPLETESCANLINES |
                        ImageConsumer.SINGLEPASS |
                        ImageConsumer.SINGLEFRAME);
            availinfo |= ImageObserver.ALLBITS;
        }
    }

    public synchronized int getWidth() {
        return width;
    }

    public synchronized int getHeight() {
        return height;
    }

    public synchronized ColorModel getColorModel() {
        return colormodel;
    }

    public synchronized int getBitState() {
        if (bit_state == BITS_ALLOCATED && numlines > 0) {
            // Trigger a get() to make sure the bits are still there.
            Object lines = get();
        }
        return bit_state;
    }

    abstract void replayLines(ImageConsumer ic, int i, int cnt, Object line);

    public synchronized boolean replay(ImageProducer ip, ImageConsumer ic) {
        return replay(ip, ic, true);
    }

    public synchronized boolean replay(ImageProducer ip, ImageConsumer ic,
        boolean full) {
        if (full && (availinfo & ImageObserver.WIDTH) != 0
            && (availinfo & ImageObserver.HEIGHT) != 0) {
            ic.setDimensions(width, height);
            if (!ip.isConsumer(ic)) {
                return true;
            }
        }
        if (full && (availinfo & ImageObserver.PROPERTIES) != 0) {
            ic.setProperties(properties);
        }
        if (full && colormodel != null) {
            ic.setColorModel(colormodel);
            if (!ip.isConsumer(ic)) {
                return true;
            }
        }
        if (hints != 0) {
            ic.setHints(hints & (ImageConsumer.TOPDOWNLEFTRIGHT |
                    ImageConsumer.COMPLETESCANLINES |
                    ImageConsumer.SINGLEPASS |
                    ImageConsumer.SINGLEFRAME));
            if (!ip.isConsumer(ic)) {
                return true;
            }
        }
        Object lines[] = null;
        if (bit_state == BITS_ALLOCATED && numlines > 0) {
            lines = (Object[]) get();
        }
        if (bit_state == BITS_LOST) {
            return false;
        }
        Thread me = Thread.currentThread();
        if (ImageFetcher.isFetcher(me) &&
            me.getPriority() > ImageFetcher.LOW_PRIORITY) {
            me.setPriority(ImageFetcher.LOW_PRIORITY);
            //me.yield();
        }
        if (lines != null) {
            Object prevline = null;
            int prevcount = 0;
            for (int i = 0; i < numlines; i++) {
                if (seen != null && !seen[i]) {
                    if (prevline != null) {
                        replayLines(ic, i - prevcount, prevcount, prevline);
                        if (!ip.isConsumer(ic)) {
                            return true;
                        }
                        prevline = null;
                        prevcount = 0;
                    }
                    continue;
                }
                Object line = lines[i];
                if (prevline != line && prevline != null) {
                    replayLines(ic, i - prevcount, prevcount, prevline);
                    if (!ip.isConsumer(ic)) {
                        return true;
                    }
                    prevcount = 0;
                }
                prevline = line;
                prevcount++;
            }
            if (prevline != null) {
                replayLines(ic, numlines - prevcount, prevcount, prevline);
                if (!ip.isConsumer(ic)) {
                    return true;
                }
            }
        }
        if (full && bit_state == BITS_ALLOCATED &&
            (availinfo & ImageObserver.ALLBITS) != 0) {
            ic.imageComplete(ImageConsumer.STATICIMAGEDONE);
        }
        return true;
    }
    static final int NO_BITS_YET = 0;
    static final int BITS_ALLOCATED = 1;
    static final int BITS_LOST = 2;
    int bit_state = NO_BITS_YET;
    int offsets[];
    abstract Object allocateLines(int num);

    public Object reconstitute() {
        Object[] lines = null;
        if (bit_state == NO_BITS_YET) {
            if (((availinfo & ImageObserver.HEIGHT) == 0)
                || ((availinfo & ImageObserver.WIDTH) == 0)
                || (colormodel == null)) {
                return null;
            }
            // if the ImageProductionMonitor disallows 
            // production, do not reconstitue the 
            // pixelstore.
            // image production is disallowed in situations
            // where we are low in memory.
            //
            if (!ImageProductionMonitor.allowsProduction()) {
                bit_state = BITS_LOST;
                return null;
            }
            bit_state = BITS_ALLOCATED;
            lines = new Object[height];
            offsets = new int[height];
            int i = 0;
            // If we cannot make it fit in 8 chunks
            // we should forget it.  This is to prevent
            // the machine from getting into serious
            // GC turning in very low memory situations.
            //
            int bailoutPoint = height / 8;
            while (i < height) {
                int trylines = height - i;
                Object chunk = null;
                while (chunk == null && trylines > 0) {
                    try {
                        chunk = allocateLines(trylines);
                        if (chunk == null) {
                            break;
                        }
                    } catch (OutOfMemoryError e) {
                        chunk = null;
                        if (trylines / 2 <= bailoutPoint ||
                            !ImageProductionMonitor.allowsProduction()) {
                            break;
                        } 
                        trylines /= 2;
                    }
                }
                if (chunk == null) {
                    bit_state = BITS_LOST;
                    return null;
                }
                int off = 0;
                while (trylines > 0) {
                    lines[i] = chunk;
                    offsets[i] = off;
                    off += width;
                    i++;
                    trylines--;
                }
            }
        } else if (bit_state == BITS_ALLOCATED) {
            // We had some bits, but they're gone now.
            bit_state = BITS_LOST;
        }
        return lines;
    }
}
