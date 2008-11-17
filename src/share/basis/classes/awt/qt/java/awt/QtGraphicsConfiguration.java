/*
 * @(#)QtGraphicsConfiguration.java	1.6 06/10/10
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


import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;


/**
 @version 1.11, 05/03/02 */

abstract class QtGraphicsConfiguration extends GraphicsConfiguration {

    QtGraphicsConfiguration(QtGraphicsDevice device) {
        this.device = device;
    }
	
    public GraphicsDevice getDevice() {
        return device;
    }

    public Rectangle getBounds() {
        return device.getBounds();
    }

    public BufferedImage createCompatibleImage(int width, int height) {
        if(width <= 0 || height <= 0) 
			return null;

		QtOffscreenImage qo = new QtOffscreenImage(null, width, height, this); 
		
       	return createBufferedImageObject(qo, qo.psd);
    }
	
	public java.awt.image.VolatileImage createCompatibleVolatileImage(int width,  int height)
	{
        if(width <= 0 || height <= 0) 
			return null;
		
		QtVolatileImage qv = new QtVolatileImage(width, height, this); 
		
       	return qv;		
		
	}
	
    int getCompatibleImageType() {
        return imageType;
    }

    protected native int createCompatibleImageType(int width, int height, boolean fresh);
    protected native int pClonePSD(int psd);
    protected native int pDisposePSD(int psd);

    int createCompatibleImagePSD(int width, int height) {
		int psd;

		try{
			psd = createCompatibleImageType(width, height, false);
		} catch(OutOfMemoryError e)
		{
			System.gc();
			psd = createCompatibleImageType(width, height, true);
		}

		return psd;
    }

    static native BufferedImage createBufferedImageObject(QtImage image, int psd);

    private QtGraphicsDevice device;
    protected int imageType;
}

