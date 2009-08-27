/*
 * @(#)X11FontMetrics.java	1.17 06/10/10
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

import java.util.*;

/**
 * A font metrics object for a WServer font.
 *
 * @version 1.6, 10/18/01
 */
class X11FontMetrics extends FontMetrics {
    private static native void initIDs();
    private static Map XFontMap;
    static {
        initIDs();
        XFontMap = new HashMap(21);
        /* Table to map magic 5 names * styles to X Fonts
         * To internationalize simply comma seperate further necessary fonts and ensure
         * the appropriate chartobyte convertors are supplied with String() the rest will
         * automatically happen
         */

        XFontMap.put("serif.0", "adobe-times-medium-r-*--*-%d-*");
        XFontMap.put("serif.2", "-adobe-times-medium-i-*--*-%d-*");
        XFontMap.put("serif.1", "-adobe-times-bold-r-*--*-%d-*");
        XFontMap.put("serif.3", "-adobe-times-bold-i-*--*-%d-*");
        XFontMap.put("sansserif.0", "-adobe-helvetica-medium-r-normal--*-%d-*");
        XFontMap.put("sansserif.2", "-adobe-helvetica-medium-i-normal--*-%d-*");
        XFontMap.put("sansserif.1", "-adobe-helvetica-bold-r-normal--*-%d-*");
        XFontMap.put("sansserif.3", "-adobe-helvetica-bold-i-normal--*-%d-*");
        XFontMap.put("monospaced.0", "-adobe-courier-medium-r-*--*-%d-*");
        XFontMap.put("monospaced.2", "-adobe-courier-medium-i-*--*-%d-*");
        XFontMap.put("monospaced.1", "-adobe-courier-bold-r-*--*-%d-*");
        XFontMap.put("monospaced.3", "-adobe-courier-bold-i-*--*-%d-*");
        XFontMap.put("dialog.0", "-adobe-helvetica-medium-r-normal--*-%d-*");
        XFontMap.put("dialog.2", "-adobe-helvetica-medium-i-normal--*-%d-*");
        XFontMap.put("dialog.1", "-adobe-helvetica-bold-r-normal--*-%d-*");
        XFontMap.put("dialog.3", "-adobe-helvetica-bold-i-normal--*-%d-*");
        XFontMap.put("dialoginput.0", "-adobe-courier-medium-r-*--*-%d-*");
        XFontMap.put("dialoginput.2", "-adobe-courier-medium-i-*--*-%d-*");
        XFontMap.put("dialoginput.1", "-adobe-courier-bold-r-*--*-%d-*");
        XFontMap.put("dialoginput.3", "-adobe-courier-bold-i-*--*-%d-*");
        XFontMap.put("default", "-adobe-courier-medium-r-*--*-%d-*");
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
    /* Native font handle */
    int XFontSet;
    private int[] widths;
    /**
     * Loads XFontSet and sets up the Font Metric fields
     */

    private native int pLoadFont(byte[] fontName);

    /**
     * Calculate the metrics from the given WServer and font.
     */
    public X11FontMetrics(Font font) {
        super(font);
        /* Here I should load the right font */
        String XLFD;
        if ((XLFD = (String) XFontMap.get(font.getName().toLowerCase() + "." + font.getStyle())) == null)
            XLFD = (String) XFontMap.get("default");
        StringBuffer newXLFD = new StringBuffer(30);
        int idx1 = 0;
        int idx2;
        while ((idx2 = XLFD.indexOf("%d", idx1)) != -1) {
            newXLFD.append(XLFD.substring(idx1, idx2));
            newXLFD.append(Integer.toString(font.getSize() * 10));
            idx1 = idx2 + 2;
        }
        newXLFD.append(XLFD.substring(idx1));
        XFontSet = pLoadFont(newXLFD.toString().getBytes());
        widths = new int[256];
        byte[] byteCharacter = new byte[1];
        int i;
        for (i = 0; i < 128; i++) {
            byteCharacter[0] = (byte) i;
            widths[i] = bytesWidth(byteCharacter, 0, 1);
        }
        for (; i < widths.length; i++)
            widths[i] = -1;
    }

    static String[] getFontList() {
        Vector fontNames = new Vector();
        Iterator fonts = XFontMap.keySet().iterator();
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
     * Fast lookup of first 127 chars as these are always the same eg. ASCII charset.
     */

    public int charWidth(char c) {
        if (c < 128) return widths[c];
        char[] ca = new char[1];
        ca[0] = c;
        return stringWidth(new String(ca));
    }

    /**
     * Return the width of the specified string in this Font.
     */
    public int stringWidth(String string) {
        byte[] tmpbarr = string.getBytes();
        return pBytesWidth(XFontSet, tmpbarr, 0, tmpbarr.length);
    }

    /**
     * Return the width of the specified char[] in this Font.
     */
    public int charsWidth(char chars[], int offset, int length) {
        String tmpstr = new String(chars).substring(offset, length);
        byte[] tmpbarr = tmpstr.getBytes();
        return  pBytesWidth(XFontSet, tmpbarr, 0, tmpbarr.length);
    }

    /**
     * Return the width of the specified byte[] in this Font.
     */
    public int bytesWidth(byte data[], int offset, int len) {
        return pBytesWidth(XFontSet, data, offset, len);
    }

    private native int pBytesWidth(int fontSet, byte data[], int offset, int len);

    /**
     * Get the widths of the first 256 characters in the font. 
     */
    public int[] getWidths() {
        int[] newWidths = new int[widths.length];
        System.arraycopy(widths, 0, newWidths, 0, newWidths.length);
        return newWidths;
    }
    private static Map table = new HashMap();
    // fix for 4187686 Several class objects are used for synchronization
    private static Object classLock = new Object();
    static X11FontMetrics getFontMetrics(Font font) {
        // fix for 4187686 Several class objects are used for synchronization
        synchronized (classLock) {
            X11FontMetrics fm = (X11FontMetrics) table.get(font);
            if (fm == null) {
                table.put(font, fm = new X11FontMetrics(font));
            }
            return fm;
        }
    }
}
