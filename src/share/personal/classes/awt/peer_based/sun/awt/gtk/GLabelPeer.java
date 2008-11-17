/*
 * @(#)GLabelPeer.java	1.15 06/10/10
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

package sun.awt.gtk;

import java.awt.*;
import sun.awt.peer.*;

/**
 * The peer for label. We don't use GtkLabel widgets from this peer
 * as they don't support the alignment of text. Instead we simply
 * override the paint method and draw the label in Java.
 *
 * @author Nicholas Allen
 */

public class GLabelPeer extends GComponentPeer implements LabelPeer {
    /** Creates a new GLabelPeer. */

    public GLabelPeer (GToolkit toolkit, Label target) {
        super(toolkit, target);
    }
	
    protected native void create();
	
    public Dimension getPreferredSize() {
        FontMetrics fm = getFontMetrics(target.getFont());
        String label = ((Label) target).getText();
        if (label == null) label = "";
        return new Dimension(fm.stringWidth(label) + 14, 
                fm.getHeight() + 8);
    }
	
    public Dimension getMinimumSize() {
        return getPreferredSize();
    }
	
    private void paintLabel(Graphics g) {
        Label label = (Label) target;
        Dimension size = target.getSize();
        int alignment = label.getAlignment();
        Font font = label.getFont();
        FontMetrics fm = toolkit.getFontMetrics(font);
        String text = label.getText();
        int x;
        Color background = target.getBackground();
        if (background != null) {
            g.setColor(background);
            g.fillRect(0, 0, size.width, size.height);
        }
        /* 4723775 - ensure string is not null */                        
        if (text == null) {
            return;
        }             
        g.setColor(target.getForeground());
        g.setFont(font);
        if (alignment == Label.LEFT)
            x = 0;
        else {
            int textWidth = fm.stringWidth(text);
            x = size.width - textWidth;
            if (alignment == Label.CENTER)
                x /= 2;
        }
        g.drawString(text, x, (size.height - fm.getHeight()) / 2 + fm.getAscent());
    }
	
    public void paint(Graphics g) {
        paintLabel(g);
        target.paint(g);
    }
	
    public void update(Graphics g) {
        paintLabel(g);
        target.update(g);
    }
	
    public void setBackground(Color c) {
        target.repaint();
    }

    public void setForeground(Color c) {
        target.repaint();
    }

    public void setFont(Font f) {
        target.repaint();
    }

    public void setText(String text) {
        target.repaint();
    }

    public void setAlignment(int alignment) {
        target.repaint();
    }
}
