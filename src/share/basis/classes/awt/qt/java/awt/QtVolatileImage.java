/*
 * @(#)QtVolatileImage.java	1.7 06/10/10
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

import java.awt.*;
import java.awt.image.*;

/**
 Qt volatile image implementation.
 @version 1.00 08/25/04
*/



class QtVolatileImage extends java.awt.image.VolatileImage
{
	
	QtOffscreenImage qtimage;
	
	
	QtVolatileImage(int width, int height, GraphicsConfiguration gc)
	{
		qtimage = new QtOffscreenImage((Component)null, width, height, (QtGraphicsConfiguration) gc);
		
	}
	
	public int getHeight(ImageObserver observer)
	{
		return qtimage.getHeight(observer);
	}
		
	public java.lang.Object getProperty(java.lang.String name, ImageObserver observer)
	{
		return qtimage.getProperty(name, observer);
	}

// 6240967
    // The scaled instance should be a snapshot of "this" 
    // (i.e BufferedImage) and the contents should be immediately 
    // available. By using the Image.getScaledInstance() we acheive the
    // desired effect (See also QtVolatileImage.getSource())
// 	public Image getScaledInstance(int width, int height, int hints)
// 	{
// 		return qtimage.getScaledInstance(width, height, hints);
// 	}		
// 6240967
			
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
					
	public int getHeight()
	{
		return qtimage.getHeight();
	}
						
	public BufferedImage getSnapshot()
	{
		//	Returns a static snapshot image of this object.
		
		QtOffscreenImage img = new QtOffscreenImage((Component)null, qtimage.width, qtimage.height, qtimage.gc);
		Graphics g = img.getGraphics();
		g.drawImage(qtimage, 0, 0, null);
        g.dispose(); // 6240967
		
		return QtGraphicsConfiguration.createBufferedImageObject(img, img.psd);
	}
						
	public ImageProducer getSource() {
        return getSnapshot().getSource(); // 6240967
	} 
	
							
	public int getWidth() {
		return qtimage.getWidth();
	} 
								
	public int validate(GraphicsConfiguration gc)
	{
		return 0;
	}

}


