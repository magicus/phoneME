/*
 * @(#)Bean.java	1.5 06/10/10
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
package basis;

import java.awt.*;
import java.beans.*;
import java.io.Serializable;

public class Bean extends Component implements Serializable {
    private static final long serialVersionUID = 8683961066421525018L;
    private PropertyChangeSupport pcs;
    private VetoableChangeSupport vcs;

    public Bean() {
        super.setName("Bean");
        super.setBackground(Builder.SUN_RED);
        super.setForeground(Color.black);
        super.setSize(60, 20);
        pcs = new PropertyChangeSupport(this);
        vcs = new VetoableChangeSupport(this);
    }

    public void addPropertyChangeListener(PropertyChangeListener listener) {
        pcs.addPropertyChangeListener(listener);
    }

    public void removePropertyChangeListener(PropertyChangeListener listener) {
        pcs.removePropertyChangeListener(listener);
    }

    public void addVetoableChangeListener(VetoableChangeListener listener) {
        vcs.addVetoableChangeListener(listener);
    }

    public void removeVetoableChangeListener(VetoableChangeListener listener) {
        vcs.removeVetoableChangeListener(listener);
    }

    public void setText(String text) throws PropertyVetoException {
        String oldText = getName();
        // NOTE: The vetoableChange method of the listeners will not be called
        // if the old and new values are the same...
        vcs.fireVetoableChange("Text", oldText, text);
        super.setName(text);
        pcs.firePropertyChange("Text", oldText, text);
    }

    public void setColor(Color color) throws PropertyVetoException {
        Color oldColor = getBackground();
        vcs.fireVetoableChange("Color", oldColor, color);
        super.setBackground(color);
        pcs.firePropertyChange("Color", oldColor, color);
    }

    public void setDimension(Dimension dimension) throws PropertyVetoException {
        Dimension oldDimension = getSize();
        vcs.fireVetoableChange("Dimension", oldDimension, dimension);
        super.setSize(dimension);
        pcs.firePropertyChange("Dimension", oldDimension, dimension);
    }

    public void setPosition(Point position) throws PropertyVetoException {
        Point oldPosition = getLocation();
        vcs.fireVetoableChange("Position", oldPosition, position);
        super.setLocation(position);
        pcs.firePropertyChange("Position", oldPosition, position);
    }

    public Dimension getPreferredSize() {
        return getSize();
    }

    public Dimension getMinimumSize() {
        return getSize();
    }

    public Dimension getMaximumSize() {
        return new Dimension(Integer.MAX_VALUE, Integer.MAX_VALUE);
    }

    public void paint(Graphics g) {
        String text = getName();
        Color color = getBackground();
        Dimension dimension = getSize();
        g.setColor(color);
        g.fillRect(0, 0, dimension.width, dimension.height);
        g.setColor(Color.black);
        Font font = new Font("sanserif", Font.PLAIN, 12);
        g.setFont(font);
        FontMetrics fm = g.getFontMetrics(font);
        int w = fm.stringWidth(text);
        int h = fm.getHeight();
        g.drawString(text, (dimension.width - w) / 2, h + (dimension.height - h) / 2 - 3);
    }
}

