/*
 * @(#)PPCFontPeer.java	1.11 06/10/10
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

/**
 * PPCFontPeer.java
 *
 */

package sun.awt.pocketpc;

import java.awt.Font;
import sun.io.CharToByteConverter;
import sun.awt.PlatformFont;

public class PPCFontPeer extends PlatformFont {

    /*
     * Name for Pocket PC FontSet.
     */
    private String wfsname;

    private PPCFontPeer(Font font){
	super(font.getName(), font.getStyle());

	if (props != null){
	    wfsname = props.getProperty
		("fontset." + aliasName + "." + styleString);
	}
	init(font);
    }

    static PPCFontPeer getFontPeer(Font font) 
    {
	PPCFontPeer peer = getPeer(font);
	if (peer == null) {
	    peer = new PPCFontPeer(font);
	}
	
	return peer;
    }

    private native void init(Font font);
    private static native PPCFontPeer getPeer(Font font);

    public CharToByteConverter
	getFontCharset(String charsetName, String fontName){

	CharToByteConverter fc;
	if (charsetName.equals("default")){
	    fc = (CharToByteConverter)charsetRegistry.get(fontName);
	} else {
	    fc = (CharToByteConverter)charsetRegistry.get(charsetName);
	}
	if (fc instanceof CharToByteConverter){
	    return fc;
	}

	Class fcc = null;

	try {
	    fcc = Class.forName(charsetName);
	} catch(ClassNotFoundException e){
	    try {
		fcc = Class.forName("sun.io." + charsetName);
	    } catch (ClassNotFoundException exx){
		try {
		    fcc = Class.forName("sun.awt.pocketpc." + charsetName);
		} catch(ClassNotFoundException ex){
		    fc = getDefaultFontCharset(fontName);
		}
	    }
	}

	if (fc == null) {
	    try {
		fc = (CharToByteConverter)fcc.newInstance();
	    } catch(InstantiationException e) {
		return getDefaultFontCharset(fontName);
	    } catch(IllegalAccessException e) {
		return getDefaultFontCharset(fontName);
	    }
	}

	if (charsetName.equals("default")){
	    charsetRegistry.put(fontName, fc);
	} else {
	    charsetRegistry.put(charsetName, fc);
	}
	return fc;
    }


    private CharToByteConverter getDefaultFontCharset(String fontName){
	return new PPCDefaultFontCharset(fontName);
    }

    /*
     * If font properties cannot be loaded (possibly because there is no file
     * system present), use a set of en_US properties by default.
     */
    static {
	if ((fprops.getProperty("serif.0").equals("unknown")) ||
	    (fprops.getProperty("sansserif.0").equals("unknown")) ||
	    (fprops.getProperty("monospaced.0").equals("unknown")) ||
	    (fprops.getProperty("dialog.0").equals("unknown")) ||
	    (fprops.getProperty("dialoginput.0").equals("unknown"))) {
	    fprops.put("dialog.0", "Arial,ANSI_CHARSET");
	    fprops.put("dialog.1", "WingDings,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("dialog.2", "Symbol,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("dialoginput.0", "Courier New,ANSI_CHARSET");
	    fprops.put("dialoginput.1", "WingDings,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("dialoginput.2", "Symbol,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("serif.0", "Times New Roman,ANSI_CHARSET");
	    fprops.put("serif.1", "WingDings,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("serif.2", "Symbol,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("sansserif.0", "Arial,ANSI_CHARSET");
	    fprops.put("sansserif.1", "WingDings,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("sansserif.2", "Symbol,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("monospaced.0", "Courier New,ANSI_CHARSET");
	    fprops.put("monospaced.1", "WingDings,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("monospaced.2", "Symbol,SYMBOL_CHARSET,NEED_CONVERTED");
	    fprops.put("timesroman.0", "Times New Roman,ANSI_CHARSET");
	    fprops.put("helvetica.0", "Arial,ANSI_CHARSET");
	    fprops.put("courier.0", "Courier New,ANSI_CHARSET");
	    fprops.put("zapfdingbats.0", "WingDings,SYMBOL_CHARSET");
	    fprops.put("default.char", "2751");
	    fprops.put("fontcharset.dialog.1", "sun.awt.pocketpc.CharToByteWingDings");
	    fprops.put("fontcharset.dialog.2", "sun.awt.CharToByteSymbol");
	    fprops.put("fontcharset.dialoginput.1", "sun.awt.pocketpc.CharToByteWingDings");
	    fprops.put("fontcharset.dialoginput.2", "sun.awt.CharToByteSymbol");
	    fprops.put("fontcharset.serif.1", "sun.awt.pocketpc.CharToByteWingDings");
	    fprops.put("fontcharset.serif.2", "sun.awt.CharToByteSymbol");
	    fprops.put("fontcharset.sansserif.1", "sun.awt.pocketpc.CharToByteWingDings");
	    fprops.put("fontcharset.sansserif.2", "sun.awt.CharToByteSymbol");
	    fprops.put("fontcharset.monospaced.1", "sun.awt.pocketpc.CharToByteWingDings");
	    fprops.put("fontcharset.monospaced.2", "sun.awt.CharToByteSymbol");
	    fprops.put("exclusion.dialog.0", "0100-20ab,20ad-ffff");
	    fprops.put("exclusion.dialoginput.0", "0100-20ab,20ad-ffff");
	    fprops.put("exclusion.serif.0", "0100-20ab,20ad-ffff");
	    fprops.put("exclusion.sansserif.0", "0100-20ab,20ad-ffff");
	    fprops.put("exclusion.monospaced.0", "0100-20ab,20ad-ffff");
	    fprops.put("inputtextcharset", "ANSI_CHARSET");
	}
    }
}
