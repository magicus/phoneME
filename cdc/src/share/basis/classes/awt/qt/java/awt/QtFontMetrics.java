/*
 * @(#)QtFontMetrics.java	1.8 06/10/10
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

import java.io.*;
import java.security.*;
import java.util.*;

import java.awt.font.TextAttribute;


/**
 * A font metrics object for a font.
 *
 * @version 1.18, 05/14/02
 */
class QtFontMetrics extends FontMetrics {
    
    static final long serialVersionUID = -4956160226949100590L;    
    
    /** A map which maps from Java logical font names and styles to native font names. */

    private static Map fontNameMap;

    private static native void initIDs();

    static {
        initIDs();
        String s = File.separator;
        String javaHome = (String) AccessController.doPrivileged(new PrivilegedAction() {
            public Object run() {
                return System.getProperty("java.home");
            }
        });
        // String dir = "lib" + s + "fonts";
        File f = new File(javaHome, "lib" + s + "fonts");
        // String dir = f.getAbsolutePath();
        fontNameMap = new HashMap(10);

	String dir = ""; s = "";

        /* Initialise table to map standard Java font names and styles to TrueType Fonts */

        //6177148
        // Changed all the physical font family names to the names that
        // Qt can understand. We were having "times", "helvetica" etc
        // as the physical font names and since it did not match what qt
        // expects, we were rendering with some font that qt picked up.
        // Following J2SE's convention of mapping 
        // "monospaced" and "dialoginput" has MonoSpaced Font (Courier)
        // "sansserif"  and "sanserif" has a sans serif Font (We just use
        //  "Sans Serif" for qt to pick some San Serif font. and the
        // "default" font is a Sans Serif font
        fontNameMap.put("serif", dir + s + "Serif");

        fontNameMap.put("sansserif", dir + s + "Sans Serif");

        fontNameMap.put("monospaced", dir + s + "Courier");

        fontNameMap.put("dialog", dir + s + "Sans Serif");

        fontNameMap.put("dialoginput", dir + s + "Courier");

        /* This should always exist and is used for fonts whose name is not any of the above. */

        fontNameMap.put("default", dir + s + "Sans Serif");
        //6177148
    }

    /**
     * The standard ascent of the font.  This is the logical height
     * above the baseline for the Alphanumeric characters and should
     * be used for determining line spacing.  Note, however, that some
     * characters in the font may extend above this height.
     */
    int ascent;

    /**
     * The standard descent of the font.  This is the logical height
     * below the baseline for the Alphanumeric characters and should
     * be used for determining line spacing.  Note, however, that some
     * characters in the font may extend below this height.
     */
    int descent;

    /**
     * The standard leading for the font.  This is the logical amount
     * of space to be reserved between the descent of one line of text
     * and the ascent of the next line.  The height metric is calculated
     * to include this extra space.
     */
    int leading;

    /**
     * The standard height of a line of text in this font.  This is
     * the distance between the baseline of adjacent lines of text.
     * It is the sum of the ascent+descent+leading.  There is no
     * guarantee that lines of text spaced at this distance will be
     * disjoint; such lines may overlap if some characters overshoot
     * the standard ascent and descent metrics.
     */
    // 6286830
    /*
    int height;
    */

    /**
     * The maximum ascent for all characters in this font.  No character
     * will extend further above the baseline than this metric.
     */
    int maxAscent;

    /**
     * The maximum descent for all characters in this font.  No character
     * will descend further below the baseline than this metric.
     */
    int maxDescent;

    /**
     * The maximum possible height of a line of text in this font.
     * Adjacent lines of text spaced this distance apart will be
     * guaranteed not to overlap.  Note, however, that many paragraphs
     * that contain ordinary alphanumeric text may look too widely
     * spaced if this metric is used to determine line spacing.  The
     * height field should be preferred unless the text in a given
     * line contains particularly tall characters.
     */
    // Code cleanup while fixing 6286830
    /*
    int maxHeight;
    */

    /**
     * The maximum advance width of any character in this font.
     */
    int maxAdvance;

    /** Native font handle */

    int nativeFont;

    /** Cache of first 256 Unicode characters as these map to ASCII characters and are often used. */

    private int[] widths;

    /**
     * Loads MicroWindows Font and sets up the Font Metric fields
     */

    private native int pLoadFont(String fontName, int fontHeight, int fontStyle, boolean st, boolean ul);

    private static native void pDestroyFont(int nativeFont);

    private static native int pStringWidth(int nativeFont, String string);

    /** A map which maps a native font name and size to a font metrics object. This is used
     as a cache to prevent loading the same fonts multiple times. */

    private static Map fontMetricsMap = new HashMap();

    /** Gets the FontMetrics object for the supplied font. This method caches font metrics
     to ensure native fonts are not loaded twice for the same font. */

