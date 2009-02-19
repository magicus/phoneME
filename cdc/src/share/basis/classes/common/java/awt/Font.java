/*
 * @(#)Font.java	1.28 06/10/10
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

import java.security.AccessController;
import sun.security.action.GetPropertyAction;
import java.util.StringTokenizer;
import java.util.Locale;
import java.util.Map;
import java.util.Hashtable;
import java.awt.font.TextAttribute;
import java.text.AttributedCharacterIterator.Attribute;


/**
 * A class that produces font objects.
 *
 * @version 	1.2, 10/18/01
 * @author 	Nicholas Allen
 * @since       JDK1.0
 */
public class Font implements java.io.Serializable {
    /*
     * Constants to be used for styles. Can be combined to mix
     * styles.
     */

    /**
     * The plain style constant.  This style can be combined with
     * the other style constants for mixed styles.
     * @since JDK1.0
     */
    public static final int PLAIN = 0;
    /**
     * The bold style constant.  This style can be combined with the
     * other style constants for mixed styles.
     * @since JDK1.0
     */
    public static final int BOLD = 1;
    /**
     * The italicized style constant.  This style can be combined
     * with the other style constants for mixed styles.
     * @since JDK1.0
     */
    public static final int ITALIC = 2;
    /**
     * The platform specific family name of this font.
     */
    transient private String family;
    /**
     * The logical name of this font.
     * @since JDK1.0
     */
    protected String name;
    /**
     * The style of the font. This is the sum of the
     * constants <code>PLAIN</code>, <code>BOLD</code>,
     * or <code>ITALIC</code>.
     */
    protected int style;
    /**
     * The point size of this font.
     * @since JDK1.0
     */
    protected int size;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = -4206021311591459213L;
    /** Cached font metrics for this font. This should be initialised the first time
     the font metrics is obtained for this font. */
	
    transient FontMetrics metrics;
	
    /**
	 * A map of font attributes available in this font.
     * Attributes include things like ligatures and glyph substitution.
     *
     * @serial
     * @see #getAttributes()
     */
    private Hashtable fRequestedAttributes;
    private static final Map EMPTY_MAP = new Hashtable(5, (float)0.9);
	
    private void initializeFont(Hashtable attributes) {
	
        if (attributes == null) {
            fRequestedAttributes = new Hashtable(5, (float)0.9);
            fRequestedAttributes.put(TextAttribute.FAMILY, name);
            fRequestedAttributes.put(TextAttribute.SIZE, new Float(size));
            fRequestedAttributes.put(TextAttribute.WEIGHT, (style & BOLD) != 0 ? 
									 TextAttribute.WEIGHT_BOLD : TextAttribute.WEIGHT_REGULAR);
            fRequestedAttributes.put(TextAttribute.POSTURE, (style & ITALIC) != 0 ? 
									 TextAttribute.POSTURE_OBLIQUE : TextAttribute.POSTURE_REGULAR);
			
        }
	}
	
	
    /**
     * Creates a new font with the specified name, style and point size.
     * @param name the font name
     * @param style the constant style used
     * @param size the point size of the font
     * @see Toolkit#getFontList
     * @since JDK1.0
     */
    public Font(String name, int style, int size) {
        this.name = name;
        if (this.name == null) {
            this.name = "Default";
        } 
        this.style = PLAIN;
        if (style == BOLD || style == ITALIC || style == (BOLD | ITALIC)) {
            this.style = style;
        }
        this.size = size;
        setFamily();
		
		initializeFont(fRequestedAttributes);
    }
	
    /**
		* Creates a new <code>Font</code> with the specified attributes.
     * This <code>Font</code> only recognizes keys defined in 
     * {@link TextAttribute} as attributes.  If <code>attributes</code>
     * is <code>null</code>, a new <code>Font</code> is initialized
     * with default attributes.
     * @param attributes the attributes to assign to the new
     *          <code>Font</code>, or <code>null</code>
     */
 	
