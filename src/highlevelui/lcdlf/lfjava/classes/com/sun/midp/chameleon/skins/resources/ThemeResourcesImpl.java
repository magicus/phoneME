/*
 *   
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.chameleon.skins.resources;

import com.sun.midp.security.*;
import com.sun.midp.log.*;
//import com.sun.midp.util.ResourceHandler;

import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Font;
import java.io.*;

import javax.microedition.theme.*;
//import com.sun.theme.*;
import java.util.Enumeration;
import java.util.Hashtable;


/** Utility class for skin resources. */
public class ThemeResourcesImpl extends SkinResources {

    /** Constant to distinguish image resources with no index */
    private static final int NO_INDEX = -1;

    /**
     * Determine how skin resources should be loaded: at once,
     * during skin loading, or lazily, on first use.
     * @return true if all resources should be loaded at once.
     */
    public boolean ifLoadAllResources() {
        return true;
    }

    /*
    static private ThemeResourcesImpl themeResource = new ThemeResourcesImpl();

    static public ThemeResourcesImpl getInstance() {
        return themeResource;
    }
    */

    /**
     * Inner class to request security token from SecurityInitializer.
     * SecurityInitializer should be able to check this inner class name.
     */
    static private class SecurityTrusted
        implements ImplicitlyTrustedClass {};

    /** A private internal reference to the system security token */
    private static SecurityToken secureToken =
        SecurityInitializer.requestToken(new SecurityTrusted());

    /**
     * True, if skin is being loaded in AMS Isolate in MVM case,
     * false otherwise. Always true in SVM case.
     */
//    private static boolean isAmsIsolate = true;

//    private static String activeTheme = "__default__";

    /**
     * This class needs no real constructor, but its here as 'public'
     * so the SecurityIntializer can do a newInstance() on it and call
     * the initSecurityToken() method.
     */
    public ThemeResourcesImpl() {
    }

/*
    public static void activateTheme(String theme) {
      System.out.println("JD: ThemeResourcesImpl.activateTheme(): theme=" + theme);
//      activeTheme = theme;
    }
*/
    /**
     * Load the skin, including all its properties and images. Some parts
     * of the skin may be lazily initialized, but this method starts the
     * process. If the flag to 'reload' is true, the method will ignore
     * all previously loaded resources and go through the process again.
     *
     * @param reload if true, ignore previously loaded resources and reload
     * @throws IOException if there was error reading skin data file
     * @throws IllegalStateException if skin data file is invalid
     */
    public void loadSkin(boolean reload) 
        throws IllegalStateException, IOException {

        
        // NOTE: The remaining properties classes need to be set and
        // possibly re-loaded lazily. 
        
        // IMPL_NOTE if (reload), need to signal the LF classes in a way they
        // will re-load their skin resources on demand
    }

    /**
     * Utility method used by skin property classes to load
     * image resources.
     * 
     * @param identifier a unique identifier for the image property
     * @return the Image if one is available, null otherwise
     */
    public Image getImage(int identifier) {
        return getImage(identifier, NO_INDEX);
    }