    static synchronized QtFontMetrics getFontMetrics(Font font, boolean backwardCompat) {

        /* See if metrics has been stored in font already. */

        QtFontMetrics fm = (QtFontMetrics) font.metrics;

        if (fm == null) {
			boolean strikethrough = false, underline = false;

            /* See if a font metrics of the same native name and size has already been loaded.
             If it has then we use that one. */

            String nativeName = (String) fontNameMap.get(font.name.toLowerCase());

            if (nativeName == null)
                nativeName = (String) fontNameMap.get("default");

            String key = nativeName + "." + font.style + "." + font.size;

			if(!backwardCompat)
			{
				Map fa = font.getAttributes();
				Object obj;
				
				if ((obj = fa.get(TextAttribute.STRIKETHROUGH)) != null) {
					if(obj.equals(TextAttribute.STRIKETHROUGH_ON)) {
						key += ".s";
						strikethrough = true;
					}
				}

				if ((obj = fa.get(TextAttribute.UNDERLINE)) != null) {
					if(obj.equals(TextAttribute.UNDERLINE_ON)) {
						key += ".u";
						underline = true;
					}
				}
			}
			
            fm = (QtFontMetrics) fontMetricsMap.get(key);

            if (fm == null) {
                fontMetricsMap.put(key, fm = new QtFontMetrics(font, nativeName, font.style, strikethrough, underline));
            }

            font.metrics = fm;
        }

        return fm;
    }

    /**
     * Creates a font metrics for the supplied font. To get a font metrics for a font
     * use the static method getFontMetrics instead which does caching.
     */
    private QtFontMetrics(Font font, String nativeName, int style, boolean st, boolean ul) {

        super(font);

	    nativeFont = pLoadFont(nativeName, font.size, style, st, ul);
		
        /* Cache first 256 char widths for use by the getWidths method and for faster metric
         calculation as they are commonly used (ASCII) characters. */

        widths = new int[256];

        int i;

		for (i = 0; i < 256; i++) {
		    widths[i] = pStringWidth(nativeFont, new String(new char[]{(char)i}));
		}
    }

    static String[] getFontList() {
        Vector fontNames = new Vector();
        Iterator fonts = fontNameMap.keySet().iterator();
        int dotidx;

        while (fonts.hasNext()) {
            String fontname = (String) fonts.next();

            if ((dotidx = fontname.indexOf('.')) == -1)
                dotidx = fontname.length();

            fontname = fontname.substring(0, dotidx);

            if (!fontNames.contains(fontname))
                fontNames.add(fontname);

        }

        String[] tmpFontNames = new String[fontNames.size()];

        System.arraycopy(fontNames.toArray(), 0, tmpFontNames, 0, tmpFontNames.length);

        return tmpFontNames;

    }

    /**
     * Get leading
     */
    public int getLeading() {
        return leading;
    }

    /**
     * Get ascent.
     */
    public int getAscent() {
        return ascent;
    }

    /**
     * Get descent
     */
    public int getDescent() {
        return descent;
    }

    /**
     * Get height
     */
    // 6286830
    // QtFontMetrics::height() is different from AWT's idea of
    // java.awt.FontMetrics.getHeight().  So don't override it.
    /*
    public int getHeight() {
        return height;
    }
    */

    /**
     * Get maxAscent
     */
    public int getMaxAscent() {
        return maxAscent;
    }

    /**
     * Get maxDescent
     */
    public int getMaxDescent() {
        return maxDescent;
    }

    /**
     * Get maxAdvance
     */
    public int getMaxAdvance() {
        return maxAdvance;
    }

    /**
     * Fast lookup of first 256 chars as these are always the same eg. ASCII charset.
     */

    public int charWidth(char c) {
        if (c < 256)
            return widths[c];

	    return pStringWidth(nativeFont, new String(new char[]{c}));
    }

    /**
     * Return the width of the specified string in this Font.
     */
    public int stringWidth(String string) {
		
		if(string==null) throw new NullPointerException("String can't be null");
		
	    return pStringWidth(nativeFont, string);
	}

    /**
		* Return the width of the specified char[] in this Font.
     */
    public int bytesWidth(byte chars[], int offset, int length) {
		if(chars==null) throw new NullPointerException("Byte array can't be null");

		if( (offset<0) || (offset+length<0) || (offset+length>chars.length) )
			throw new ArrayIndexOutOfBoundsException("offset or length inappropriate");
		
	    return pStringWidth(nativeFont, new String(chars, offset, length));
	}	
	
	
    /**
     * Return the width of the specified char[] in this Font.
     */
    public int charsWidth(char chars[], int offset, int length) {
		if(chars==null) throw new NullPointerException("Char array can't be null");

		if( (offset<0) || (offset+length<0) || (offset+length>chars.length) )
			throw new ArrayIndexOutOfBoundsException("offset or length inappropriate");		
		
	    return pStringWidth(nativeFont, new String(chars, offset, length));
	}

    /**
     * Get the widths of the first 256 characters in the font. 
     */
    public int[] getWidths() {

        int[] newWidths = new int[256];

        System.arraycopy(widths, 0, newWidths, 0, 256);

        return newWidths;
    }

    protected void finalize() throws Throwable {
		pDestroyFont(nativeFont);
        super.finalize();
    }
}