	public Font(Map attributes)
	{
        this.name = "Default";
        this.style = PLAIN;
        this.size = 12;

        if((attributes != null) && (!attributes.equals(EMPTY_MAP)))
        {
            Object obj;
            fRequestedAttributes = new Hashtable(attributes);

            if ((obj = attributes.get(TextAttribute.FAMILY)) != null) {
                this.name = (String)obj;
            }
			
            if ((obj = attributes.get(TextAttribute.WEIGHT)) != null) {
                if(obj.equals(TextAttribute.WEIGHT_BOLD)) {
                    this.style |= BOLD;
                }
            }
			
            if ((obj = attributes.get(TextAttribute.POSTURE)) != null) {
                if(obj.equals(TextAttribute.POSTURE_OBLIQUE)) {
                    this.style |= ITALIC;
                }
            }
			
            if ((obj = attributes.get(TextAttribute.SIZE)) != null) {
                float pointSize = ((Float)obj).floatValue();
                this.size = (int)(pointSize + 0.5);
            }
        }

		setFamily();
		initializeFont(fRequestedAttributes);  		
	}

    /**
     * Returns a <code>Font</code> appropriate to this attribute set.
     *
     * @param attributes the attributes to assign to the new 
     *          <code>Font</code>
     * @return a new <code>Font</code> created with the specified
     *          attributes
     * @since 1.2
     * @see java.awt.font.TextAttribute
     */
    public static Font getFont(Map attributes) {
        Font font = (Font)attributes.get(TextAttribute.FONT);
        if (font != null) {
            return font;
        }
		
        return new Font(attributes);
    }	


    /**
     * Returns a map of font attributes available in this
     * <code>Font</code>.  Attributes include things like ligatures and
     * glyph substitution.
     * @return the attributes map of this <code>Font</code>.
     */
    public Map getAttributes() {
        return (Map)fRequestedAttributes.clone();
    }
	
    /**
     * Returns the keys of all the attributes supported by this
     * <code>Font</code>.  These attributes can be used to derive other
     * fonts.
     * @return an array containing the keys of all the attributes
     *          supported by this <code>Font</code>.
     * @since 1.2
     */
    public Attribute[] getAvailableAttributes(){
        Attribute attributes[] = {
            TextAttribute.FAMILY,
            TextAttribute.WEIGHT,
            TextAttribute.POSTURE,
            TextAttribute.SIZE,
        };

        return attributes;
    }
	
    /**
     * Gets the platform specific family name of the font. Use the
     * <code>getName</code> method to get the logical name of the font.
     * @return    a string, the platform specific family name.
     * @see       java.awt.Font#getName
     * @since     JDK1.0
     */
    public String getFamily() {
        return family;
    }

    /**
     * Gets the logical name of the font.
     * @return    a string, the logical name of the font.
     * @see #getFamily
     * @since     JDK1.0
     */
    public String getName() {
        return name;
    }

    /**
     * Gets the style of the font.
     * @return the style of this font.
     * @see #isPlain
     * @see #isBold
     * @see #isItalic
     * @since JDK1.0
     */
    public int getStyle() {
        return style;
    }

    /**
     * Gets the point size of the font.
     * @return the point size of this font.
     * @since JDK1.0
     */
    public int getSize() {
        return size;
    }

    /**
     * Indicates whether the font's style is plain.
     * @return     <code>true</code> if the font is neither
     *                bold nor italic; <code>false</code> otherwise.
     * @see        java.awt.Font#getStyle
     * @since      JDK1.0
     */
    public boolean isPlain() {
        return style == 0;
    }

    /**
     * Indicates whether the font's style is bold.
     * @return    <code>true</code> if the font is bold;
     *            <code>false</code> otherwise.
     * @see       java.awt.Font#getStyle
     * @since     JDK1.0
     */
    public boolean isBold() {
        return (style & BOLD) != 0;
    }

    /**
     * Indicates whether the font's style is italic.
     * @return    <code>true</code> if the font is italic;
     *            <code>false</code> otherwise.
     * @see       java.awt.Font#getStyle
     * @since     JDK1.0
     */
    public boolean isItalic() {
        return (style & ITALIC) != 0;
    }

    /**
     * Gets a font from the system properties list.
     * @param nm the property name
     * @see       java.awt.Font#getFont(java.lang.String, java.awt.Font)
     * @since     JDK1.0
     */
    public static Font getFont(String nm) {
        return getFont(nm, null);
    }

