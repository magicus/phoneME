/*
 * @(#)QtFontPeer.java	1.24 06/10/10
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
 * QtFontPeer.java
 *
 */

package sun.awt.qt;

import sun.awt.peer.FontPeer;
import java.awt.Font;
import java.awt.FontMetrics;
import sun.io.CharacterEncoding;
import java.io.InputStream;
import java.io.BufferedInputStream;
import sun.io.FileIO;
import sun.io.FileIOFactory;
import java.util.*;
import java.awt.Color;
import java.awt.font.TextAttribute;

/** The peer used for fonts in Qt. This class extends the FontMetrics
    class so that it is very quick to get the font metrics for a font
	and we don't need to create a new object each time. We also maintain a cache
    which allows multiple font objects to share the same peer if they are of the
 same name, style and size. This reduces further the number of objects created. */

class QtFontPeer extends FontMetrics implements FontPeer
{
    private static int generatedFontCount = 0;

    /* 
     * Added for 1.1 attribute support
     */
    private boolean strikethrough;
    private boolean underline;

	
    private QtFontPeer (Font font, boolean strikethrough, boolean underline)
    {
        super (font);
        this.strikethrough = strikethrough;
        this.underline = underline;
        init(font.getFamily(), font.isItalic(), font.isBold(), 
             font.getSize());
    }
	
    private QtFontPeer (int qtfont)
    {
        super (null);
        data = qtfont;
    }
	
    /** Gets the FontPeer for the specified Font. This will look in a
	cache first to prevent loading of the font for a second time. */
	
    synchronized static QtFontPeer getFontPeer (Font font)
    {
        QtFontPeer peer=null;
        String fontKey = font.getName() + font.getSize() + font.getStyle();
        Map attributes = font.getAttributes();
        Object obj;
        boolean strikeThrough=false, underline=false;
        if ((obj = attributes.get(TextAttribute.STRIKETHROUGH)) != null) {
            if(obj.equals(TextAttribute.STRIKETHROUGH_ON)) {
                fontKey += "strike";
                strikeThrough = true;
            }
        }
        
        if ((obj = attributes.get(TextAttribute.UNDERLINE)) != null) {
            if(obj.equals(TextAttribute.UNDERLINE_ON)) {
                fontKey += "under";
                underline = true;
            }
        }
        
        peer = (QtFontPeer)fontToPeerMap.get(fontKey);
        
        if (peer == null) {
            peer = new QtFontPeer (font, strikeThrough, underline);
            fontToPeerMap.put (fontKey, peer);
        }
        
        return peer;
    }

    
	
    /** Gets the font for the specified QtFont. This is used when installing fonts on
	the Java components when a widget is created. We check what font the native widget uses and,
	if one has not been explicitly set for the Java component, we set the font on the component.
	This will examine all loaded fonts in the cache and see if there is one for qtfont.
	If not it will create a new font object with a unique name. */
	
    private synchronized static Font getFont (int qtfont)
    {
	Iterator i = fontToPeerMap.entrySet().iterator();
	    
	while (i.hasNext())	{
	    Map.Entry entry = (Map.Entry)i.next();
	    QtFontPeer peer = (QtFontPeer)entry.getValue();
		
	    if (areFontsTheSame(qtfont, peer.data)) {
            // fontToPeerMap has multi-type keys which is either a String
            // (See getFontPeer(Font) or a Font (See getFont(int)), so 
            // instead of performing a dynamic cast on "entry.getKey()",
            // we simply return the Font contained in the peer
            return peer.getFont();
        }
	}
	    
	// We need to generate a new font for the QtFont.
	    
	QtFontPeer peer = new QtFontPeer (qtfont);
	Font font = createFont ("GeneratedFont" + (++generatedFontCount), peer);
    // use a string as a key here to be consistent with getFontPeer
    // but, here we don't need to worry about strikethrough and underline 
    // because we won't create a native widget with a font that has those 
    // attributes
    String fontKey = font.getName() + font.getSize() + font.getStyle();
    
	fontToPeerMap.put (fontKey, peer);
	    
	return font;
    }
	
