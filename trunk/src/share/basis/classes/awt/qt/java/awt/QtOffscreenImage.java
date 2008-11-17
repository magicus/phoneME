/*
 * @(#)QtOffscreenImage.java	1.12 06/10/10
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

import java.util.Vector;

import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ImageConsumer;

import java.awt.image.ColorModel;

/** 
@version 1.15, 05/27/02 */


/** An offscreen image created by Component.createImage(int width, int height). As it is not
 possible to create a graphics from an image produced
 through the image producer/consumer model this class exists to allow us to get the graphics
 for an offscreen image. */

class QtOffscreenImage extends QtImage implements ImageProducer {
	Color foreground, background;
	Font font;
	private int originX, originY;

	// We can only have one consumer since we immediately return the data...
	private ImageConsumer theConsumer;
        
        // 4679080 - create instance of props as in JPEG and GIF image architecture
        private java.util.Hashtable props = new java.util.Hashtable();


	QtOffscreenImage (Component component, int width, int height, QtGraphicsConfiguration gc) {
		super(width, height, gc);

		if (component != null) {
			foreground = component.getForeground();
			background = component.getBackground();
			font = component.getFont();
		}
                
                // 4679080 - setProperties as n JEPG and GIF architecture.
                setProperties(props);

		/* Clear image to background color. */

		Graphics g = getGraphics();

		g.clearRect(0, 0, width, height);
		g.dispose();

	}

	public void flush() {};

	public Graphics getGraphics() {
		return new QtGraphics (this);
	}

	public ImageProducer getSource() {
		return this;
	}

	public synchronized void addConsumer(ImageConsumer ic) {
			theConsumer = ic;
			startProduction(ic);
	}

	public synchronized boolean isConsumer(ImageConsumer ic) {
			return (ic == theConsumer);
	}

	public synchronized void removeConsumer(ImageConsumer ic) {
			if(theConsumer==ic)
				   theConsumer = null;
	}

	public void requestTopDownLeftRightResend(ImageConsumer ic) { }

	public void startProduction(ImageConsumer ic) {

		if(ic==null) return;

		int[] rgbArray;
		ColorModel cm = ColorModel.getRGBdefault();

		ic.setDimensions(width, height);
		ic.setColorModel(cm);

		rgbArray = getRGB(originX, originY, width, height, null, 0, width);

		ic.setPixels(0, 0, width, height, cm, rgbArray, 0, width);

		ic.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
	}

}




