/*
 * @(#)ComponentPeer.java	1.37 04/12/20
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

/*
 * NOTE: Changes to this class need to be synchronized with
 * 	 src/share/java/sun.awt.peer/ComponentPeer.java
 */

package sun.awt.peer;

import java.awt.*;
import java.awt.image.ImageProducer;
import java.awt.image.ImageObserver;
import java.awt.image.ColorModel;
import java.awt.image.VolatileImage;

public interface ComponentPeer {
    void    	    	setVisible(boolean b);
    void    	    	setEnabled(boolean b);
    void		paint(Graphics g);
    void		repaint(long tm, int x, int y, int width, int height);
    void		print(Graphics g);
    void		clearBackground(Graphics g);
    void		setBounds(int x, int y, int width, int height);
    void                handleEvent(AWTEvent e);
    Point		getLocationOnScreen();
    Dimension		getPreferredSize();
    Dimension		getMinimumSize();
    ColorModel		getColorModel();
    java.awt.Toolkit	getToolkit();
    Graphics		getGraphics();
    FontMetrics		getFontMetrics(Font font);
    void		dispose();
    void		setForeground(Color c);
    void		setBackground(Color c);
    void		setFont(Font f);
    void 		setCursor(Cursor cursor);
    boolean		requestFocus(Component lightweightChild, 
                                     Window parent, boolean temporary,
                                     boolean focusedWindowChangeAllowed, 
                                     long time);
    boolean		isFocusTraversable();
    void		setFocusable(boolean focusable);
    Image 		createImage(ImageProducer producer);
    Image 		createImage(int width, int height);
    boolean		prepareImage(Image img, int w, int h, ImageObserver o);
    int			checkImage(Image img, int w, int h, ImageObserver o);
    VolatileImage       createVolatileImage(int width, int height); 
}