    /** Tests whether two qtfonts are the same. */
	
    private static native boolean areFontsTheSame (int font1, int font2);
	
    /** Creates a font with the specified name and qtfont. */
    private static native Font createFont (String name, QtFontPeer peer);
	
    /**
     * 6228133
     * Deletes the native font.
     */
    private static native void deleteFonts (int len, int[] qtfonts);
	
    /** Loads the font and sets the ascent and descent fields. */
	
    private native void init(String family, boolean italic,
                             boolean bold, int size);
	
    /**
     * Determines the <em>font ascent</em> of the font described by this
     * font metric. The font ascent is the distance from the font's
     * baseline to the top of most alphanumeric characters. Some
     * characters in the font may extend above the font ascent line.
     * @return     the font ascent of the font.
     * @see        java.awt.FontMetrics#getMaxAscent
     * @since      JDK1.0
     */
    public int getAscent() {return ascent;}
	
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
    public int getDescent() {return descent;}
	
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
    public int getLeading() {return leading;}
	
    /**
     * Gets the maximum advance width of any character in this Font.
     * The advance width is the amount by which the current point is
     * moved from one character to the next in a line of text.
     * @return    the maximum advance width of any character
     *            in the font, or <code>-1</code> if the
     *            maximum advance width is not known.
     * @since     JDK1.0
     */
    public int getMaxAdvance() {return -1;}
	
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
    public int charWidth(char ch)
    {
	// Optimization for ASCII characters. Prevents creating a string and using all the
	// char to byte converters to get the bytes.
		
	if (ch < 128)
	    return asciiCharWidth (ch);
		
	String string = new String(new char[] {ch});

	return stringWidthNative(string);
    }
	
    private native int asciiCharWidth (char c);

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
    public int stringWidth(String str)
    {
        // 6261520
        // throwing NPE to be complaint with J2SE impl, eventhough the spec
        // does not enforce it.
        if ( str == null ) {
            throw new NullPointerException("str is null") ;
        }
        // 6261520
        return stringWidthNative(str);
    }
	
    protected native int stringWidthNative(String str);

    private static native void initIDs();
	
    /* The font propertiues used to map from font names to fonset names. */
	
    private static Properties defaultProperties;
	