    /**
     * Returns the <code>Font</code> that the <code>str</code> 
     * argument describes.
     * To ensure that this method returns the desired Font, 
     * format the <code>str</code> parameter in
     * one of two ways:
     * <p>
     * "fontfamilyname-style-pointsize" or <br>
     * "fontfamilyname style pointsize"<p>
     * in which <i>style</i> is one of the three 
     * case-insensitive strings:
     * <code>"BOLD"</code>, <code>"BOLDITALIC"</code>, or
     * <code>"ITALIC"</code>, and pointsize is a decimal
     * representation of the point size.
     * For example, if you want a font that is Arial, bold, and
     * a point size of 18, you would call this method with:
     * "Arial-BOLD-18".
     * <p>
     * The default size is 12 and the default style is PLAIN.
     * If you don't specify a valid size, the returned 
     * <code>Font</code> has a size of 12.  If you don't specify 
     * a valid style, the returned Font has a style of PLAIN.
     * If you do not provide a valid font family name in 
     * the <code>str</code> argument, this method still returns 
     * a valid font with a family name of "dialog".
     * To determine what font family names are available on
     * your system, use the
     * {@link GraphicsEnvironment#getAvailableFontFamilyNames()} method.
     * If <code>str</code> is <code>null</code>, a new <code>Font</code>
     * is returned with the family name "dialog", a size of 12 and a 
     * PLAIN style.
       If <code>str</code> is <code>null</code>, 
     * a new <code>Font</code> is returned with the name "dialog", a  
     * size of 12 and a PLAIN style.
     * @param str the name of the font, or <code>null</code>
     * @return the <code>Font</code> object that <code>str</code>
     *		describes, or a new default <code>Font</code> if 
     *          <code>str</code> is <code>null</code>.
     * @see #getFamily
     * @since JDK1.1
     */
    public static Font decode(String str) {
	String fontName = str;
	String styleName = "";
	int fontSize = 12;
	int fontStyle = Font.PLAIN;

        if (str == null) {
            return new Font("Dialog", fontStyle, fontSize) {
                public String getFamily() {
                    return "dialog";
                }
            };
        }
	
	int lastHyphen = str.lastIndexOf('-');
	int lastSpace = str.lastIndexOf(' ');
	char sepChar = (lastHyphen > lastSpace) ? '-' : ' ';
	int sizeIndex = str.lastIndexOf(sepChar);
	int styleIndex = str.lastIndexOf(sepChar, sizeIndex-1);
	int strlen = str.length();

	if (sizeIndex > 0 && sizeIndex+1 < strlen) {
	    try {
		fontSize =
		    Integer.valueOf(str.substring(sizeIndex+1)).intValue();
		if (fontSize <= 0) {
		    fontSize = 12;
		}
	    } catch (NumberFormatException e) {
		/* It wasn't a valid size, if we didn't also find the
		 * start of the style string perhaps this is the style */
		styleIndex = sizeIndex;
		sizeIndex = strlen;
		if (str.charAt(sizeIndex-1) == sepChar) {
		    sizeIndex--;
		}
	    }
	}

	if (styleIndex >= 0 && styleIndex+1 < strlen) {
	    styleName = str.substring(styleIndex+1, sizeIndex);
	    styleName = styleName.toLowerCase(Locale.ENGLISH);
	    if (styleName.equals("bolditalic")) {
		fontStyle = Font.BOLD | Font.ITALIC;
	    } else if (styleName.equals("italic")) {
		fontStyle = Font.ITALIC;
	    } else if (styleName.equals("bold")) {
		fontStyle = Font.BOLD;
	    } else if (styleName.equals("plain")) {
		fontStyle = Font.PLAIN;
	    } else {
		/* this string isn't any of the expected styles, so
		 * assume its part of the font name
		 */
		styleIndex = sizeIndex;
		if (str.charAt(styleIndex-1) == sepChar) {
		    styleIndex--;
		}
	    }
	    fontName = str.substring(0, styleIndex);

	} else {
	    int fontEnd = strlen;
	    if (styleIndex > 0) {
		fontEnd = styleIndex;
	    } else if (sizeIndex > 0) {
		fontEnd = sizeIndex;
	    }
	    if (fontEnd > 0 && str.charAt(fontEnd-1) == sepChar) {
		fontEnd--;
	    }
	    fontName = str.substring(0, fontEnd);
	}

	return new Font(fontName, fontStyle, fontSize);
    }