    private Element getElement(int identifier) {

        Element element = null;
        ChameleonVsElements.ChamElementInfo elementInfo =
                ChameleonVsElements.elements[identifier];

        if(elementInfo != null) {
            try {

                Theme theme = ThemeManager.getInstance().getActiveTheme();
                element = theme.getToolkitElement(elementInfo.feature,
                        elementInfo.role, elementInfo.toolkitName);
            } catch (ElementNotFoundException ex) {
                if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                    Logging.report(Logging.ERROR, LogChannels.LC_JSR258,
                        "Can't find theme element (feature=" +
                        elementInfo.feature + ", role=" +
                        elementInfo.role + ")");
                }
            }
        } else {
            if (Logging.REPORT_LEVEL <= Logging.CRITICAL) {
                Logging.report(Logging.CRITICAL, LogChannels.LC_JSR258,
                     "Can't find Chameleon element (id=" + identifier + ")");
            }
        }
        return element;
    }

    private Object getContent(int identifier) {

        Element element = getElement(identifier);
        return (element != null) ? element.getContent() : null;
    }

    /**
     * Utility method used by skin property classes to load
     * image resources.
     * 
     * @param identifier a unique identifier for the image property
     * @param index index of the image
     *
     * @return the Image if one is available, null otherwise
     */
    public Image getImage(int identifier, int index) {

        Image image = null;

        MediaObject mediaObject =  (MediaObject) getContent(identifier);
        if(mediaObject != null) {
            try {
                byte[] mediaData = mediaObject.getData();
                image = Image.createImage(mediaData, 0, mediaData.length);
            } catch(Exception ex) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_JSR258,
                         "Can't get resource image (id=" + identifier + ")");
                }
            }
        }
 
        return image;
    }

    /**
     * Utility method used by skin property classes to load
     * composite image resources consisting of a few images.
     *
     * @param identifier a unique identifier for the composite image property
     * @param piecesNumber number of pieces consisting the composite image 
     *
     * @return the Image[] with loaded image pieces,
     * or null if some of the pieces is not available
     */
    public Image[] getCompositeImage(
            int identifier, int piecesNumber) {

        ElementGroup elGroup = (ElementGroup) getElement(identifier);
        if(elGroup == null) {
            return null;
        }

        if(elGroup.getElementCount() != piecesNumber) {
            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_JSR258,
                 "Wrong number of elements for composite image (id=" +
                 identifier + "): " + elGroup.getElementCount());
            }
            return null;
        }

        Image[] images = new Image[piecesNumber];


        int index = 0;
        try {
            for(Enumeration en = elGroup.getElements(); en.hasMoreElements(); ) {
                MediaObject mediaObject = (MediaObject)
                        ((Element) en.nextElement()).getContent();
                byte[] mediaData = mediaObject.getData();
                images[index++] = Image.createImage(mediaData, 0, mediaData.length);
            }
        } catch(Exception ex) {

            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_JSR258,
                 "Can't get resource composite image (id=" +
                 identifier + ")");
            }
            images = null;
        }
        return images;
    }

    /**
     * Utility method used by skin property classes to load
     * Font resources.
     *
     * @param identifier a unique identifier for the Font property
     * @return the Font object or null in case of error
     */     
    public Font getFont(int identifier) {
        return (Font) getContent(identifier);
    }
    
    /**
     * Utility method used by skin property classes to load
     * String resources.
     *
     * @param identifier a unique identifier for the String property
     * @return the String object or null in case of error
     */     
    public String getString(int identifier) {
        return (String) getContent(identifier);
    }
    
    /**
     * Utility method used by skin property classes to load
     * integer resources.
     *
     * @param identifier a unique identifier for the integer property
     * @return an integer or -1 in case of error
     */     
    public int getInt(int identifier) {

        int retValue = -1;
        Element element = (Element) getElement(identifier);
        if(element != null) {
            retValue = getIntOrPresentation(element);
        }
        return retValue;
    }

    /**
     * Converts presentation for the Element into int value or
     * retrieves int from the content of the Element
     * @param element an Element
     * @return the result of conversion as an integer value
     */
    private static int getIntOrPresentation(Element element) {
        int retValue = -1;
        if(element.getPresentation() != null) {
            retValue = getPresentationAsInt(element.getPresentation());
        }
        if(retValue == -1) {
            Integer value = (Integer) element.getContent();
            if(value != null) {
                retValue = value.intValue();
            }
        }
        return retValue;

    }

    /**
     * Returns sequence of integer numbers corresponding to 
     * specified property identifer.
     *
     * @param identifier a unique identifier for the property
     * @return the int[] representing the sequence or null in case of error
     */
    public int[] getNumbersSequence(int identifier) {

        ElementGroup elGroup = (ElementGroup) getElement(identifier);
        if(elGroup == null) {
            return null;
        }

        int[] values = new int[elGroup.getElementCount()];
        int index = 0;
        try {
            for(Enumeration en = elGroup.getElements(); en.hasMoreElements(); ) {
                Element element = (Element) en.nextElement();
                    values[index++] = getIntOrPresentation(element);
            }
        } catch(Exception ex) {
            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_JSR258,
                 "Can't get number sequence (id=" +
                 identifier + ")");
            }
            values = null;
        }
        return values;
    }    

    private static Hashtable hints;

    static {
        hints = new Hashtable();

        Hashtable hintValues = new Hashtable();
        hintValues.put("left", new Integer(Graphics.LEFT));
        hintValues.put("center", new Integer(Graphics.HCENTER));
        hintValues.put("right", new Integer(Graphics.RIGHT));
        hints.put("halign", hintValues);

        hintValues = new Hashtable();
        hintValues.put("top", new Integer(Graphics.TOP));
        hintValues.put("middle", new Integer(Graphics.VCENTER));
        hintValues.put("bottom", new Integer(Graphics.BOTTOM));
        hints.put("valign", hintValues);

        hintValues = new Hashtable();
        hintValues.put("100", new Integer(Graphics.SOLID));
        hints.put("opacity", hintValues);
    }

    private static int getPresentationAsInt(Hashtable elHints) {

        int retValue = 0;
        boolean found = false;
        for(Enumeration en = elHints.keys(); en.hasMoreElements(); ) {
            String hintKey = (String) en.nextElement();
            Hashtable hintValues = (Hashtable) hints.get(hintKey);
            if(hintValues != null) {
                String hintValue = (String) elHints.get(hintKey);
                Integer intValue = (Integer) hintValues.get(hintValue);
                if(intValue != null) {
                    found = true;
                    retValue |= intValue.intValue();
                }
            }
        }
        return found ? retValue : -1;
    }
}
