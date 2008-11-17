/*
 * @(#)PlatformFont.java	1.33 06/10/10
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
package sun.awt;

import java.awt.Font;
import sun.awt.peer.FontPeer;
import java.util.Properties;
import java.util.Hashtable;
import java.util.Vector; 
import java.io.InputStream;
import java.io.BufferedInputStream;
import sun.io.CharToByteConverter;
import sun.io.CharacterEncoding;
import sun.io.FileIO;
import sun.io.FileIOFactory;

public abstract class PlatformFont implements FontPeer {
    protected FontDescriptor[] componentFonts;
    protected char defaultChar;
    protected Properties props;
    protected FontDescriptor defaultFont;
    protected static Hashtable charsetRegistry = new Hashtable(5);
    protected String aliasName;
    protected String styleString;
	
	public PlatformFont() {
		/* This is a dummy holder ... used in very special cases */
	}
	
    public PlatformFont(String name, int style) {
        if (fprops == null) {
            // there is no default font properties
            this.props = null;
            return;
        }
        this.props = fprops;
        aliasName = props.getProperty("alias" + "." +
                    name.toLowerCase());
        if (aliasName == null)
            aliasName = name.toLowerCase();
            // check this name is correct or not
        if ((props.getProperty(aliasName + ".0") == null) &&
            (props.getProperty(aliasName + ".plain.0") == null))
            aliasName = "sansserif";
        styleString = styleStr(style);
        Vector compFonts = new Vector(5);
        int numOfFonts = 0;
        for (;; numOfFonts++) {
            String index = String.valueOf(numOfFonts);
            // search native font name
            //
            String nativeName = props.getProperty
                (aliasName + "." + styleString + "." + index);
            if (nativeName == null) {
                nativeName = props.getProperty(aliasName + "." + index);
                if (nativeName == null) {
                    break;
                }
            }
            // search font charset
            //
            String fcName = props.getProperty
                ("fontcharset." + aliasName + "." + styleString + "." + index);
            if (fcName == null) {
                fcName = props.getProperty
                        ("fontcharset." + aliasName + "." + index);
                if (fcName == null) {
                    fcName = "default";
                }
            }
            CharToByteConverter
                fontCharset = getFontCharset(fcName.trim(), nativeName);
            // search exclusion range for this font
            //
            String exString = props.getProperty
                ("exclusion." + aliasName + "." + styleString + "." + index);
            if (exString == null) {
                exString = props.getProperty
                        ("exclusion." + aliasName + "." + index);
                if (exString == null) {
                    exString = "none";
                }
            }
            int[] exRange;
            if (exString.equals("none")) {
                exRange = new int[0];
            } else {
                /*
                 * range format is xxxx-XXXX,yyyy-YYYY,.....
                 */
                int numRange = 1, idx = 0;
                for (;; numRange++) {
                    idx = exString.indexOf(',', idx);
                    if (idx == -1) {
                        break;
                    }
                    idx++;
                }
                exRange = new int[numRange];
                for (int j = 0; j < numRange; j++) {
                    String lower, upper;
                    int lo = 0, up = 0;
                    try {
                        lower = exString.substring(j * 10, j * 10 + 4);
                        upper = exString.substring(j * 10 + 5, j * 10 + 9);
                    } catch (StringIndexOutOfBoundsException e) {
                        exRange = new int[0];
                        break;
                    }
                    try {
                        lo = Integer.parseInt(lower, 16);
                        up = Integer.parseInt(upper, 16);
                    } catch (NumberFormatException e) {
                        exRange = new int[0];
                        break;
                    }
                    exRange[j] = lo << 16 | up;
                }
            }
            compFonts.addElement(new FontDescriptor(nativeName,
                    fontCharset,
                    exRange));
        }
        componentFonts = new FontDescriptor[numOfFonts];
        for (int i = 0; i < numOfFonts; i++) {
            componentFonts[i] = (FontDescriptor) compFonts.elementAt(i);
        }
        // search default character
        //
        int dfChar;
        try {
            dfChar = Integer.parseInt
                    (props.getProperty("default.char", "003f"), 16);
        } catch (NumberFormatException e) {
            dfChar = 0x3f;
        }
        defaultChar = 0x3f;
        if (componentFonts.length > 0)
            defaultFont = componentFonts[0];
        for (int i = 0; i < componentFonts.length; i++) {
            if (componentFonts[i].isExcluded((char) dfChar)) {
                continue;
            }
            if (componentFonts[i].fontCharset.canConvert((char) dfChar)) {
                defaultFont = componentFonts[i];
                defaultChar = (char) dfChar;
                break;
            }
        }
    }

    /**
     * Is it possible that this font's metrics require the multi-font calls?
     * This might be true, for example, if the font supports kerning.
     **/
    public boolean mightHaveMultiFontMetrics() {
        return props != null;
    }
    /*
     * make default font properties. 
     */
    protected static Properties fprops;
    static {
        // initialize fprops

        // Find property file
        fprops = (Properties) java.security.AccessController.doPrivileged(
                    new java.security.PrivilegedAction() { 
                        public Object run() {
                            Properties fpr = null;
                            String jhome = System.getProperty("java.home");
                            String uhome = System.getProperty("user.home");
                            if (jhome == null) {
                                throw new Error("java.home property not set");
                            }
                            String language = System.getProperty("user.language", "en");
                            String region = System.getProperty("user.region");
                            // Translate the raw encoding name returned by the VM to the canonical
                            // name from the alias table in CharacterEncoding. Map unlisted raw
                            // encoding names to themselves. - bug 4163038
                            String rawEncoding = System.getProperty("file.encoding");
                            String encoding = CharacterEncoding.aliasName(rawEncoding);
                            if (encoding == null) {
                                encoding = rawEncoding;
                            }
                            try {
                                FileIO f = null;
                                if (region != null) {
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
                                // set default props to prevent crashing
                                // with corrupted font.properties
                                Properties defaultProps = new Properties();
                                defaultProps.put("serif.0", "unknown");
                                defaultProps.put("sansserif.0", "unknown");
                                defaultProps.put("monospaced.0", "unknown");
                                defaultProps.put("dialog.0", "unknown");
                                defaultProps.put("dialoginput.0", "unknown");
                                fpr = new Properties(defaultProps);
                                // Load property file
                                InputStream in = new BufferedInputStream(f.getInputStream());
                                fpr.load(in);
                                in.close();
                            } catch (Exception e) {}
                            return fpr;
                        }
                    }
                );
    }
    private static FileIO tryOpeningFontProp(
        FileIO f, String homedir,
        String language, String ext) {
        if (f != null) {
            return f;       // already validated
        }
        String filename = homedir + FileIO.separator
            + "lib" + FileIO.separator
            + "font.properties";
        if (language != null) {
            filename += "." + language;
            if (ext != null) {
                filename += "_" + ext;
            }
        }
        FileIO propsFile = FileIOFactory.newInstance(filename);
        if ((propsFile != null) && propsFile.canRead()) {
            return propsFile;
        }
        return null;
    }

    /**
     * make a array of CharsetString with given String.
     */
    public CharsetString[] makeMultiCharsetString(String str) {
        return makeMultiCharsetString(str.toCharArray(), 0, str.length());
    }

    /**
     * make a array of CharsetString with given char array.
     * @param str The char array to convert.
     * @param offset offset of first character of interest
     * @param len number of characters to convert
     */
    public CharsetString[] makeMultiCharsetString(char str[], int offset, int len) {
        if (len < 1) {
            return new CharsetString[0];
        }
        Vector mcs = null;
        char[] tmpStr = new char[len];
        char tmpChar = defaultChar;
        FontDescriptor currentFont = defaultFont;
        for (int i = 0; i < componentFonts.length; i++) {
            if (componentFonts[i].isExcluded(str[offset])) {
                continue;
            }
            if (componentFonts[i].fontCharset.canConvert(str[offset])) {
                currentFont = componentFonts[i];
                tmpChar = str[offset];
                break;
            } 
        }
        tmpStr[0] = tmpChar;
        int lastIndex = 0;
        for (int i = 1; i < len; i++) {
            char ch = str[offset + i];
            FontDescriptor fd = defaultFont;
            tmpChar = defaultChar;
            for (int j = 0; j < componentFonts.length; j++) {
                if (componentFonts[j].isExcluded(ch)) {
                    continue;
                }
                if (componentFonts[j].fontCharset.canConvert(ch)) {
                    fd = componentFonts[j];
                    tmpChar = ch;
                    break;
                }
            }
            tmpStr[i] = tmpChar;
            if (currentFont != fd) {
                if (mcs == null) {
                    mcs = new Vector(3);
                }
                mcs.addElement(new CharsetString(tmpStr, lastIndex, 
                        i - lastIndex, currentFont));
                currentFont = fd;
                fd = defaultFont;
                lastIndex = i;
            }
        }
        CharsetString[] result;
        CharsetString cs = new CharsetString(tmpStr, lastIndex,
                len - lastIndex, currentFont);
        if (mcs == null) {
            result = new CharsetString[1];
            result[0] = cs;
        } else {
            mcs.addElement(cs);
            result = new CharsetString[mcs.size()];
            for (int i = 0; i < mcs.size(); i++) {
                result[i] = (CharsetString) mcs.elementAt(i);
            }
        }
        return result;
    }

    protected abstract CharToByteConverter getFontCharset(String charsetName,
        String fontName);

    /*
     * return String representation of style
     */
    public static String styleStr(int num) {
        switch (num) {
        case Font.BOLD:
            return "bold";

        case Font.ITALIC:
            return "italic";

        case Font.ITALIC + Font.BOLD:
            return "bolditalic";

        default:
            return "plain";
        }
    }
	
	public String getNativeName(FontDescriptor fd) {
		return fd.nativeName;
	}
}
