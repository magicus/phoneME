/*
 * @(#)QtImage.java	1.12 03/01/16
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

package sun.awt.qt;

import java.awt.Component;
import java.awt.Color;
import java.awt.ImageCapabilities;
import java.awt.Toolkit;
import java.awt.GraphicsConfiguration;
import java.awt.image.BufferedImage;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.image.VolatileImage;

import java.awt.Graphics;
import java.awt.Graphics2D;

class QtVolatileImage extends java.awt.image.VolatileImage implements QtDrawableImage 
{
    private QtImage qtimage;
    //6258458
    private static final Component IMAGE_COMPONENT = new java.awt.Canvas();
    //6258458

    QtVolatileImage(int width, int height, GraphicsConfiguration gc)
    {
        // 6258458: Following the same pattern as BufferedImage by passing
        // IMAGE_COMPONENT. This allows the Image.getGraphics() to return
        // the graphics object for all images except for encoded images.
        // (See QtImage.getGraphics() for details)
        qtimage = new QtImage(IMAGE_COMPONENT,
                              width, height, (QtGraphicsConfiguration) gc);
        //6258458
		Graphics g = qtimage.getGraphics();

		g.clearRect(0, 0, width, height);
		g.dispose();
    }

    QtVolatileImage(Component c, int width, int height) {
        // 6262150 : Overlooked this constructor when fixing 6258458
        qtimage = new QtImage(c, width, height);
        // 6262150
		Graphics g = qtimage.getGraphics();
		g.clearRect(0, 0, width, height);
		g.dispose();
    }

    public QtDrawableImage getQtImage() {
        return qtimage;
    }
    
    public int getHeight(ImageObserver observer)
    {
        return qtimage.getHeight(observer);
    }
    
    public java.lang.Object getProperty(java.lang.String name, ImageObserver observer)
    {
        return qtimage.getProperty(name, observer);
    }
    
    public int getWidth(ImageObserver observer)
    {
        return qtimage.getWidth(observer);
    }
    
    public boolean contentsLost()
    {
        return false;
    }
    
    public Graphics2D createGraphics()
    {
        return (Graphics2D)qtimage.getGraphics();
    }
    
    public void flush()
    {
        qtimage.flush();
    }
    
    public ImageCapabilities getCapabilities()
    {
        return new ImageCapabilities(false);
    }
    
    public Graphics getGraphics()
    {
        return qtimage.getGraphics();
        
    }
    
    public boolean isComplete() {
        return qtimage.isComplete();
    }
    
    public int getHeight()
    {
        return qtimage.getHeight();
    }
    
    public BufferedImage getSnapshot()
    {
        //Returns a static snapshot image of this object.
       
        //6199364: Fixed by swapping the width and height 
        BufferedImage img = qtimage.gc.createCompatibleImage(
                            qtimage.getWidth(), qtimage.getHeight());
        Graphics g = img.getGraphics();
        g.drawImage(qtimage, 0, 0, null);
        g.dispose(); // 6240468
        
        return img;
    }
    
    public ImageProducer getSource() {
        return getSnapshot().getSource(); // 6240468
    }
    
    
    public int getWidth() {
        return qtimage.getWidth();
    }
    
    public int validate(GraphicsConfiguration gc)
    {
        return 0;
    }
    
    public boolean draw(QtGraphics g, int x, int y, ImageObserver observer) {
        return qtimage.draw(g, x, y, observer);
    }
	
    public boolean draw(QtGraphics g, int x, int y, int width, int height,
                        ImageObserver observer) {
        return qtimage.draw(g, x, y, width, height, observer);
    }
	
    public boolean draw(QtGraphics g, int x, int y, Color bg,
        ImageObserver observer) {
        return qtimage.draw(g, x, y, bg, observer);
    }
	
    public boolean draw(QtGraphics g, int x, int y, int width, int height,
        Color bg, ImageObserver observer) {
        return qtimage.draw(g, x, y, width, height, bg, observer);
    }
	
    public boolean draw(QtGraphics g, int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        ImageObserver observer) {
        return qtimage.draw(g, dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2, 
                            observer);
    }
	
    public boolean draw(QtGraphics g, int dx1, int dy1, int dx2, int dy2,
        int sx1, int sy1, int sx2, int sy2,
        Color bgcolor, ImageObserver observer) {
        return qtimage.draw(g, dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2, 
                            bgcolor, observer);
    }
}
