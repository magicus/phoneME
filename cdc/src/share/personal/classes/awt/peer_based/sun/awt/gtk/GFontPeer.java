/*
 * @(#)GFontPeer.java	1.16 06/10/10
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

/**
 * GFontPeer.java
 *
 * @author Created by Omnicore CodeGuide
 */

package sun.awt.gtk;

import sun.awt.peer.FontPeer;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Toolkit;
import java.util.*;

import sun.awt.CharsetString;


/** The peer used for fonts in Gtk. This class extends the FontMetrics
 class so that it is very quick to get the font metrics for a font
 and we don't need to create a new object each time. We also maintain a cache
 which allows multiple font objects to share the same peer if they are of the
 same name, style and size. This reduces further the number of objects created. */

class GFontPeer extends FontMetrics implements FontPeer {
    private static int generatedFontCount = 0;
	GPlatformFont gpf;
	
    private GFontPeer (Font font) {
        super(font);
		gpf = new GPlatformFont(font.getName(), font.getStyle(), font.getSize());
		
		/* gtk init of font is done in GPlatformFont */
		
    }
	
    private GFontPeer (int gdkfont) {
        super(null);
//        data = gdkfont;
		gpf = new GPlatformFont(gdkfont);
    }
	
    /** Gets the FontPeer for the specified Font. This will look in a
     cache first to prevent loading of the font for a second time. */
	
    synchronized static GFontPeer getFontPeer(Font font) {
        GFontPeer peer = (GFontPeer) fontToPeerMap.get(font);
        if (peer == null) {
            peer = new GFontPeer (font);
            fontToPeerMap.put(font, peer);
        }
        return peer;
    }
	
    /** Gets the font for the specified GdkFont. This is used when installing fonts on
     the Java components when a widget is created. We check what font the native widget uses and,
     if one has not been explicitly set for the Java component, we set the font on the component.
     This will examine all loaded fonts in the cache and see if there is one for gdkfont.
     If not it will create a new font object with a unique name. */
	
    private synchronized static Font getFont(int gdkfont) {

/*		
			Iterator i = fontToPeerMap.entrySet().iterator();
			while (i.hasNext()) {
				Map.Entry entry = (Map.Entry) i.next();
				GFontPeer peer = (GFontPeer) entry.getValue();
				if (areFontsTheSame(gdkfont, peer.data))
					return (Font) entry.getKey();
			}
*/
		
		Iterator i = fontToPeerMap.entrySet().iterator();
		while (i.hasNext()) {
			Map.Entry entry = (Map.Entry) i.next();
			GFontPeer peer = (GFontPeer) entry.getValue();
			if (peer.gpf.containsGdkFont(gdkfont))
				return (Font) entry.getKey();
		}
		
		// We need to generate a new font for the GdkFont.
			
			GFontPeer peer = new GFontPeer (gdkfont);
			Font font = createFont("GdkFont" + (++generatedFontCount), peer, gdkfont);
			fontToPeerMap.put(font, peer);
			return font;
		
    }
	
    /** Tests whether two gdkfonts are the same. */
	
    static native boolean areFontsTheSame(int font1, int font2);
	
    /** Creates a font with the specified name and gdkfont. */
	
    private static native Font createFont(String name, GFontPeer peer, int gdkfont);
	
    /** Loads the font and sets the ascent and descent fields. */
	
    private native void init(Font font, String nativeName, int size);
	
    /**
     * Determines the <em>font ascent</em> of the font described by this
     * font metric. The font ascent is the distance from the font's
     * baseline to the top of most alphanumeric characters. Some
     * characters in the font may extend above the font ascent line.
     * @return     the font ascent of the font.
     * @see        java.awt.FontMetrics#getMaxAscent
     * @since      JDK1.0
     */
    public int getAscent() {
		return gpf.ascent;
       // return ascent;
    }
	
    /**
     * Determines the <em>font descent</em> of the font described by this
     * font metric. The font descent is the distance from the font's
     * baseline to the bottom of most alphanumeric characters with
     * descenders. Some characters in the font may extend below the font
     * descent line.
     * @return     the font descent of the font.
     * @see        java.awt.FontMetrics#getMaxDescent
     * @since      JDK1.0
     */
    public int getDescent() {
        // return descent;
		return gpf.descent;
    }
	
    /**
     * Determines the <em>standard leading</em> of the font described by
     * this font metric. The standard leading (interline spacing) is the
     * logical amount of space to be reserved between the descent of one
     * line of text and the ascent of the next line. The height metric is
     * calculated to include this extra space.
     * @return    the standard leading of the font.
     * @see       java.awt.FontMetrics#getHeight
     * @see       java.awt.FontMetrics#getAscent
     * @see       java.awt.FontMetrics#getDescent
     * @since     JDK1.0
     */
    public int getLeading() {
        return 0;
    }
	
    /**
     * Gets the maximum advance width of any character in this Font.
     * The advance width is the amount by which the current point is
     * moved from one character to the next in a line of text.
     * @return    the maximum advance width of any character
     *            in the font, or <code>-1</code> if the
     *            maximum advance width is not known.
     * @since     JDK1.0
     */
    public int getMaxAdvance() {
        return -1;
    }
	
    /**
     * Returns the advance width of the specified character in this Font.
     * The advance width is the amount by which the current point is
     * moved from one character to the next in a line of text.
     * @param ch the character to be measured
     * @return    the advance width of the specified <code>char</code>
     *                 in the font described by this font metric.
     * @see       java.awt.FontMetrics#charsWidth
     * @see       java.awt.FontMetrics#stringWidth
     * @since     JDK1.0
     */
    public int charWidth(char ch) {
        // Optimization for ASCII characters. Prevents creating a string and using all the
        // char to byte converters to get the bytes.
		
        if (ch < 128)
            return asciiCharWidth(ch);
        String string = new String(new char[] {ch}
            );
        return stringWidth(string);
    }
	
    private native int asciiCharWidth(char c);
	
    /**
     * Returns the total advance width for showing the specified String
     * in this Font.
     * The advance width is the amount by which the current point is
     * moved from one character to the next in a line of text.
     * @param str the String to be measured
     * @return    the advance width of the specified string
     *                  in the font described by this font metric.
     * @see       java.awt.FontMetrics#bytesWidth
     * @see       java.awt.FontMetrics#charsWidth
     * @since     JDK1.0
     */
    public int stringWidth(String str) {
		int strWidth = 0;
		CharsetString[] cs = gpf.makeMultiCharsetString(str);
		
		for(int i=0;i<cs.length;i++)
		{
			byte[] s = new byte[cs[i].length*3];
			int len;
			
			try {
				len = cs[i].fontDescriptor.fontCharset.convertAny(cs[i].charsetChars, cs[i].offset, cs[i].length, s, 0, s.length);
			} catch(Exception e)
			{
				/* FIXME ... */
				continue;
			}
			
			int gdkfont = gpf.getGdkFont(cs[i].fontDescriptor);
			strWidth += stringWidthNative(s, len, gdkfont);
			
		}
		
		return strWidth;
    }
	
    native int stringWidthNative(byte[] str, int len, int gdkfont);
	
    private static native void initIDs();
    /*
     * Try to load font properties from default locations.
     * If font properties cannot be loaded (possibly because there is no file
     * system present), use a set of en_US properties by default.
     */
    static {
        initIDs();
    }
    /* The map which maps fonts to their peers. */
	
    private static Map fontToPeerMap = new HashMap (10);
    /* Private data used by native code to store the GdkFont. */
	
//    private int data;
    /* The ascent of this font. */
	
//    private int ascent;
    /* The descent of this font. */
	
//    private int descent;
}