    /*
     * Try to load font properties from default locations.
     * If font properties cannot be loaded (possibly because there is no file
     * system present), use a set of en_US properties by default.
     */
    static
    {
	initIDs();
	defaultProperties = new Properties();
		
	java.security.AccessController.doPrivileged(new java.security.PrivilegedAction()
	    {
		public Object run()
		{
		    String jhome = System.getProperty("java.home");
		    String uhome = System.getProperty("user.home");
				
		    if (jhome == null){
			throw new Error("java.home property not set");
		    }
				
		    String language = System.getProperty("user.language", "en");
		    String region = System.getProperty("user.region");
				
				// Translate the raw encoding name returned by the VM to the canonical
				// name from the alias table in CharacterEncoding. Map unlisted raw
				// encoding names to themselves. - bug 4163038
				
		    String rawEncoding = System.getProperty("file.encoding");
		    String encoding = CharacterEncoding.aliasName(rawEncoding);
				
		    if (encoding == null)
			encoding = rawEncoding;
				
		    try
			{
			    FileIO f = null;
					
			    if (region != null)
				{
				    f = tryOpeningFontProp(
							   f, uhome, language, region + "_" + encoding);
				    f = tryOpeningFontProp(
							   f, jhome, language, region + "_" + encoding);
				    f = tryOpeningFontProp(f, uhome, language, region);
				    f = tryOpeningFontProp(f, jhome, language, region);
				}
					
			    f = tryOpeningFontProp(f, uhome, language, encoding);
			    f = tryOpeningFontProp(f, jhome, language, encoding);
			    f = tryOpeningFontProp(f, uhome, language, null);
			    f = tryOpeningFontProp(f, jhome, language, null);
			    f = tryOpeningFontProp(f, uhome, encoding, null);
			    f = tryOpeningFontProp(f, jhome, encoding, null);
			    f = tryOpeningFontProp(f, uhome, null, null);
			    f = tryOpeningFontProp(f, jhome, null, null);
					
			    // Load property file
			    InputStream in = new BufferedInputStream(f.getInputStream());
			    defaultProperties.load(in);
			    in.close();
			}
				
				// If anything goes wrong then resort to default properties.
				
		    catch (Exception e)
			{
			    setDefaultProperty("serif.0", "adobe-times-normal-r---*-%d-*");
			    setDefaultProperty("serif.1", "-urw-itc zapfdingbats-medium-r-normal--*-%d-*");
			    setDefaultProperty("serif.2", "*-symbol-medium-r-normal--*-%d-*");
			    setDefaultProperty("serif.italic.0", "-adobe-times-normal-i---*-%d-*");
			    setDefaultProperty("serif.bold.0", "-adobe-times-bold-r---*-%d-*");
			    setDefaultProperty("serif.bolditalic.0", "-adobe-times-bold-i---*-%d-*");
			    setDefaultProperty("sansserif.0", "-adobe-helvetica-normal-r-normal--*-%d-*");
			    setDefaultProperty("sansserif.1", "-urw-itc zapfdingbats-medium-r-normal--*-%d-*");
			    setDefaultProperty("sansserif.2", "-*-symbol-medium-r-normal--*-%d-*");
			    setDefaultProperty("sansserif.italic.0", "-adobe-helvetica-normal-i-normal--*-%d-*");
			    setDefaultProperty("sansserif.bold.0", "-adobe-helvetica-bold-r-normal--*-%d-*");
			    setDefaultProperty("sansserif.bolditalic.0", "-adobe-helvetica-bold-i-normal--*-%d-*");
			    setDefaultProperty("monospaced.0", "-adobe-courier-normal-r---*-%d-*");
			    setDefaultProperty("monospaced.1", "-urw-itc zapfdingbats-medium-r-normal--*-%d-*");
			    setDefaultProperty("monospaced.2", "-*-symbol-medium-r-normal--*-%d-*");
			    setDefaultProperty("monospaced.italic.0", "-adobe-courier-normal-i---*-%d-*");
			    setDefaultProperty("monospaced.bold.0", "-adobe-courier-bold-r---*-%d-*");
			    setDefaultProperty("monospaced.bolditalic.0", "-adobe-courier-bold-i---*-%d-*");
			    setDefaultProperty("dialog.0", "-adobe-helvetica-normal-r-normal--*-%d-*");
			    setDefaultProperty("dialog.1", "-urw-itc zapfdingbats-medium-r-normal--*-%d-*");
			    setDefaultProperty("dialog.2", "-*-symbol-medium-r-normal--*-%d-*");
			    setDefaultProperty("dialog.italic.0", "-adobe-helvetica-normal-i-normal--*-%d-*");
			    setDefaultProperty("dialog.bold.0", "-adobe-helvetica-bold-r-normal--*-%d-*");
			    setDefaultProperty("dialog.bolditalic.0", "-adobe-helvetica-bold-i-normal--*-%d-*");
			    setDefaultProperty("dialoginput.0", "-adobe-courier-normal-r---*-%d-*");
			    setDefaultProperty("dialoginput.1", "-urw-itc zapfdingbats-medium-r-normal--*-%d-*");
			    setDefaultProperty("dialoginput.2", "-*-symbol-medium-r-normal--*-%d-*");
			    setDefaultProperty("dialoginput.italic.0", "-adobe-courier-normal-i---*-%d-*");
			    setDefaultProperty("dialoginput.bold.0", "-adobe-courier-bold-r---*-%d-*");
			    setDefaultProperty("dialoginput.bolditalic.0", "-adobe-courier-bold-i---*-%d-*");
			    setDefaultProperty("default.char", "274f");
			    setDefaultProperty("timesroman.0", "-adobe-times-normal-r---*-%d-*");
			    setDefaultProperty("timesroman.italic.0", "-adobe-times-normal-i---*-%d-*");
			    setDefaultProperty("timesroman.bold.0", "-adobe-times-bold-r---*-%d-*");
			    setDefaultProperty("timesroman.bolditalic.0", "-adobe-times-bold-i---*-%d-*");
			    setDefaultProperty("helvetica.0", "-adobe-helvetica-normal-r-normal--*-%d-*");
			    setDefaultProperty("helvetica.italic.0", "-adobe-helvetica-normal-i-normal--*-%d-*");
			    setDefaultProperty("helvetica.bold.0", "-adobe-helvetica-bold-r-normal--*-%d-*");
			    setDefaultProperty("helvetica.bolditalic.0", "-adobe-helvetica-bold-i-normal--*-%d-*");
			    setDefaultProperty("courier.0", "-adobe-courier-normal-r---*-%d-*");
			    setDefaultProperty("courier.italic.0", "-adobe-courier-normal-i---*-%d-*");
			    setDefaultProperty("courier.bold.0", "-adobe-courier-bold-r---*-%d-*");
			    setDefaultProperty("courier.bolditalic.0", "-adobe-courier-bold-i---*-%d-*");
			    setDefaultProperty("zapfdingbats.0", "-urw-itc zapfdingbats-medium-r-normal--*-%d-*");
					
					
			    setDefaultProperty("fontset.serif.plain", "-adobe-times-normal-r-*--*-%d-*");
			    setDefaultProperty("fontset.serif.italic", "-adobe-times-normal-i-*--*-%d-*");
			    setDefaultProperty("fontset.serif.bold", "-adobe-times-bold-r-*--*-%d-*");
			    setDefaultProperty("fontset.serif.bolditalic", "-adobe-times-bold-i-*--*-%d-*");
			    setDefaultProperty("fontset.sansserif.plain", "-adobe-helvetica-medium-r-normal--*-%d-*");
			    setDefaultProperty("fontset.sansserif.italic", "-adobe-helvetica-medium-i-normal--*-%d-*");
			    setDefaultProperty("fontset.sansserif.bold", "-adobe-helvetica-bold-r-normal--*-%d-*");
			    setDefaultProperty("fontset.sansserif.bolditalic", "-adobe-helvetica-bold-i-normal--*-%d-*");
			    setDefaultProperty("fontset.monospaced.plain", "-adobe-courier-medium-r-*--*-%d-*");
			    setDefaultProperty("fontset.monospaced.italic", "-adobe-courier-medium-i-*--*-%d-*");
			    setDefaultProperty("fontset.monospaced.bold", "-adobe-courier-bold-r-*--*-%d-*");
			    setDefaultProperty("fontset.monospaced.bolditalic", "-adobe-courier-bold-i-*--*-%d-*");
			    setDefaultProperty("fontset.dialog.plain", "-adobe-helvetica-medium-r-normal--*-%d-*");
			    setDefaultProperty("fontset.dialog.italic", "-adobe-helvetica-medium-i-normal--*-%d-*");
			    setDefaultProperty("fontset.dialog.bold", "-adobe-helvetica-bold-r-normal--*-%d-*");
			    setDefaultProperty("fontset.dialog.bolditalic", "-adobe-helvetica-bold-i-normal--*-%d-*");
			    setDefaultProperty("fontset.dialoginput.plain", "-adobe-courier-medium-r-*--*-%d-*");
			    setDefaultProperty("fontset.dialoginput.italic", "-adobe-courier-medium-i-*--*-%d-*");
			    setDefaultProperty("fontset.dialoginput.bold", "-adobe-courier-bold-r-*--*-%d-*");
			    setDefaultProperty("fontset.dialoginput.bolditalic", "-adobe-courier-bold-i-*--*-%d-*");
			    setDefaultProperty("fontset.zapfdingbats", "-urw-itc zapfdingbats-medium-r-normal--*-%d-*");
			    setDefaultProperty("fontset.timesroman.plain", "-adobe-times-medium-r-*--*-%d-*");
			    setDefaultProperty("fontset.timesroman.italic", "-adobe-times-medium-i-*--*-%d-*");
			    setDefaultProperty("fontset.timesroman.bold", "-adobe-times-bold-r-*--*-%d-*");
			    setDefaultProperty("fontset.timesroman.bolditalic", "-adobe-times-bold-i-*--*-%d-*");
			    setDefaultProperty("fontset.helvetica.plain", "-adobe-helvetica-medium-r-normal--*-%d-*");
			    setDefaultProperty("fontset.helvetica.italic", "-adobe-helvetica-medium-i-normal--*-%d-*");
			    setDefaultProperty("fontset.helvetica.bold", "-adobe-helvetica-bold-r-medium--*-%d-*");
			    setDefaultProperty("fontset.helvetica.bolditalic", "-adobe-helvetica-bold-i-normal--*-%d-*");
			    setDefaultProperty("fontset.courier.plain", "-adobe-courier-medium-r-*--*-%d-*");
			    setDefaultProperty("fontset.courier.italic", "-adobe-courier-medium-i-*--*-%d-*");
			    setDefaultProperty("fontset.courier.bold", "-adobe-courier-bold-r-*--*-%d-*");
			    setDefaultProperty("fontset.courier.bolditalic", "-adobe-courier-bold-i-*--*-%d-*");
			    setDefaultProperty("fontset.zapfdingbats", "-urw-itc zapfdingbats-medium-r-normal--*-%d-*");
			    setDefaultProperty("fontset.default", "-adobe-helvetica-medium-r-normal--*-%d-*");
			}
				
		    return null;
		}
	    });
    }
	
