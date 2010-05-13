/*
 * %W% %E%
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
 * GPlatformFont.java
 *
 */

package sun.awt.gtk;

import sun.awt.peer.FontPeer;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Toolkit;
import java.util.*;

import sun.awt.PlatformFont;
import sun.awt.FontDescriptor;
import sun.awt.CharsetString;
import sun.io.CharToByteConverter;

class GPlatformFont extends PlatformFont {
	
    private static final String DEFAULT_NATIVE_FONT_NAME = "-adobe-courier-medium-r-normal--0-%d-0-0-m-0-iso8859-1";
    private static String defaultNativeFontName = null;
		
	private HashMap fontDescriptorToGdkFont = new HashMap(20);
	
	private native int init(String gdkFontName, int size);
	
	int ascent, descent;
	
	GPlatformFont(String name, int style, int size) {
		super(name, style);
		
		for(int i=0; i< componentFonts.length; i++) {
			int gdkfont = init(getNativeName(componentFonts[i]), size);
			fontDescriptorToGdkFont.put(componentFonts[i], new Integer(gdkfont));
		}

	}
	
	GPlatformFont(int gdkFont) {
		componentFonts = new FontDescriptor[1];
		componentFonts[0] = new FontDescriptor("SystemAssigned", null, null);
		fontDescriptorToGdkFont.put(componentFonts[0], new Integer(gdkFont));		
	}
	
	int getGdkFont(FontDescriptor fd) {
		Integer gdkFont = (Integer)(fontDescriptorToGdkFont.get(fd));
		return gdkFont!=null? gdkFont.intValue() : 0;
	}
	
	boolean containsGdkFont(int gdkFont) {
		Iterator i = fontDescriptorToGdkFont.entrySet().iterator();
		while (i.hasNext()) {
			Map.Entry entry = (Map.Entry) i.next();
			Integer peer = (Integer) entry.getValue();
			if (GFontPeer.areFontsTheSame(gdkFont, peer.intValue()))
				return true;
		}
		
		return false;
	}
	
	protected CharToByteConverter getFontCharset(String charsetName, String fontName) {
		System.out.println("GPLATFORMFONT: (cn) :"+charsetName);
		System.out.println("GPLATFORMFONT: (fn) :"+fontName);
		
		if(!charsetName.equalsIgnoreCase("default"))
		{
			try {
				return ((sun.io.CharToByteConverter)(Class.forName(charsetName)).newInstance());
			}
			catch(Exception e) {
				System.err.println("Can't find an encoder for " + charsetName + " " + e);
			}
		}
		
		return CharToByteConverter.getDefault();
		
	}

	static String[] getAvailableFontFamilyNames() {
		
		/* Call the static version in GPlatformFont */
		
        List fontNames = new ArrayList();
        Iterator fonts = fprops.keySet().iterator();
        int dotidx;
        while (fonts.hasNext()) {
            String fontName = (String) fonts.next();
            if ((dotidx = fontName.indexOf('.')) == -1)
                dotidx = fontName.length();
            fontName = fontName.substring(0, dotidx);
            if (!fontNames.contains(fontName))
                fontNames.add(fontName);
        }
        if (!fontNames.contains("default"))
            fontNames.add("default");
        return (String[]) fontNames.toArray(new String[fontNames.size()]);
    }
	
    /* Given the name and style of a font this method will detrmine the fontset name
	that needs to be loaded by gdk_font_load. */
	
    static String getNativeFontName(String name, int style) {
		
		/* Call the static version in GPlatformFont */
		
        String nativeName;
        name = name.toLowerCase();
        nativeName = fprops.getProperty(name + "." + style);
        if (nativeName == null) {
            // Check if plain version exists
			
            nativeName = fprops.getProperty(name + ".0");
			
			// Get the default native font name from a system property if
			//   exists as a system property
            if (nativeName == null) {
				if (defaultNativeFontName == null) {
					String fontNameTmp = 
					System.getProperty("j2me.pp.gtk.default.font");
					if (fontNameTmp == null) {
						defaultNativeFontName = DEFAULT_NATIVE_FONT_NAME;
					}
					else {
						defaultNativeFontName = fontNameTmp;
					}
				}
				nativeName = defaultNativeFontName;
			}
        }
        return nativeName;
    }
	
}
