/*
 * @(#)ImageDemo.java	1.6 06/10/10
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
package basis.demos;

import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.net.URL;
import basis.Builder;

public class ImageDemo extends Demo implements MouseMotionListener, MouseListener {
    private Image gif;
    private Image jpg;
    private Image png;
    private Image logo;
    private BufferedImage bi;
    private int[] original;
    private boolean drag;
    private int xorX;
    private int xorY;

    public ImageDemo() {
        gif = loadImage(this, "images/duke.gif");
        jpg = loadImage(this, "images/duke.jpg");
        png = loadImage(this, "images/duke.png");
        logo = loadImage(this, "images/logo.gif");
        GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
        GraphicsDevice gd = ge.getDefaultScreenDevice();
        GraphicsConfiguration gc = gd.getDefaultConfiguration();
        int w = gif.getWidth(this);
        int h = gif.getHeight(this);
        bi = gc.createCompatibleImage(w, h);
        while (bi == null) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException ie) {}
        }
        Graphics big = bi.getGraphics();
        big.setColor(Builder.SUN_RED);
        big.fillRect(0, 0, w, h);
        big.drawImage(gif, 0, 0, w, h, 0, 0, w, h, this);
        addMouseMotionListener(this);
        addMouseListener(this);
    }

    public void paint(Graphics g) {
        Dimension d = getSize();
        int w = d.width / 6;
        int h = d.height / 2;
        g.setColor(Color.black);
        for (int i = 2; i < d.height - 3; i += 4) {
            g.drawLine(0, i, d.width, i);
        }
        int x = 0;
        int y = 0;
        g.drawImage(gif, x, y, w, h, this);
        x += w;
        g.drawImage(jpg, x, y, w, h, this);
        x += w;
        g.drawImage(png, x, y, w, h, this);
        x += w;
        g.drawImage(gif, x, y, w, h, Builder.SUN_BLUE, this);
        x += w;
        g.drawImage(logo, x, y, w, h, Builder.SUN_YELLOW, this);
        x += w;
        g.drawImage(bi, x, y, w, h, this);
        x = 0;
        y += h;
        ((Graphics2D) g).setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.0f));
        g.drawImage(gif, x, y, w, h, this);
        x += w;
        ((Graphics2D) g).setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.2f));
        g.drawImage(gif, x, y, w, h, this);
        x += w;
        ((Graphics2D) g).setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.4f));
        g.drawImage(gif, x, y, w, h, this);
        x += w;
        ((Graphics2D) g).setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.6f));
        g.drawImage(gif, x, y, w, h, this);
        x += w;
        ((Graphics2D) g).setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.8f));
        g.drawImage(gif, x, y, w, h, this);
        x += w;
        ((Graphics2D) g).setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 1.0f));
        g.drawImage(gif, x, y, w, h, this);
    }

    public void mouseDragged(MouseEvent e) {
        Dimension d = getSize();
        int w = d.width / 6;
        int h = d.height / 2;
        Graphics g = getGraphics();
        g.setXORMode(Color.black);
        if (drag) {
            g.drawImage(gif, xorX, xorY, xorX + w, xorY + h, 0, 0, gif.getWidth(this), gif.getHeight(this), this);
        }
        xorX = e.getX() - w / 2;
        xorY = e.getY() - w / 2;
        g.drawImage(gif, xorX, xorY, xorX + w, xorY + h, 0, 0, gif.getWidth(this), gif.getHeight(this), this);
        drag = true;
    }

    public void mouseMoved(MouseEvent e) {}

    public void mouseClicked(MouseEvent e) {
        int w = bi.getWidth(this);
        int h = bi.getHeight(this);
        if (original == null) {
            original = bi.getRGB(0, 0, w, h, null, 0, w);
        }
        int modifier = e.getModifiers();
        if ((modifier & InputEvent.BUTTON2_MASK) != 0) {
            bi.setRGB(0, 0, w, h, original, 0, w);
            repaint();
            return;
        }
        int increment = -20;
        if ((modifier & InputEvent.BUTTON3_MASK) != 0) {
            increment = 20;
        }
        int[] rgb = bi.getRGB(0, 0, w, h, null, 0, w);
        for (int i = 0; i < rgb.length; i++) {
            int r = increment + ((rgb[i] & 0x00FF0000) >> 16);
            int g = increment + ((rgb[i] & 0x0000FF00) >> 8);
            int b = increment + ((rgb[i] & 0x000000FF) >> 0);
            r = r < 255 ? r : 255;
            g = g < 255 ? g : 255;
            b = b < 255 ? b : 255;
            r = r > 0 ? r : 0;
            g = g > 0 ? g : 0;
            b = b > 0 ? b : 0;
            rgb[i] = rgb[i] & 0xFF000000;
            rgb[i] += (r << 16);
            rgb[i] += (g << 8);
            rgb[i] += b;
        }
        bi.setRGB(0, 0, w, h, rgb, 0, w);
        repaint();
    }

    public void mouseEntered(MouseEvent e) {}

    public void mouseExited(MouseEvent e) {}

    public void mousePressed(MouseEvent e) {}

    public void mouseReleased(MouseEvent e) {
        Dimension d = getSize();
        int w = d.width / 6;
        int h = d.height / 2;
        if (drag) {
            Graphics g = getGraphics();
            g.setXORMode(Color.black);
            g.drawImage(gif, xorX, xorY, xorX + w, xorY + h, 0, 0, gif.getWidth(this), gif.getHeight(this), this);
        }
        drag = false;
    }

    static Image loadImage(Component component, String name) {
        Image image = null;
        URL url = null;
        MediaTracker mt = new MediaTracker(component);
        //First try from current directory
        ClassLoader cl = component.getClass().getClassLoader();
        if (cl != null) {
            url = cl.getResource(name);
        }
        //if not found, try CLASSPATH
        if (url == null) {
            url = ClassLoader.getSystemResource(name);
        }
        try {
            image = component.getToolkit().createImage(url);
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            mt.addImage(image, 0);
            mt.waitForAll();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return image;
    }
}

