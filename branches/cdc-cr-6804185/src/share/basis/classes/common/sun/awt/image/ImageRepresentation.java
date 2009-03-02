/*
 * @(#)ImageRepresentation.java	1.64 06/10/10
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

import java.awt.Color;
import java.awt.Graphics;
import java.awt.AWTException;
import java.awt.Rectangle;
import java.awt.image.ColorModel;
import java.awt.image.ImageConsumer;
import java.awt.image.ImageObserver;
import sun.awt.image.ImageWatched;
import sun.awt.AWTFinalizeable;
import sun.awt.AWTFinalizer;
import java.util.Hashtable;

public abstract class ImageRepresentation extends ImageWatched
    implements ImageConsumer, AWTFinalizeable {
    private int pData;
    private InputStreamImageSource src;
    Image image;
    private int tag;
    private int width;
    private int height;
    private int hints;
    private int availinfo;
    boolean offscreen;
    private Rectangle newbits;
    /**
     * Create an ImageRepresentation for the given Image scaled
     * to the given width and height and dithered or converted to
     * a ColorModel appropriate for the given image tag.
     */
    public ImageRepresentation(Image im, int t) {
        image = im;
        tag = t;
        if (image.getSource() instanceof InputStreamImageSource) {
            src = (InputStreamImageSource) image.getSource();
        }
    }

    /**
     * Initialize this ImageRepresentation object to act as the
     * destination drawable for this OffScreen Image.
     */
    protected abstract void offscreenInit(Color bg);

    /* TODO: Only used for Frame.setIcon - should use ImageWatcher instead */
    public synchronized void reconstruct(int flags) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        int missinginfo = flags & ~availinfo;
        if ((availinfo & ImageObserver.ERROR) == 0 && missinginfo != 0) {
            numWaiters++;
            try {
                startProduction();
                missinginfo = flags & ~availinfo;
                while ((availinfo & ImageObserver.ERROR) == 0 &&
                    missinginfo != 0) {
                    try {
                        wait();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        return;
                    }
                    missinginfo = flags & ~availinfo;
                }
            } finally {
                decrementWaiters();
            }
        }
    }
	
    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public void setDimensions(int w, int h) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        image.setDimensions(w, h);
        newInfo(image, (ImageObserver.WIDTH | ImageObserver.HEIGHT),
            0, 0, w, h);
        width = w;
        height = h;
        if (width <= 0 || height <= 0) {
            imageComplete(ImageConsumer.IMAGEERROR);
            return;
        }
        availinfo |= ImageObserver.WIDTH | ImageObserver.HEIGHT;
    }

    public void setProperties(Hashtable props) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        image.setProperties(props);
        newInfo(image, ImageObserver.PROPERTIES, 0, 0, 0, 0);
    }

    public void setColorModel(ColorModel model) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
    }

    public void setHints(int h) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        hints = h;
    }

    protected abstract boolean setBytePixels(int x, int y, int w, int h,
        ColorModel model,
        byte pix[], int off, int scansize);

    public void setPixels(int x, int y, int w, int h,
        ColorModel model,
        byte pix[], int off, int scansize) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        boolean ok = false;
        int cx, cy, cw, ch;
        synchronized (this) {
            if (newbits == null) {
                newbits = new Rectangle(0, 0, width, height);
            }
            if (setBytePixels(x, y, w, h, model, pix, off, scansize)) {
                availinfo |= ImageObserver.SOMEBITS;
                ok = true;
            }
            cx = newbits.x;
            cy = newbits.y;
            cw = newbits.width;
            ch = newbits.height;
        }
        if (ok && !offscreen && ((availinfo & ImageObserver.FRAMEBITS) == 0)) {
            newInfo(image, ImageObserver.SOMEBITS, cx, cy, cw, ch);
        }
    }

    protected abstract boolean setIntPixels(int x, int y, int w, int h,
        ColorModel model,
        int pix[], int off, int scansize);

    public void setPixels(int x, int y, int w, int h,
        ColorModel model,
        int pix[], int off, int scansize) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        boolean ok = false;
        int cx, cy, cw, ch;
        synchronized (this) {
            if (newbits == null) {
                newbits = new Rectangle(0, 0, width, height);
            }
            if (setIntPixels(x, y, w, h, model, pix, off, scansize)) {
                availinfo |= ImageObserver.SOMEBITS;
                ok = true;
            }
            cx = newbits.x;
            cy = newbits.y;
            cw = newbits.width;
            ch = newbits.height;
        }
        if (ok && !offscreen && ((availinfo & ImageObserver.FRAMEBITS) == 0)) {
            newInfo(image, ImageObserver.SOMEBITS, cx, cy, cw, ch);
        }
    }
    private boolean consuming = false;
    protected abstract boolean finish(boolean force);

    public void imageComplete(int status) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        boolean done;
        int info;
        switch (status) {
        default:
        case ImageConsumer.IMAGEABORTED:
            done = true;
            info = ImageObserver.ABORT;
            break;

        case ImageConsumer.IMAGEERROR:
            image.addInfo(ImageObserver.ERROR);
            done = true;
            info = ImageObserver.ERROR;
            dispose();
            break;

        case ImageConsumer.STATICIMAGEDONE:
            done = true;
            info = ImageObserver.ALLBITS;
            if (finish(false)) {
                image.getSource().requestTopDownLeftRightResend(this);
                finish(true);
            }
            break;

        case ImageConsumer.SINGLEFRAMEDONE:
            done = false;
            info = ImageObserver.FRAMEBITS;
            break;
        }
        synchronized (this) {
            if (done) {
                image.getSource().removeConsumer(this);
                consuming = false;
                newbits = null;
            }
            availinfo |= info;
            notifyAll();
        }
        if (!offscreen) {
            newInfo(image, info, 0, 0, width, height);
        }
        image.infoDone(status);
    }

    /*synchronized*/ void startProduction() {
        if (!ImageProductionMonitor.allowsProduction()) {
            // we are low on memory so terminate
            // image loading and flag an image error.
            //
            imageComplete(ImageConsumer.IMAGEERROR);
            return;
        }
        if (!consuming) {
            consuming = true;
            image.getSource().startProduction(this);
        }
    }
    private int numWaiters;
    private synchronized void checkConsumption() {
        if (watchers == null && numWaiters == 0
            && ((availinfo & ImageObserver.ALLBITS) == 0)) {
            dispose();
        }
    }

    public synchronized void removeWatcher(ImageObserver iw) {
        super.removeWatcher(iw);
        checkConsumption();
    }

    private synchronized void decrementWaiters() {
        --numWaiters;
        checkConsumption();
    }

    public boolean prepare(ImageObserver iw) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        if ((availinfo & ImageObserver.ERROR) != 0) {
            if (iw != null) {
                iw.imageUpdate(image, ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        boolean done = ((availinfo & ImageObserver.ALLBITS) != 0);
        if (!done) {
            addWatcher(iw);
            startProduction();
            // Some producers deliver image data synchronously
            done = ((availinfo & ImageObserver.ALLBITS) != 0);
        }
        return done;
    }

    public int check(ImageObserver iw) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        if ((availinfo & (ImageObserver.ERROR | ImageObserver.ALLBITS)) == 0) {
            addWatcher(iw);
        }
        return availinfo;
    }

    protected abstract void imageDraw(Graphics g, int x, int y, Color c);

    protected abstract void imageStretch(Graphics g,
        int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color c);

    /*
     * Delegates to the abstract imageStretch() routine to be implemented by the
     * subclasses.  Opportunities also exist for the subclasses to cache the
     * scaled image representations to avoid recomputations.
     */
    protected void imageScale(Graphics g, int x, int y, int w, int h,
                              Color c, boolean done)
    {
        imageStretch(g, x, y, x + w, y + h, 0, 0, width, height, c);
    }

    public boolean drawImage(Graphics g, int x, int y, Color c,
        ImageObserver iw) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        if ((availinfo & ImageObserver.ERROR) != 0) {
            if (iw != null) {
                iw.imageUpdate(image, ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        boolean done = ((availinfo & ImageObserver.ALLBITS) != 0);
        if (!done) {
            addWatcher(iw);
            startProduction();
            // Some producers deliver image data synchronously
            done = ((availinfo & ImageObserver.ALLBITS) != 0);
        }
        imageDraw(g, x, y, c);
        return done;
    }

    public boolean drawStretchImage(Graphics g,
        int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color c, ImageObserver iw) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        if ((availinfo & ImageObserver.ERROR) != 0) {
            if (iw != null) {
                iw.imageUpdate(image, ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        boolean done = ((availinfo & ImageObserver.ALLBITS) != 0);
        if (!done) {
            addWatcher(iw);
            startProduction();
            // Some producers deliver image data synchronously
            done = ((availinfo & ImageObserver.ALLBITS) != 0);
        }
        imageStretch(g, dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2, c);
        return done;
    }

    public boolean drawScaledImage(Graphics g,
        int x, int y, int w, int h,
        Color c, ImageObserver iw) {
        if (src != null) {
            src.checkSecurity(null, false);
        }
        if ((availinfo & ImageObserver.ERROR) != 0) {
            if (iw != null) {
                iw.imageUpdate(image, ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        boolean done = ((availinfo & ImageObserver.ALLBITS) != 0);
        if (!done) {
            addWatcher(iw);
            startProduction();
            // Some producers deliver image data synchronously
            done = ((availinfo & ImageObserver.ALLBITS) != 0);
        }
        if ((~availinfo & (ImageObserver.WIDTH | ImageObserver.HEIGHT)) != 0) {
            return false;
        }
        if (w < 0) {
            if (h < 0) {
                w = width;
                h = height;
            } else {
                w = h * width / height;
            }
        } else if (h < 0) {
            h = w * height / width;
        }
        if (w == width && h == height) {
            imageDraw(g, x, y, c);
        } else {
            imageScale(g, x, y, w, h, c, done);
        }
        return done;
    }

    //
    // Check with all our ImageObservers to see if anybody is still interested.
    // If not, remove ourselves as a consumer.  cf.
    // InputStreamImageSource.proactivelyUpdateObservers()
    //
    void checkForInterest() {
        newInfo(image, 0, -1, -1, -1, -1);
    }

    protected abstract void disposeImage();

    synchronized void abort() {
        image.getSource().removeConsumer(this);
        consuming = false;
        newbits = null;
        disposeImage();
        newInfo(image, ImageObserver.ABORT, -1, -1, -1, -1);
        availinfo &= ~(ImageObserver.SOMEBITS
                    | ImageObserver.FRAMEBITS
                    | ImageObserver.ALLBITS
                    | ImageObserver.ERROR);
    }

    synchronized void dispose() {
        image.getSource().removeConsumer(this);
        consuming = false;
        newbits = null;
        disposeImage();
        availinfo &= ~(ImageObserver.SOMEBITS
                    | ImageObserver.FRAMEBITS
                    | ImageObserver.ALLBITS);
    }
    AWTFinalizeable finalnext;
    public void finalize() {
        AWTFinalizer.addFinalizeable(this);
    }

    public void doFinalization() {
        disposeImage();
    }

    public void setNextFinalizeable(AWTFinalizeable o) {
        finalnext = o;
    }

    public AWTFinalizeable getNextFinalizeable() {
        return finalnext;
    }
}