    /*
     * 6228133
     *
     * Called when the VM is shutting down by QtToolkit's shutdown hook.
     */
    static void cleanup() {
        if (fontToPeerMap.size() == 0) {
            return;
        }

        int i = 0, len = 0;;

        // Free the native qt font memory.  This pointers are being held
        // in the static cache (Map).
        len = fontToPeerMap.size();
        int[] fontArray = new int[len];
        Iterator itr = fontToPeerMap.entrySet().iterator();
        
        while (itr.hasNext())	{
            Map.Entry entry = (Map.Entry)itr.next();
            QtFontPeer peer = (QtFontPeer)entry.getValue();
            fontArray[i] = peer.data;
            i++;
        }
        deleteFonts(len, fontArray);        
    }

    private static void setDefaultProperty (String name, String value)
    {
	if (defaultProperties.getProperty(name) == null)
	    defaultProperties.setProperty(name, value);
    }
	
    private static FileIO tryOpeningFontProp(FileIO f, String homedir, String language, String ext)
    {
	if (f != null)
	    return f;       // already validated
		
	String filename = homedir + FileIO.separator
	    + "lib" + FileIO.separator
	    + "font.properties";
		
	if (language != null)
	    {
		filename += "." + language;
			
		if (ext != null)
		    filename += "_" + ext;
	    }
		
	FileIO propsFile = FileIOFactory.newInstance(filename);
	if ((propsFile != null) && propsFile.canRead()) {
	    return propsFile;
	}
		
	return null;
    }
	
    /* The map which maps fonts to their peers. */
	
    private static Map fontToPeerMap = new HashMap (10);
	
    /* Private data used by native code to store the QtFont. */
	
    int data;
	
    /* The ascent of this font. */
	
    private int ascent;
	
    /* The descent of this font. */
	
    private int descent;

    /* The leading, i.e., the natural inter-line spacing, of this font. */
	
    private int leading;
}

