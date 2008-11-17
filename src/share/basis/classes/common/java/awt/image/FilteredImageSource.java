/*
 * @(#)FilteredImageSource.java	1.26 06/10/10
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

package java.awt.image;

import java.awt.Image;
import java.awt.image.ImageFilter;
import java.awt.image.ImageConsumer;
import java.awt.image.ImageProducer;
import java.util.Hashtable;
import java.awt.image.ColorModel;

/**
 * This class is an implementation of the ImageProducer interface which
 * takes an existing image and a filter object and uses them to produce
 * image data for a new filtered version of the original image.
 * Here is an example which filters an image by swapping the red and
 * blue compents:
 * <pre>
 * 
 *	Image src = getImage("doc:///demo/images/duke/T1.gif");
 *	ImageFilter colorfilter = new RedBlueSwapFilter();
 *	Image img = createImage(new FilteredImageSource(src.getSource(),
 *							colorfilter));
 * 
 * </pre>
 *
 * @see ImageProducer
 *
 * @version	1.22 08/19/02
 * @author 	Jim Graham
 */
public class FilteredImageSource implements ImageProducer {
    ImageProducer src;
    ImageFilter filter;
    /**
     * Constructs an ImageProducer object from an existing ImageProducer
     * and a filter object.
     * @see ImageFilter
     * @see java.awt.Component#createImage
     */
    public FilteredImageSource(ImageProducer orig, ImageFilter imgf) {
        src = orig;
        filter = imgf;
    }
    private Hashtable proxies;
    /**
     * Adds an ImageConsumer to the list of consumers interested in
     * data for this image.
     * @see ImageConsumer
     */
    public synchronized void addConsumer(ImageConsumer ic) {
        if (proxies == null) {
            proxies = new Hashtable();
        }
        if (!proxies.containsKey(ic)) {
            ImageFilter imgf = filter.getFilterInstance(ic);
            proxies.put(ic, imgf);
            src.addConsumer(imgf);
        }
    }

    /**
     * Determines whether an ImageConsumer is on the list of consumers 
     * currently interested in data for this image.
     * @return true if the ImageConsumer is on the list; false otherwise
     * @see ImageConsumer
     */
    public synchronized boolean isConsumer(ImageConsumer ic) {
        return (proxies != null && proxies.containsKey(ic));
    }

    /**
     * Removes an ImageConsumer from the list of consumers interested in
     * data for this image.
     * @see ImageConsumer
     */
    public synchronized void removeConsumer(ImageConsumer ic) {
        if (proxies != null) {
            ImageFilter imgf = (ImageFilter) proxies.get(ic);
            if (imgf != null) {
                src.removeConsumer(imgf);
                proxies.remove(ic);
                if (proxies.isEmpty()) {
                    proxies = null;
                }
            }
        }
    }

    /**
     * Adds an ImageConsumer to the list of consumers interested in
     * data for this image, and immediately starts delivery of the
     * image data through the ImageConsumer interface.
     * @see ImageConsumer
     */
    public void startProduction(ImageConsumer ic) {
        if (proxies == null) {
            proxies = new Hashtable();
        }
        ImageFilter imgf = (ImageFilter) proxies.get(ic);
        if (imgf == null) {
            imgf = filter.getFilterInstance(ic);
            proxies.put(ic, imgf);
        }
        src.startProduction(imgf);
    }

    /**
     * Requests that a given ImageConsumer have the image data delivered
     * one more time in top-down, left-right order.  The request is
     * handed to the ImageFilter for further processing, since the
     * ability to preserve the pixel ordering depends on the filter.
     * @see ImageConsumer
     */
    public void requestTopDownLeftRightResend(ImageConsumer ic) {
        if (proxies != null) {
            ImageFilter imgf = (ImageFilter) proxies.get(ic);
            if (imgf != null) {
                imgf.resendTopDownLeftRight(src);
            }
        }
    }
}