    /**
     * Gets the specified font from the system properties list.
     * The first argument is treated as the name of a system property to
     * be obtained as if by the method <code>System.getProperty</code>.
     * The string value of this property is then interpreted as a font.
     * <p>
     * The property value should be one of the following forms:
     * <ul>
     * <li><em>fontname-style-pointsize</em>
     * <li><em>fontname-pointsize</em>
     * <li><em>fontname-style</em>
     * <li><em>fontname</em>
     * </ul>
     * where <i>style</i> is one of the three strings
     * <code>"BOLD"</code>, <code>"BOLDITALIC"</code>, or
     * <code>"ITALIC"</code>, and point size is a decimal
     * representation of the point size.
     * <p>
     * The default style is <code>PLAIN</code>. The default point size
     * is 12.
     * <p>
     * If the specified property is not found, the <code>font</code>
     * argument is returned instead.
     * @param nm the property name
     * @param font a default font to return if property <code>nm</code>
     *             is not defined
     * @return    the <code>Font</code> value of the property.
     * @since     JDK1.0
     */
    public static Font getFont(String nm, Font font) {
        String str = null;
        try {
            str = System.getProperty(nm);
        } catch(SecurityException e) {
        }
        if (str == null) {
            return font;
        }
        return decode(str);
    }

    /**
     * Returns a hashcode for this font.
     * @return     a hashcode value for this font.
     * @since      JDK1.0
     */
    public int hashCode() {
        return name.hashCode() ^ style ^ size;
    }
    
    /**
     * Compares this object to the specifed object.
     * The result is <code>true</code> if and only if the argument is not
     * <code>null</code> and is a <code>Font</code> object with the same
     * name, style, and point size as this font.
     * @param     obj   the object to compare this font with.
     * @return    <code>true</code> if the objects are equal;
     *            <code>false</code> otherwise.
     * @since     JDK1.0
     */
    public boolean equals(Object obj) {
        if (obj instanceof Font) {
            Font font = (Font) obj;
            return (size == font.size) && (style == font.style) && name.equals(font.name);
        }
        return false;
    }

    /**
     * Converts this object to a String representation.
     * @return     a string representation of this object
     * @since      JDK1.0
     */
    public String toString() {
        String	strStyle;
        if (isBold()) {
            strStyle = isItalic() ? "bolditalic" : "bold";
        } else {
            strStyle = isItalic() ? "italic" : "plain";
        }
        return getClass().getName() + "[family=" + family + ",name=" + name + ",style=" +
            strStyle + ",size=" + size + "]";
    }
    /* Serialization support.  A readObject method is neccessary because
     * the constructor creates the fonts peer, and we can't serialize the
     * peer.  Similarly the computed font "family" may be different
     * at readObject time than at writeObject time.  An integer version is
     * written so that future versions of this class will be able to recognize
     * serialized output from this one.
     */

    private int fontSerializedDataVersion = 1;
    private void writeObject(java.io.ObjectOutputStream s)
        throws java.lang.ClassNotFoundException,
            java.io.IOException {
        s.defaultWriteObject();
    }

    private void readObject(java.io.ObjectInputStream s)
        throws java.lang.ClassNotFoundException,
            java.io.IOException {
        s.defaultReadObject();
        setFamily();
    }

    private void setFamily() {
        // Fix for bug 4559151 - family must be an existing family name
        // Also fix for bug 4655736 - must be called from readObject method
        GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
        String[] names = ge.getAvailableFontFamilyNames();
        if (names.length == 0) {
            this.family = "Default";
        } else {
            String defaultFamilyName = null;
            for (int i = 0; i < names.length; i++) {
                if (names[i].equalsIgnoreCase(this.name)) {
                    this.family = names[i];
                    break;
                }
                /*
                 * Font.decode() specifies the following:
                 *
                 * The default size is 12 and the default style is PLAIN.
                 * If you don't specify a valid size, the returned 
                 * <code>Font</code> has a size of 12.  If you don't specify 
                 * a valid style, the returned Font has a style of PLAIN.
                 * If you do not provide a valid font family name in 
                 * the <code>str</code> argument, this method still returns 
                 * a valid font with a family name of "dialog".
                 */
                if (names[i].equalsIgnoreCase("Dialog")) {
                    defaultFamilyName = names[i];
                }
            }
            /*
             * Note that the five logical fonts: Serif, SansSerif, Monospaced,
             * Dialog, and DialogInput must be supported by the Java runtime
             * environment.  But just in case they are missing, set the family
             * to the first name.
             */
            if (this.family == null) {
                this.family = (defaultFamilyName != null) ? defaultFamilyName : names[0];
            }
        }
    }
}
