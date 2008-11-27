/*
 * @(#)GraphicsEnvironment.java	1.10 06/10/10
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


package java.awt;

import java.awt.image.BufferedImage;
import java.util.Hashtable;
import java.util.Locale;
import java.util.Map;
import java.io.InputStream;

/**
 *
 * The <code>GraphicsEnvironment</code> class describes the collection
 * of {@link GraphicsDevice} objects and {@link java.awt.Font} objects
 * available to a Java(tm) application on a particular platform.
 * The resources in this <code>GraphicsEnvironment</code> might be local
 * or on a remote machine.  <code>GraphicsDevice</code> objects can be
 * screens, printers or image buffers and are the destination of
 * {@link Graphics2D} drawing methods.  Each <code>GraphicsDevice</code>
 * has a number of {@link GraphicsConfiguration} objects associated with
 * it.  These objects specify the different configurations in which the
 * <code>GraphicsDevice</code> can be used.  
 * @see GraphicsDevice
 * @see GraphicsConfiguration
 * @version 	1.42, 02/02/00
 */

public abstract class GraphicsEnvironment {
    private static GraphicsEnvironment localEnv;
    /**
     * This is an abstract class and cannot be instantiated directly.
     * Instances must be obtained from a suitable factory or query method.
     */
    protected GraphicsEnvironment() {}

    /**
     * Returns the local <code>GraphicsEnvironment</code>.
     * @return this <code>GraphicsEnvironment</code>.
     */
    public static synchronized GraphicsEnvironment getLocalGraphicsEnvironment() {
        if (localEnv == null)
            localEnv = new X11GraphicsEnvironment();
        return localEnv;
    }

    /**
     * Returns an array of all of the screen <code>GraphicsDevice</code>
     * objects.
     * @return an array containing all the <code>GraphicsDevice</code>
     * objects that represent screen devices.
     */
    public abstract GraphicsDevice[] getScreenDevices();

    /**
     * Returns the default screen <code>GraphicsDevice</code>.
     * @return the <code>GraphicsDevice</code> that represents the
     * default screen device.
     */
    public abstract GraphicsDevice getDefaultScreenDevice();

    /**
     * Returns a <code>Graphics2D</code> object for rendering into the
     * specified {@link BufferedImage}.
     * @param img the specified <code>BufferedImage</code>
     * @return a <code>Graphics2D</code> to be used for rendering into
     * the specified <code>BufferedImage</code>.
     */
    // public abstract Graphics2D createGraphics(BufferedImage img);

    /**
     * Returns an array containing a one-point size instance of all fonts
     * available in this <code>GraphicsEnvironment</code>.  Typical usage
     * would be to allow a user to select a particular font.  Then, the
     * application can size the font and set various font attributes by
     * calling the <code>deriveFont</code> method on the choosen instance.
     * <p>
     * This method provides for the application the most precise control
     * over which <code>Font</code> instance is used to render text.
     * If a font in this <code>GraphicsEnvironment</code> has multiple
     * programmable variations, only one
     * instance of that <code>Font</code> is returned in the array, and
     * other variations must be derived by the application.
     * <p>
     * If a font in this environment has multiple programmable variations,
     * such as Multiple-Master fonts, only one instance of that font is
     * returned in the <code>Font</code> array.  The other variations
     * must be derived by the application.
     * @return an array of <code>Font</code> objects.
     * @see #getAvailableFontFamilyNames
     * @see java.awt.Font
     * @see java.awt.Font#deriveFont
     * @see java.awt.Font#getFontName
     * @since 1.2
     */
    // public abstract Font[] getAllFonts();

    /**
     * Returns an array containing the names of all font families available
     * in this <code>GraphicsEnvironment</code>.
     * Typical usage would be to allow a user to select a particular family
     * name and allow the application to choose related variants of the
     * same family when the user specifies style attributes such
     * as Bold or Italic.
     * <p>
     * This method provides for the application some control over which
     * <code>Font</code> instance is used to render text, but allows the 
     * <code>Font</code> object more flexibility in choosing its own best
     * match among multiple fonts in the same font family.
     * @return an array of <code>String</code> containing names of font
     * families.
     * @see #getAllFonts
     * @see java.awt.Font
     * @see java.awt.Font#getFamily
     * @since 1.2
     */
    public abstract String[] getAvailableFontFamilyNames();

    /**
     * Returns an array containing the localized names of all font families
     * available in this <code>GraphicsEnvironment</code>.
     * Typical usage would be to allow a user to select a particular family
     * name and allow the application to choose related variants of the
     * same family when the user specifies style attributes such
     * as Bold or Italic.
     * <p>
     * This method provides for the application some control over which
     * <code>Font</code> instance used to render text, but allows the 
     * <code>Font</code> object more flexibility in choosing its own best
     * match among multiple fonts in the same font family.
     * If <code>l</code> is <code>null</code>, this method returns an 
     * array containing all font family names available in this
     * <code>GraphicsEnvironment</code>.
     * @param l a {@link Locale} object that represents a
     * particular geographical, political, or cultural region
     * @return an array of <code>String</code> objects containing names of
     * font families specific to the specified <code>Locale</code>.
     * @see #getAllFonts
     * @see java.awt.Font
     * @see java.awt.Font#getFamily
     * @since 1.2
     */
    public abstract String[] getAvailableFontFamilyNames(Locale l);
}
