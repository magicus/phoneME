/*
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

package sun.awt.pocketpc;

import java.awt.*;
import java.util.Hashtable;
import sun.awt.FontDescriptor;
import sun.awt.PlatformFont;
import sun.awt.CharsetString;
import java.io.CharConversionException;
import sun.io.CharToByteConverter;
import sun.awt.PeerBasedToolkit;

/** 
 * A font metrics object for a PPCServer font.
 * 
 * @version 1.1 Dec 16, 2002
 */
class PPCFontMetrics extends FontMetrics {
    /**
     * The widths of the first 256 characters.
     */
    int widths[];

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

    private static native void initIDs();
    static {
        initIDs();
    }
    /**
     * Calculate the metrics from the given PPCServer and font.
     */
    public PPCFontMetrics(Font font) {
	super(font);
	init();
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
     * Return the width of the specified string in this Font. 
     */
    public int stringWidth(String string) {
	return charsWidth(string.toCharArray(), 0, string.length());
    }

    /**
     * Return the width of the specified char[] in this Font.
     */
    public int charsWidth(char chars[], int offset, int length) {
        if (chars == null ) {
            throw new NullPointerException("chars[] is null");
        }
        if ((length<0) || (offset < 0) || (length+offset > chars.length) 
            || (length > chars.length) || (offset > chars.length)) {
            throw new ArrayIndexOutOfBoundsException("off or len");
        }
	Font font = getFont();
        PlatformFont pf = ((PlatformFont) ((PeerBasedToolkit) Toolkit.getDefaultToolkit()).getFontPeer(font));
	//PlatformFont pf = ((PlatformFont) font.getPeer());
	if (pf.mightHaveMultiFontMetrics()) {
	    return getMFStringWidth(chars, offset, length, font, pf);
	} else {
	    if (widths != null) {
		int w = 0;
		for (int i = offset; i < offset + length; i++) {
		    int ch = chars[i];
		    if (ch < 0 || ch >= widths.length) {
			w += maxAdvance;
		    } else {
			w += widths[ch];
		    }
		}
		return w;
	    } else {
		return maxAdvance * length;
	    }
        }
    }

    /*
     * get multi font string width with a multiple native font
     */
    private int getMFStringWidth(char string[], int offset, int length, 
				 Font font, PlatformFont pf) {
        if (length == 0) {
            return 0;
        }

	sun.awt.CharsetString[] css = 
            pf.makeMultiCharsetString(string, offset, length);

	byte[] csb = null;
        int baLen = 0;
	int w = 0;
	for (int i = 0; i < css.length; i++) {
	    CharsetString cs = css[i];
            if (needsConversion(font, cs.fontDescriptor)) {
                try {
                    int l = cs.fontDescriptor.fontCharset.getMaxBytesPerChar()
                        * cs.length;
                    if (l > baLen) {
                        baLen = l;
                        csb = new byte[baLen];
                    }
                 // use a new instance of the fontCharset (bug 4078870)
				 CharToByteConverter cv = (CharToByteConverter)
					 cs.fontDescriptor.fontCharset.getClass().newInstance();
                 int csbLen = cv.convert(cs.charsetChars, 
								  cs.offset, cs.offset + cs.length,
								  csb, 0, csb.length);
								  
		
                    w += getMFCharSegmentWidth(font, cs.fontDescriptor, true,
                                               null, 0, 0, csb, csbLen);
                } catch (InstantiationException ignored) {
				} catch (IllegalAccessException ignored) {
					// CharToByte converters are public and can be instantiated
					// so no exceptions should be thrown
                } catch (CharConversionException ignored) {
                }
            } else {
                w += getMFCharSegmentWidth(font, cs.fontDescriptor, false,
                                           cs.charsetChars, cs.offset, 
                                           cs.length, null, 0);
            }
	}
	return w;
    }

    static native boolean needsConversion(Font font, FontDescriptor des);

    // Get the width of the char segment, which is in the native charset
    // of font.
    private native int getMFCharSegmentWidth(
        Font font, FontDescriptor des, boolean converted, 
        char[] chars, int offset, int charLength,
        byte[] bytes, int byteLength);

    /**
     * Return the width of the specified byte[] in this Font. 
     */
    public native int bytesWidth(byte data[], int offset, int len);

    /**
     * Get the widths of the first 256 characters in the font.
     */
    public int[] getWidths() {
	return widths;
    }

    native void init();

    static Hashtable table = new Hashtable();
    
    static FontMetrics getFontMetrics(Font font) {
	FontMetrics fm = (FontMetrics)table.get(font);
	if (fm == null) {
	    table.put(font, fm = new PPCFontMetrics(font));
	}
	return fm;
    }
}
