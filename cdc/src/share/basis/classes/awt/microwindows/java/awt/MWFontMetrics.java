/*
 * @(#)MWFontMetrics.java	1.25 06/10/10
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

/**
 * A font metrics object for a font.
 *
 * @version 1.19, 08/19/02
 */
class MWFontMetrics extends FontMetrics {
    // serialVersionUID - 4683929
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
                }
            );
        //dir += s + "lib" + s + "fonts";
        File f = new File(javaHome, "lib" + s + "fonts");
        String dir = f.getAbsolutePath();
        fontNameMap = new HashMap(24);
        /* Initialise table to map standard Java font names and styles to TrueType Fonts */

        fontNameMap.put("serif.0", dir + s + "LucidaBrightRegular.ttf");
        fontNameMap.put("serif.1", dir + s + "LucidaBrightDemiBold.ttf");
        fontNameMap.put("serif.2", dir + s + "LucidaBrightItalic.ttf");
        fontNameMap.put("serif.3", dir + s + "LucidaBrightDemiItalic.ttf");
        fontNameMap.put("sansserif.0", dir + s + "LucidaSansRegular.ttf");
        fontNameMap.put("sansserif.1", dir + s + "LucidaSansDemiBold.ttf");
        fontNameMap.put("sansserif.2", dir + s + "LucidaSansOblique.ttf");
        fontNameMap.put("sansserif.3", dir + s + "LucidaSansDemiOblique.ttf");
        fontNameMap.put("monospaced.0", dir + s + "LucidaTypewriterRegular.ttf");
        fontNameMap.put("monospaced.1", dir + s + "LucidaTypewriterBold.ttf");
        fontNameMap.put("monospaced.2", dir + s + "LucidaTypewriterOblique.ttf");
        fontNameMap.put("monospaced.3", dir + s + "LucidaTypewriterBoldOblique.ttf");
        fontNameMap.put("dialog.0", dir + s + "LucidaSansRegular.ttf");
        fontNameMap.put("dialog.1", dir + s + "LucidaSansDemiBold.ttf");
        fontNameMap.put("dialog.2", dir + s + "LucidaSansOblique.ttf");
        fontNameMap.put("dialog.3", dir + s + "LucidaSansDemiOblique.ttf");
        fontNameMap.put("dialoginput.0", dir + s + "LucidaTypewriterRegular.ttf");
        fontNameMap.put("dialoginput.1", dir + s + "LucidaTypewriterBold.ttf");
        fontNameMap.put("dialoginput.2", dir + s + "LucidaTypewriterOblique.ttf");
        fontNameMap.put("dialoginput.3", dir + s + "LucidaTypewriterBoldOblique.ttf");
        /* This should always exist and is used for fonts whose name is not any of the above. */

        fontNameMap.put("default.0", dir + s + "LucidaSansRegular.ttf");
        fontNameMap.put("default.1", dir + s + "LucidaSansDemiBold.ttf");
        fontNameMap.put("default.2", dir + s + "LucidaSansOblique.ttf");
        fontNameMap.put("default.3", dir + s + "LucidaSansDemiOblique.ttf");
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
    int height;
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
    int maxHeight;
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

    private native int pLoadFont(String fontName, int fontHeight);

    private static native void pDestroyFont(int nativeFont);

    private static native int pCharWidth(int nativeFont, char c);

    private static native int pCharsWidth(int nativeFont, char chars[], int offset, int len);

    private static native int pStringWidth(int nativeFont, String string);
    /** A map which maps a native font name and size to a font metrics object. This is used
     as a cache to prevent loading the same fonts multiple times. */

    private static Map fontMetricsMap = new HashMap();
    /** Gets the MWFontMetrics object for the supplied font. This method caches font metrics
     to ensure native fonts are not loaded twice for the same font. */

    static synchronized MWFontMetrics getFontMetrics(Font font) {
        /* See if metrics has been stored in font already. */

        MWFontMetrics fm = (MWFontMetrics) font.metrics;
        if (fm == null) {
            /* See if a font metrics of the same native name and size has already been loaded.
             If it has then we use that one. */

            String nativeName = (String) fontNameMap.get(font.name.toLowerCase() + "." + font.style);
            if (nativeName == null)
                nativeName = (String) fontNameMap.get("default." + font.style);
            String key = nativeName + "." + font.size;
            fm = (MWFontMetrics) fontMetricsMap.get(key);
            if (fm == null) {
                fontMetricsMap.put(key, fm = new MWFontMetrics(font, nativeName));
            }
            font.metrics = fm;
        }
        return fm;
    }

    /**
     * Creates a font metrics for the supplied font. To get a font metrics for a font
     * use the static method getFontMetrics instead which does caching.
     */
    private MWFontMetrics(Font font, String nativeName) {
        super(font);
        nativeFont = pLoadFont(nativeName, font.size);
        /* Cache first 256 char widths for use by the getWidths method and for faster metric
         calculation as they are commonly used (ASCII) characters. */

        widths = new int[256];
        int i;
        for (i = 0; i < 256; i++) {
            widths[i] = pCharWidth(nativeFont, (char) i);
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
    public int getHeight() {
        return height;
    }

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
        return pCharWidth(nativeFont, c);
    }

    /**
     * Return the width of the specified string in this Font.
     */
    public int stringWidth(String string) {
        return pStringWidth(nativeFont, string);
    }

    /**
     * Return the width of the specified char[] in this Font.
     */
    public int charsWidth(char chars[], int offset, int length) {
        return  pCharsWidth(nativeFont, chars, offset, length);
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
