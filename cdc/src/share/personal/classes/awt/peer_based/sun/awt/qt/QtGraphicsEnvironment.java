/*
 * @(#)QtGraphicsEnvironment.java	1.9 06/10/10
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

package sun.awt.qt;


import java.awt.Graphics2D;
import java.awt.GraphicsEnvironment;
import java.awt.GraphicsDevice;
import java.awt.image.BufferedImage;
import java.util.Locale;
import sun.io.FileIO;
import sun.io.FileIOFactory;
import sun.io.CharacterEncoding;
import java.io.InputStream;
import java.io.BufferedInputStream;
import java.util.Properties;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;


public class QtGraphicsEnvironment extends GraphicsEnvironment 
{
    private static final String DEFAULT_NATIVE_FONT_NAME =
	"-adobe-courier-medium-r-normal--0-%d-0-0-m-0-iso8859-1";

// GTech-port (Bug #5078958)
    private static final String[] hardwiredFontList = {
        "Dialog", "SansSerif", "Serif", "Monospaced", "DialogInput"
    };

    private static String[] availableFontFamilyNames;
// GTech-port (Bug #5078958)

    public GraphicsDevice getDefaultScreenDevice() {
        return graphicsDevice;
    }
	
    public GraphicsDevice[] getScreenDevices() {
        return new GraphicsDevice[] {graphicsDevice};
    }

// GTech-port (Bug #5078958)
    public String[] getAvailableFontFamilyNames() {
        // Should we not create a copy here ??? gtech port
        // does not seem to do and we follow the same lines.
        return availableFontFamilyNames;
    }

	private static String[] getAvailableFontFamilyNamesInit() {
        List fontNames = new ArrayList();
        Iterator fonts = defaultProperties.keySet().iterator();
        int dotidx;
        while (fonts.hasNext()) {
            String fontName = (String) fonts.next();
            if ((dotidx = fontName.indexOf('.')) == -1)
                dotidx = fontName.length();
            fontName = fontName.substring(0, dotidx);
            for(int i=0; i<hardwiredFontList.length; i++) {
                if((hardwiredFontList[i].toLowerCase()).equals(
                                         fontName.toLowerCase())){
                    if (!fontNames.contains(fontName))
                        fontNames.add(fontName);
                }
            }
        }
        if (!fontNames.contains("default"))
            fontNames.add("default");
        return (String[]) fontNames.toArray(new String[fontNames.size()]);
    }
// GTech-port (Bug #5078958)
	
    public String[] getAvailableFontFamilyNames(Locale l) {
        return getAvailableFontFamilyNames();
    }
	
    /**
     * Returns a <code>Graphics2D</code> object for rendering into the
     * specified {@link BufferedImage}.
     * @param img the specified <code>BufferedImage</code>
     * @return a <code>Graphics2D</code> to be used for rendering into
     * the specified <code>BufferedImage</code>.
     */
    public Graphics2D createGraphics( BufferedImage img )
    {
        return img.createGraphics();
    }
	
    /* Given the name and style of a font this method will determine
       the fontset name that needs to be loaded by gdk_fontset_load. */
	
    static String getNativeFontName(String name, int style) {
        String nativeName;
		
        name = name.toLowerCase();
        nativeName = defaultProperties.getProperty(name + "." + style);
		
        if (nativeName == null) {
            // Check if plain version exists
			
            nativeName = defaultProperties.getProperty(name + ".0");
			
            if (nativeName == null)
                nativeName = DEFAULT_NATIVE_FONT_NAME;
        }
		
        return nativeName;
    }
	
    private QtGraphicsDevice graphicsDevice = new QtGraphicsDevice();
	
    /* The font propertiues used to map from font names to fontset names. */
	
    private static Properties defaultProperties;
	
    static {
        defaultProperties = new Properties();

        // Ensure the lib is loaded.
        java.awt.Toolkit.getDefaultToolkit();
		
        java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {
                public Object run() {
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
						
                    if (encoding == null)
                        encoding = rawEncoding;
						
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
							
                        // Load property file
                        InputStream in = new BufferedInputStream(f.getInputStream());

                        defaultProperties.load(in);
                        in.close();
                    } // If anything goes wrong then resort to default properties.
                    catch (Exception e) {
                        defaultProperties.put("serif.0", "-URW-Times-medium-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("serif.1", "-URW-Times-bold-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("serif.2", "-URW-Times-medium-i-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("serif.3", "-URW-Times-bold-i-normal--0-%d-0-0-p-0-iso8859-1");
        
                        defaultProperties.put("sansserif.0", "-URW-Helvetica-medium-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("sansserif.1", "-URW-Helvetica-bold-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("sansserif.2", "-URW-Helvetica-medium-o-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("sansserif.3", "-URW-Helvetica-bold-o-normal--0-%d-0-0-p-0-iso8859-1");
        
                        defaultProperties.put("monospaced.0", "-URW-Courier-medium-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("monospaced.1", "-URW-Courier-bold-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("monospaced.2", "-URW-Courier-medium-o-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("monospaced.3", "-URW-Courier-bold-o-normal--0-%d-0-0-p-0-iso8859-1");

                        defaultProperties.put("dialog.0", "-URW-Helvetica-medium-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("dialog.1", "-URW-Helvetica-bold-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("dialog.2", "-URW-Helvetica-medium-o-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("dialog.3", "-URW-Helvetica-bold-o-normal--0-%d-0-0-p-0-iso8859-1");
        
                        defaultProperties.put("dialoginput.0", "-URW-Courier-medium-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("dialoginput.1", "-URW-Courier-bold-r-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("dialoginput.2", "-URW-Courier-medium-o-normal--0-%d-0-0-p-0-iso8859-1");
                        defaultProperties.put("dialoginput.3", "-URW-Courier-bold-o-normal--0-%d-0-0-p-0-iso8859-1");
                    }

// GTech-port (Bug #5078958)
                    availableFontFamilyNames = 
                        getAvailableFontFamilyNamesInit();
// GTech-port (Bug #5078958)
						
                    return null;
                }
            }
        );
    }
	
    private static FileIO tryOpeningFontProp(FileIO f, String homedir, String language, String ext) {
        if (f != null)
            return f;       // already validated
		
        String filename = homedir + FileIO.separator
            + "lib" + FileIO.separator
            + "font.properties";
		
        if (language != null) {
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
}
