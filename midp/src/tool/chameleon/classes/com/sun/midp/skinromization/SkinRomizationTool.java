/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * This tool takes XML file describing skin as input and produces Java
 * and C files with romized skin's properties values. Those files are
 * intended to be compiled with the rest of Chameleon source files.
 *
 */

package com.sun.midp.skinromization;

import java.io.*;
import java.util.*;

import javax.xml.parsers.*;
import org.w3c.dom.*;

import java.awt.image.*;
import java.awt.*;
import com.sun.midp.imageutil.*;
import java.lang.reflect.*;
import com.sun.midp.romization.*;
import com.sun.midp.chameleon.skins.resources.*;

/**
 * Represents romization job
 */
class RomizationJob {
    /** XML file name to get skin properties from */
    public String skinXMLFileName = "";

    /** Skin images directory name */
    public String skinImagesDirName = "";

    /** Output file name for RomizedSkin class */
    public String outBinFileName = "";

    /** Output file name for romized images data */
    public String outCFileName = "";

    /** For QA purposes: overrides images romization settings from XML */
    public String imageRomOverride = "";

    /**
     * Forces images romization overriding the settings from XML
     * and romization of skin.bin.
     */
    public boolean romizeAll = false;
}


/**
 * Main tool class
 */
public class SkinRomizationTool {
    /** Romizer that does actual romization */
    private static SkinRomizer romizer = null;

    /** Romization job to perform */
    private static RomizationJob romizationJob = null;

    /** Print debug output while running */
    private static boolean debug = false;
    
    /** Print usage info and exit */
    private static boolean printHelp = false;

    /**
     * Main method
     *
     * @param args Command line arguments
     */
    public static void main(String[] args) {
        try {
            parseArgs(args);
            if (printHelp) {
                printHelp();
                return;
            }
            
            if (!validateParsedArgs()) {
                System.exit(1);
            }
            
            romizer = new SkinRomizer(debug);
            romizer.romize(romizationJob);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    /**
     * Parses command line arguments
     *
     * @param args command line arguments
     */
    private static void parseArgs(String[] args)
    {
        romizationJob = new RomizationJob();

        for (int i = 0; i < args.length; ++i) {
            String arg = args[i];
            if (arg.equals("-help")) {
                printHelp = true;
                break;
            } else if (arg.equals("-xml")) {
                romizationJob.skinXMLFileName = args[++i];
            } else if (arg.equals("-outbin")) {
                romizationJob.outBinFileName = args[++i];
            } else if (arg.equals("-imagedir")) {
                romizationJob.skinImagesDirName = args[++i];
            } else if (arg.equals("-outc")) {
                romizationJob.outCFileName = args[++i];
            } else if (arg.equals("-romizeall")) {
                romizationJob.romizeAll = true;
            } else if (arg.equals("-debug")) {
                debug = true;
            // this option is for QA purposes only and therefore
            // hidden, undocumented and unsupported
            } else if (arg.equals("-qaimagerom")) {
                romizationJob.imageRomOverride = args[++i];
            } else {
                throw new IllegalArgumentException("invalid option \"" 
                        + args[i] + "\"");
            }
        }
    }

    /**
     * Validates parsed arguments, printing error message
     * if some arg is invalid.
     *
     * @return true, if all arguments are valid
     */
    private static boolean validateParsedArgs() {
        File f = new File(romizationJob.skinXMLFileName);
        if (!f.isFile()) {
            System.err.println(
                    "SkinRomizationTool: Couldn't find input XML file: "
                    + '"' + romizationJob.skinXMLFileName + '"');

            return false;
        }

        f = new File(romizationJob.skinImagesDirName);
        if (!f.isDirectory()) {
            System.err.println("SkinRomizationTool: " + 
                    '"' + romizationJob.skinImagesDirName + '"' 
                    + " isn't a directory");

            return false;
        }
        
        return true;
    }
    
    /**
     * Prints usage information
     */
    private static void printHelp() {
        /**
         * Following options are recognized:
         * -xml:        XML file describing skin.
         * -outbin:     Output binary file. Has no effect
         *              if -romizeall option is given.
         * -outc:       Output C file. If empty, output will be to stdout.
         * -romizeall:  Forces romization of all Chameleon resources.
         * -help:       Print usage information.
         * -debug:      Be verbose: print some debug info while running. 
         *
         */
        System.err.println("Usage: java -jar "
            + "com.sun.midp.SkinRomizationTool "
            + "-xml <localXMLFile> " 
            + "-imagedir <skinImagesDirName> "
            + "-outbin <localOutputBinFile> "
            + "-outc <localOutputCFile> "
            + "[-romizeall] "
            + "[-debug] "
            + "[-help]");
    }
}

/**
 * Base class for all skin properties
 */
abstract class SkinPropertyBase {
    
    /** Print some debug info */
    static boolean debug = false;
    
    /** Name of property ID */
    String idName;

    /** Integer number used as property ID */
    int id;

    /** Offset of the property value(s) in romized values array */
    int valueOffset;
    
    /**
     * Constructor
     *
     * @param idName name of the property's ID
     * @param id property's ID
     * @exception IllegalArgumentException if some of parameters 
     * are invalid
     */
    SkinPropertyBase(String idName, String id) {
        this.idName = idName;

        try {
            this.id = Integer.parseInt(id);
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(
                    "Skin property " + '"' + idName + '"' +
                    " has illegal ID value: " + '"' + id + '"');
        }

        // offset will be set later
        this.valueOffset = -1;
    }

    /**
     * Tests if two properties have the same value
     *
     * @param prop property to compare value with
     * @return true if properties have the same value
     */
    abstract boolean isEqualValue(SkinPropertyBase prop);

    /**
     * How many entries in values array is required for 
     * storing this property value
     *
     * @return number of array entries required for storing
     * the value of this property
     */
    abstract int getValueOffsetDelta();

    /**
     * Prints values array entries for this property's value
     *
     * @param out where to print entries
     */
    abstract void outputValue(BinaryOutputStreamExt out)
        throws java.io.IOException;

    /**
     * Factory method: constructs SkinProperty from DOM node that
     * corresponds to the property description in XML file.`
     *
     * @param n DOM node that corresponds to the property 
     *          description in XML file
     * @return SkinPropertyBase object constructed from DOM node
     * @exception IllegalArgumentException if DOM node is invalid
     */
    static SkinPropertyBase valueOf(Node n) 
        throws IllegalArgumentException {
            
        if (debug) {
            System.err.println("Property node: " + n.getNodeName());
        }

        SkinPropertyBase p = null;
            
        String idName = getAttributeValue(n, "Key");
        String id = getAttributeValue(n, "KeyValue");
        String value = getAttributeValue(n, "Value");
         
        if (debug) {
            System.err.println("\tID name: " + idName);
            System.err.println("\tID: " + id);
            System.err.println("\tValue: " + value);
            System.err.println("");
        }

        String nodeName = n.getNodeName();

        if (nodeName.equals("integer")) {
            p = new IntSkinProperty(idName, id, value);
        } else if (nodeName.equals("integer_seq")) {
            p = new IntSeqSkinProperty(idName, id, value);
        } else if (nodeName.equals("string")) {
            p = new StringSkinProperty(idName, id, value);
        } else if (nodeName.equals("font")) {
            p = new FontSkinProperty(idName, id, value);
        } else if (nodeName.equals("image")) {
            String isRomized = getAttributeValue(n, "Romized");
            p = new ImageSkinProperty(idName, id, value, isRomized);
        } else if (nodeName.equals("composite_image")) {
            String totalPieces = getAttributeValue(n, "Pieces");
            String isRomized = getAttributeValue(n, "Romized");
            p = new CompositeImageSkinProperty(idName, id, value,
                    totalPieces, isRomized);
        } else {
            throw new IllegalArgumentException(
                    "Illegal skin property node: " + '"' + nodeName + '"');
        }

        return p;
    }

    /**
     * Helper method: gets attribute value from node with some
     * errors checking.
     *
     * @param n DOM node to get attribute value from
     * @param attrName attribute name
     * @return requested attribute value
     */
    private static String getAttributeValue(Node n, String attrName) {
        Element e = (Element)n;
        String attrValue = e.getAttribute(attrName);

        return attrValue.trim();
    }

    /**
     * Helper method for renaming symbolic property value coming
     * from XML file (constant name) into another constant name.
     * Needed for backward compatibility.
     *
     * @param value original value
     * @return renamed value
     */
    protected static String renameSymbolicValue(String value) {
        /**
         * Value can be in form of "Graphics.VALUE" where "VALUE" is 
         * the name of one of the constants defined in Graphics class.
         * Convert such value into "SkinResourcesConstants.VALUE".
         * The reason why "SkinResourcesConstants.VALUE" is not used 
         * directly in XML file is backward compatibility.
         */
        if (value.startsWith("Graphics.")) {
            int dotIdx = value.indexOf(".");
            value = "SkinResourcesConstants" + value.substring(dotIdx);
        }

        /**
         * Value can be in form of "ScrollIndSkin.VALUE" where "VALUE" is 
         * the name of one of the constants defined in 
         * ScrollIndResourcesConstants class.
         * Convert such value into "ScrollIndResourcesConstants.VALUE".
         * The reason why "ScrollIndResourcesConstants.VALUE" is not 
         * used directly in XML file is backward compatibility.
         */
        if (value.startsWith("ScrollIndSkin.")) {
            int dotIdx = value.indexOf(".");
            value = "ScrollIndResourcesConstants" + value.substring(dotIdx);
        }

        /**
         * Value can be in form of "FontResources.VALUE" where "VALUE" is 
         * the name of one of the constants defined in 
         * FontResourcesConstants class.
         * Convert such value into "FontResourcesConstants.VALUE".
         * The reason why "FontResourcesConstants.VALUE" is not 
         * used directly in XML file is backward compatibility.
         */
        if (value.startsWith("FontResources.")) {
            int dotIdx = value.indexOf(".");
            value = "FontResourcesConstants" + value.substring(dotIdx);
        }

        return value;
    }

    /**
     * Helper method for convertic symbolic property value into int value.
     * Basically, it converts integer constant name into constant value.
     *
     * @param value symbolic value
     * @return integer value
     */
    protected static int symbolicValueToInt(String value) {
        int dotIdx = value.indexOf(".");
        if (dotIdx == -1 || dotIdx == value.length() - 1) {
            throw new IllegalArgumentException();
        }

        String className = value.substring(0, dotIdx);
        if (className.length() == 0) {
            throw new IllegalArgumentException(); 
        }

        className = "com.sun.midp.chameleon.skins.resources." + className;
        Class c = null;
        try {
            c = Class.forName(className);
        } catch (Throwable t) {
            throw new IllegalArgumentException();
        }

        String fieldName = value.substring(dotIdx + 1);
        if (fieldName.length() == 0) {
            throw new IllegalArgumentException();
        }

        Field f = null;
        try {
            f = c.getField(fieldName);
        } catch (Throwable t) {
            throw new IllegalArgumentException();
        }

        int val = 0;
        try {
            val = f.getInt(null);
        } catch (Throwable t) {
            throw new IllegalArgumentException();
        }

        return val;
    }

    /**
     * Property value can be primitive arithmetic expression. 
     * This helper method evaluates such expression. Operators
     * precedence is simple, left to right.
     *
     * @param exp expression to evaluate
     * @return integer expression value
     */
    protected static int evalValueExpression(String exp) {
        int result = 0;
        char op = '=';

        StringTokenizer st = new StringTokenizer(exp);
        while (true) {
            String v = renameSymbolicValue(st.nextToken());

            int intVal = 0;
            try {
                int radix = 10;
                if (v.startsWith("0x")) {
                    v = v.substring(2);
                    radix = 16;
                }
                intVal = Integer.parseInt(v, radix);
            } catch (NumberFormatException e) {
                intVal = symbolicValueToInt(v);
            }

            switch (op) {
                case '=':
                    result = intVal;
                    break;

                case '|':
                    result |= intVal;
                    break;

                default:
                    throw new IllegalArgumentException();
            }

            if (!st.hasMoreTokens()) {
                break;
            }

            String opStr = st.nextToken();
            if (opStr.length() > 1) {
                throw new IllegalArgumentException(); 
            }
            op = (opStr.toCharArray())[0];
        }

        return result;
    }
}

/**
 * Integer property
 */
class IntSkinProperty extends SkinPropertyBase {
    
    /**
     * Property's value: integer number 
     */
    int value;

    /**
     * Constructor
     * 
     * @param idName name of the property's ID
     * @param id property's ID
     * @param value integer value as string
     */
    IntSkinProperty(String idName, String id, String value) {
        super(idName, id);

        String v = value;
        try {
            this.value = evalValueExpression(v);
        } catch (Throwable t) {
            throw new IllegalArgumentException(
                    "Invalid int property value: " + value);
        }
    }

    /**
     * Tests if two properties have the same value
     *
     * @param prop property to compare value with
     * @return true if properties have the same value
     */
    boolean isEqualValue(SkinPropertyBase prop) {
        if (!(prop instanceof IntSkinProperty)) {
            return false;
        }
        IntSkinProperty p = (IntSkinProperty)prop;

        return (p.value == value);
    }

    /**
     * How many entries in values array is required for 
     * storing this property value
     *
     * @return number of array entries required for storing
     * the value of this property
     */
    int getValueOffsetDelta() {
        // For performance optimization, integers values aren't stored in
        // separate values array as in case for other properties. So there
        // is no offset delta for integer value.
        return 0;
    }

    /**
     * Prints values array entries for this property's value
     *
     * @param out where to print entries
     */
    void outputValue(BinaryOutputStreamExt out)
        throws java.io.IOException {

        out.writeInt(value);
    }
}

/**
 * Integers sequence property
 */
class IntSeqSkinProperty extends SkinPropertyBase {

    /**
     * Propety's value: array of integers 
     */
    int[] value;

    /**
     * Constructor
     *
     * @param idName name of the property's ID
     * @param id property's ID
     * @param value comma separated integer values as string
     */
    IntSeqSkinProperty(String idName, String id, String value) {
        super(idName, id); 
        
        // find number of integers in sequence
        int seqLen = 0;
        char[] chars = value.toCharArray();
        int numStart = 0;
        for (int i = 0; i <= chars.length; ++i) {
            if (i == chars.length || chars[i] == ',') {
                seqLen++;
                numStart = i + 1;
            }
        }

        this.value = new int[seqLen];
        
        // fill values array
        int idx = 0;
        numStart = 0;
        for (int i = 0; i <= chars.length; ++i) {
            if (i == chars.length || chars[i] == ',') {
                String v = (value.substring(numStart, i)).trim();
                int intVal = 0;
                try {
                    intVal = evalValueExpression(v);
                } catch (Throwable t) {
                    throw new IllegalArgumentException(
                        "Invalid integer sequence property value: " + value);
                }
                
                this.value[idx++] = intVal;
                numStart = i + 1;
            }
        }
    }

    /**
     * Tests if two properties have the same value
     *
     * @param prop property to compare value with
     * @return true if properties have the same value
     */
    boolean isEqualValue(SkinPropertyBase prop) {
        if (!(prop instanceof IntSeqSkinProperty)) {
            return false;
        }
        IntSeqSkinProperty p = (IntSeqSkinProperty)prop;
        
        if (p.value.length != value.length) {
            return false;
        }
        
        for (int i = 0; i < value.length; ++i) {
            if (p.value[i] != value[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * How many entries in values array is required for 
     * storing this property value
     *
     * @return number of array entries required for storing
     * the value of this property
     */
    int getValueOffsetDelta() {
        // integers sequence value is stored as length of 
        // the sequence followed by sequence integers
        return (1 + value.length);
    }

    /**
     * Prints values array entries for this property's value
     *
     * @param out where to print entries
     */
    void outputValue(BinaryOutputStreamExt out)
        throws java.io.IOException {

        // out sequence length
        out.writeInt(value.length);

        // out sequence integers
        for (int i = 0; i < value.length; ++i) {
            out.writeInt(value[i]);
        }
    }
}

/**
 * String property
 */
class StringSkinProperty extends SkinPropertyBase {

    /**
     * Property's value: string
     */
    String value;

    /**
     * Constructor
     *
     * @param idName name of the property's ID
     * @param id property's ID
     * @param value string value
     */
    StringSkinProperty(String idName, String id, String value) {
        super(idName, id);
        this.value = value;
    }

    /**
     * Tests if two properties have the same value
     *
     * @param prop property to compare value with
     * @return true if properties have the same value
     */
    boolean isEqualValue(SkinPropertyBase prop) {
        if (!(prop instanceof StringSkinProperty)) {
            return false;
        }
        StringSkinProperty p = (StringSkinProperty)prop;

        return p.value.equals(value);
    }

    /**
     * How many entries in values array is required for 
     * storing this property value
     *
     * @return number of array entries required for storing
     * the value of this property
     */
    int getValueOffsetDelta() {
        return 1;
    }

    /**
     * Prints values array entries for this property's value
     *
     * @param out where to print entries
     */
    void outputValue(BinaryOutputStreamExt out)
        throws java.io.IOException {

        out.writeString(value);
    }
}

/**
 * Font property
 */
class FontSkinProperty extends SkinPropertyBase {
    
    /**
     * Property's value: integer font type 
     */
    int value;

    /**
     * Constructor
     *
     * @param idName name of the property's ID
     * @param id property's ID
     * @param value integer font ID as string
     */
    FontSkinProperty(String idName, String id, String value) {
        super(idName, id);

        String v = value;
        try {
            this.value = evalValueExpression(v);
        } catch (Throwable t) {
            throw new IllegalArgumentException(
                    "Invalid font property value: " + value);
        }
    }

    /**
     * Tests if two properties have the same value
     *
     * @param prop property to compare value with
     * @return true if properties have the same value
     */
    boolean isEqualValue(SkinPropertyBase prop) {
        if (!(prop instanceof FontSkinProperty)) {
            return false;
        }
        FontSkinProperty p = (FontSkinProperty)prop;

        return (p.value == value);
    }

    /**
     * How many entries in values array is required for 
     * storing this property value
     *
     * @return number of array entries required for storing
     * the value of this property
     */
    int getValueOffsetDelta() {
        return 1;
    }

    /**
     * Prints values array entries for this property's value
     *
     * @param out where to print entries
     */
    void outputValue(BinaryOutputStreamExt out)
        throws java.io.IOException {

        out.writeInt(value);
    }
}

/**
 * Image property
 */
class ImageSkinProperty extends SkinPropertyBase {

    /**
     * Property's value: image file name without extension
     */
    String value;

    /**
     * Romize this image
     */
    boolean isRomized;

    /**
     * True, if image value has been specified,
     * i.e. it is not an empty string
     */
    boolean hasValue;

    /** total unique images for all properties */
    static int totalImages;
    
    /**
     * Constructor
     *
     * @param idName name of the property's ID
     * @param id property's ID
     * @param value image file name without extension
     * @param isRomized boolean value as string
     * @exception IllegalArgumentException if some of parameters
     * are invalid
     */
    ImageSkinProperty(String idName, String id, String value, 
            String isRomized) 
        throws IllegalArgumentException {
            
        super(idName, id);
        this.value = value;

        if (isRomized.equals("true")) {
            this.isRomized = true; 
        } else if (isRomized.equals("false")) {
            this.isRomized = false;
        } else {
            throw new IllegalArgumentException(
                    "Skin property " + '"' + idName + '"' +
                    " has illegal romization value: " + 
                    '"' + isRomized + '"');
        }

        hasValue = true;
        if (value.equals("")) {
            hasValue = false;
        }       
    }

    /**
     * Tests if two properties have the same value
     *
     * @param prop property to compare value with
     * @return true if properties have the same value
     */
    boolean isEqualValue(SkinPropertyBase prop) {
        if (!(prop instanceof ImageSkinProperty)) {
            return false;
        }
        ImageSkinProperty p = (ImageSkinProperty)prop;

        return p.value.equals(value);
    }

    /**
     * How many entries in values array is required for 
     * storing this property value
     *
     * @return number of array entries required for storing
     * the value of this property
     */
    int getValueOffsetDelta() {
        return 1;
    }

    /**
     * Prints values array entries for this property's value
     *
     * @param out where to print entries
     */
    void outputValue(BinaryOutputStreamExt out)
        throws java.io.IOException {

        out.writeString(value);
    }
}

/**
 * Composite image property. Composite image is an image that consists
 * of several pieces, where each piece is some image file.
 */
class CompositeImageSkinProperty extends SkinPropertyBase {
    /**
     * Property's value: pieces files names without extension.
     * File name for each piece is constructed from base prefix by
     * addition of piece number, i.e. if the prefix is "image" and 
     * number of pieces is 2, then file name for first piece is 
     * "image0" and for the second piece is "image1".
     */
    String[] value;

    /**
     * Number of pieces
     */
    int totalPieces;

    /**
     * Romize this image
     */
    boolean isRomized;

    /**
     * True, if image value has been specified,
     * i.e. it is not an empty string
     */
    boolean hasValue;
    
    /** total unique images for all properties */
    static int totalImages;

    /**
     * Constructor
     *
     * @param idName name of the property's ID
     * @param id property's ID
     * @param value base prefix for pieces files names
     * @param totalPieces number of pieces in composite image
     * @param isRomized boolean value as string
     * @exception IllegalArgumentException if some of parameters
     * are invalid
     */
    CompositeImageSkinProperty(String idName, String id, String value, 
            String totalPieces, String isRomized) 
        throws IllegalArgumentException {
        
        super(idName, id);
        

        try {
            this.totalPieces = Integer.parseInt(totalPieces);
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException(
                    "Composite image skin property " + '"' + idName + '"' +
                    " has illegal total pieces value: " + '"' + 
                    totalPieces + '"');
        }


        this.value = new String[this.totalPieces];
        for (int i = 0; i < this.totalPieces; ++i) {
            String v = "";
            if (!value.equals("")) {
                v = value + i;
            }
            
            this.value[i] = v;
        }

        if (isRomized.equals("true")) {
            this.isRomized = true; 
        } else if (isRomized.equals("false")) {
            this.isRomized = false;
        } else {
            throw new IllegalArgumentException(
                    "Skin property " + '"' + idName + '"' +
                    " has illegal romization value: " + 
                    '"' + isRomized + '"');
        }

        hasValue = true;
        if (value.equals("")) {
            hasValue = false;
        }
    }

    /**
     * Tests if two properties have the same value
     *
     * @param prop property to compare value with
     * @return true if properties have the same value
     */
    boolean isEqualValue(SkinPropertyBase prop) {
        if (!(prop instanceof CompositeImageSkinProperty)) {
            return false;
        }
        CompositeImageSkinProperty p = (CompositeImageSkinProperty)prop;

        if (p.value.length != value.length) {
            return false;
        }
        
        boolean equal = true;
        for (int i = 0; i < value.length; ++i) {
            if (!(p.value[i].equals(value[i]))) {
                equal = false;
                break;
            }
        }

        return equal;
    }

    /**
     * How many entries in values array is required for 
     * storing this property value
     *
     * @return number of array entries required for storing
     * the value of this property
     */
    int getValueOffsetDelta() {
        // for composite image, values array contain 
        // file names of image pieces
        return value.length;
    }

    /**
     * Prints values array entries for this property's value
     *
     * @param out where to print entries
     */
    void outputValue(BinaryOutputStreamExt out)
        throws java.io.IOException {

        // output pieces file names
        for (int i = 0; i < value.length; ++i) {
            out.writeString(value[i]);
        }
    }
}

/**
 * Represents romized image
 */
final class RomizedImage extends RomizedByteArray {
    /** romized image index */
    int imageIndex;

    /**
     * Constructor
     *
     * @param data romized image data
     * @param index romized image index
     */
    RomizedImage(byte data[], int index) {
        super(data);
        this.imageIndex = index;
    }
}

/**
 * Creates RomizedImage instances
 */
final class RomizedImageFactory {
    /** for converting image into raw format */
    ImageToRawConverter converter;
    
    /** romized images counter */
    int romizedImageCounter = 0;
    
    /**
     * Constructor
     *
     * @param converter converter to use for converting images
     * into raw format
     */
    private RomizedImageFactory(ImageToRawConverter converter) {
        this.converter = converter;
    }
    
    /**
     * Creates RomizedImage from BufferedImage object
     *
     * @param image image to create romized image from
     * @param imageIndex romized image index
     * @return created RomizedImage
     */
    RomizedImage createFromBufferedImage(BufferedImage image,
            int imageIndex) {

        int width = image.getWidth(null);
        int height = image.getHeight(null);
        boolean hasAlpha = image.getColorModel().hasAlpha();
        int[] imageData = getBufferedImageData(image);
        
        byte[] rawData = converter.convertToRaw(imageData, width, height, 
                hasAlpha);

        return new RomizedImage(rawData, romizedImageCounter++);
    }
    
    /**
     * Utility method. Gets BuffredImage data as in array.
     *
     * @param image BufferedImage to return data for
     * @return image data as byte array
     */
    private static int[] getBufferedImageData(BufferedImage image) {
        int width = image.getWidth(null);
        int height = image.getHeight(null);
        BufferedImage bi = new BufferedImage(width, height, 
                BufferedImage.TYPE_INT_ARGB);
        
        Graphics g = bi.getGraphics(); 
        try {
            g.drawImage(image, 0, 0, width, height, null);
        } finally {
            g.dispose();
        }
        
        DataBuffer srcBuf = bi.getData().getDataBuffer();
        int[] buf = ((DataBufferInt)srcBuf).getData();

        return buf;
    }

    /**
     * Returns factory class instance
     *
     * @param rawFormat format of raw file
     * @param colorFormat format of pixel in raw file
     * @param endian romized image data endianess
     * @return RomizedImage factory
     */
    static RomizedImageFactory getFactory(int rawFormat, int colorFormat, 
            int endian) {

        ImageToRawConverter converter = new ImageToRawConverter(rawFormat, 
                colorFormat, endian);

        return new RomizedImageFactory(converter);
    }
}

/**
 * Binary output stream capable of writing data 
 * in big/little endian format.
 */
final class BinaryOutputStreamExt extends BinaryOutputStream {
    /**
     * Constructor
     *
     * @param out underlying output stream for writing bytes into
     * @param isBigEndian true for big endian format, false for little
     */
    BinaryOutputStreamExt(OutputStream out, boolean isBigEndian) {
        super(out, isBigEndian);
    }

    /**
     * Writes string into stream. The string data is written 
     * in follwoing order:
     * - Number of bytes for string chars
     * - Encoding (US ASCII or UTF8)
     * - String chars as bytes
     * 
     * The number of bytes for string chars is written as 
     * single byte, so it can't exceed 255.
     *
     * @param value String value to write into stream
     */
    public void writeString(String value) 
        throws java.io.IOException {

        byte[] chars = value.getBytes("UTF8");
        int length = chars.length;

        // determine what encoding to use
        int encoding = SkinResourcesConstants.STRING_ENCODING_USASCII;
        for (int i = 0; i < length; ++i) {
            int ch = chars[i] & 0xFF;
            if (ch >= 128) {
                encoding = SkinResourcesConstants.STRING_ENCODING_UTF8;
                break;
            }
        }

        if (encoding == SkinResourcesConstants.STRING_ENCODING_UTF8) {
            System.err.println("UTF8: " + value);
            // for '\0' at the end of the string
            length += 1;
        }

        // write string data length
        if (length > 255) {
            throw new IllegalArgumentException(
                    "String data length exceeds 255 bytes");
        }
        outputStream.writeByte(length);

        // write string encoding
        outputStream.writeByte(encoding);

        // write string data
        for (int i = 0; i < chars.length; ++i) {
            outputStream.writeByte(chars[i] & 0xFF);
        }

        if (encoding == SkinResourcesConstants.STRING_ENCODING_UTF8) {
            // '\0' at the end of the string
            outputStream.writeByte(0);
        }
    }
}

/**
 * Perform the romization
 */
class SkinRomizer extends RomUtil {
    /** current romization job */
    RomizationJob romizationJob;
    
    /** XML with skin properties as Document */
    private Document domDoc = null;

    /** Be verbose: print some debug info while running */
    private boolean debug = false;

    /** All skin properties */
    Vector allProps = null;

    /** All integers sequence skin properties */
    Vector intSeqProps = null;

    /** All string skin properties */
    Vector stringProps = null;

    /** All integers font skin properties */
    Vector fontProps = null;

    /** All image skin properties */
    Vector imageProps = null;

    /** All composite image skin properties */
    Vector compImageProps = null;

    /** All romized images */
    Vector romizedImages = null;

    /** Romized image factory */
    RomizedImageFactory romizedImageFactory;

    /** Binary output file stream */
    BinaryOutputStreamExt outputStream = null;
    
    /** raw image file format */
    int rawFormat = ImageToRawConverter.FORMAT_INVALID;

    /** raw image color format */
    int colorFormat = ImageToRawConverter.FORMAT_INVALID;

    /** raw image data endianess */
    int endianFormat = ImageToRawConverter.FORMAT_INVALID;

    /** array of strings used in XML file to specify raw image format */
    static String[] imageFormatStrings = {
        // raw file formats 
        "Putpixel",
        "ARGB",
        // endianess
        "Little",
        "Big",
        // supported pixel formats
        "565",
        "888",
    };

    /** array of constants corresponding to strings above */
    static int[] imageFormats = {
        // raw file formats 
        ImageToRawConverter.RAW_FORMAT_PP,
        ImageToRawConverter.RAW_FORMAT_ARGB,
        // endianess
        ImageToRawConverter.INT_FORMAT_LITTLE_ENDIAN,
        ImageToRawConverter.INT_FORMAT_BIG_ENDIAN,
        // supported pixel formats        
        ImageToRawConverter.COLOR_FORMAT_565,
        ImageToRawConverter.COLOR_FORMAT_888
    };
    
    /**
     * Constructor
     *
     * @param dbg print some debug output while running
     */
    public SkinRomizer(boolean dbg)
    {
        debug = dbg;
        SkinPropertyBase.debug = dbg;

    }

    /**
     * Does the actual romization
     * 
     * @param romizationJob romization to perform
     * @exception Exception if there was an error during romization
     */
    public void romize(RomizationJob romizationJob) 
        throws Exception {

        this.romizationJob = romizationJob;
            
        allProps = new Vector();
        intSeqProps = new Vector();
        stringProps = new Vector();
        fontProps = new Vector();
        imageProps = new Vector();
        compImageProps = new Vector();

        // load XML file as DOM tree 
        DocumentBuilderFactory domFactory = 
            DocumentBuilderFactory.newInstance();

        // do not validate input XML file,
        // we assume it has been validated before
        domFactory.setValidating(false);

        DocumentBuilder domBuilder = domFactory.newDocumentBuilder();
        domDoc = domBuilder.parse(new File(romizationJob.skinXMLFileName));

        // traverse DOM tree constructed from input XML and
        // collect all skin properties described there
        collectSkinProperties(domDoc.getDocumentElement());

        // now, when we know format of romized images,
        // we can create romized images factory
        romizedImageFactory = RomizedImageFactory.getFactory(
                rawFormat, colorFormat, endianFormat);
        
        // assign offsets
        assignPropertiesValuesOffsets(intSeqProps, 0);
        assignPropertiesValuesOffsets(stringProps, 0);
        assignPropertiesValuesOffsets(fontProps, 0);
        
        // values for composite image properties are stored in same
        // array as values for image properties. first, values for 
        // image properties are stored, followed by values for 
        // composite image properties.
        int lastOffset = assignPropertiesValuesOffsets(imageProps, 0); 
        ImageSkinProperty.totalImages = lastOffset - 1;
        lastOffset = assignPropertiesValuesOffsets(compImageProps, lastOffset);
        CompositeImageSkinProperty.totalImages = lastOffset - 1;

        // total unique skin images
        int totalImages = ImageSkinProperty.totalImages + 
            CompositeImageSkinProperty.totalImages;

        // if image isn't romized, then corresponding value will be null
        romizedImages = new Vector(totalImages);
        romizedImages.setSize(totalImages);

        romizeImages();

        // output generated skin description file
        OutputStream outForSkinDescr;

        if (!romizationJob.romizeAll) {
            makeDirectoryTree(romizationJob.outBinFileName);

            outForSkinDescr = new BufferedOutputStream(new FileOutputStream(
                    romizationJob.outBinFileName), 8192);
        } else {
            outForSkinDescr = new ByteArrayOutputStream(8192);
        }
        outputStream = new BinaryOutputStreamExt(outForSkinDescr,
                endianFormat == ImageToRawConverter.INT_FORMAT_BIG_ENDIAN);

        writeBinHeader();
        writeRomizedProperties();
        
        // output generated C file with images
        OutputStream outForCFile =
                new FileOutputStream(romizationJob.outCFileName);
        writer = new PrintWriter(new OutputStreamWriter(outForCFile));

        writeCHeader();
        writeRomizedImagesData();
        writeGetMethod();

        if (romizationJob.romizeAll) {
            writeSkinDescription(
                    ((ByteArrayOutputStream)outForSkinDescr).toByteArray());
        } else {
            writeSkinDescription(null);
        }

        outputStream.close();
        writer.close();
    }

    /**
     * Walks the XML tree and collects all skin properties elements
     * 
     * @param n current DOM node in document
     * @exception Exception if there was an error during romization
     */
    private void collectSkinProperties(Node n) 
        throws Exception {
            
        if (n.getNodeName().equals("skin") && (n instanceof Element)) {
            NodeList list = n.getChildNodes();
            for (int i = 0; i < list.getLength(); i++) {
                collectSkinProperties(list.item(i));
            }           
        } else if (n.getNodeName().equals("rawimage") && 
            (n instanceof Element)) {

            collectImageFormat(n);           
        } else if (n.getNodeName().equals("skin_properties") &&
            (n instanceof Element)) {
                
            NodeList list = n.getChildNodes();
            for (int i = 0; i < list.getLength(); i++) {
                collectSkinProperty(list.item(i));
            }
        } else {
            NodeList list = n.getChildNodes();
            for (int i = 0; i < list.getLength(); i++) {
                collectSkinProperties(list.item(i));
            }
        }
    }

    /**
     * Collects raw images format specification
     * @param n current DOM node in document
     * @exception Exception if there was an error during romization
     */
    private void collectImageFormat(Node n) {
        Element e = (Element)n;
        
        String rawFormatStr = e.getAttribute("Format");
        rawFormat = ImageToRawConverter.FORMAT_INVALID; 
        for (int i = 0; i < imageFormatStrings.length; ++i) {
            if (imageFormatStrings[i].equals(rawFormatStr)) {
                rawFormat = imageFormats[i];
                break;
            }
        }
        if (rawFormat == ImageToRawConverter.FORMAT_INVALID) {
            throw new IllegalArgumentException("invalid raw file format " 
                    + '"' + rawFormatStr + '"');
        }
        
        String colorFormatStr = e.getAttribute("Colors");
        colorFormat = ImageToRawConverter.FORMAT_INVALID; 
        for (int i = 0; i < imageFormatStrings.length; ++i) {
            if (imageFormatStrings[i].equals(colorFormatStr)) {
                colorFormat = imageFormats[i];
                break;
            }
        }
        if (colorFormat == ImageToRawConverter.FORMAT_INVALID) {
            throw new IllegalArgumentException("invalid color format " 
                    + '"' + colorFormatStr + '"');
        }

        
        String endianFormatStr = e.getAttribute("Endian");
        endianFormat = ImageToRawConverter.FORMAT_INVALID; 
        for (int i = 0; i < imageFormatStrings.length; ++i) {
            if (imageFormatStrings[i].equals(endianFormatStr)) {
                endianFormat = imageFormats[i];
                break;
            }
        }
        if (endianFormat == ImageToRawConverter.FORMAT_INVALID) {
            throw new IllegalArgumentException("invalid color format " 
                    + '"' + endianFormatStr + '"');
        }

        if (!ImageToRawConverter.isFormatSupported(rawFormat, colorFormat)) {
            throw new IllegalArgumentException(
                    "unsupported romized image format: " 
                    + "raw file " + '"' + rawFormat + '"' 
                    + ", color " + '"' + colorFormat + '"');
        }
    }
    
    /**
     * Collects single skin property element.
     *
     * @param n DOM node corresponding to skin property description
     * in XML file
     * @exception Exception if there was an error during romization
     */
    private void collectSkinProperty(Node n) 
        throws Exception {

        if (n instanceof Element) {
            SkinPropertyBase p = SkinPropertyBase.valueOf(n);

            // ID of the property is the index into skinProperties vector
            if (allProps.size() < p.id + 1) {
                allProps.setSize(p.id + 1);
            }

            if (allProps.elementAt(p.id) != null) {
                throw new IllegalArgumentException(
                        "Duplicate skin property " + p.idName);
            }
            allProps.setElementAt(p, p.id);

            if (p instanceof IntSeqSkinProperty) {
                intSeqProps.add(p);
            } else if (p instanceof StringSkinProperty) {
                stringProps.add(p);
            } else if (p instanceof FontSkinProperty) {
                fontProps.add(p);
            } else if (p instanceof ImageSkinProperty) {
                ImageSkinProperty ip = (ImageSkinProperty)p;
                if (romizationJob.imageRomOverride.equals("all") ||
                        romizationJob.romizeAll) {
                    ip.isRomized = true;
                } else if (romizationJob.imageRomOverride.equals("none")) {
                    ip.isRomized = false;
                }
                
                imageProps.add(p);
            } else if (p instanceof CompositeImageSkinProperty) {
                CompositeImageSkinProperty ip = (CompositeImageSkinProperty)p;
                if (romizationJob.imageRomOverride.equals("all") ||
                        romizationJob.romizeAll) {
                    ip.isRomized = true;
                } else if (romizationJob.imageRomOverride.equals("none")) {
                    ip.isRomized = false;
                }
                
                compImageProps.add(p);
            }
        }
    }
    
    /**
     * Assigns offsets into properties values array
     *
     * @param props properties to assign offsets for
     * @param startOffset offset to start from
     * @return offset past the last property value
     */
    private static int assignPropertiesValuesOffsets(Vector props, 
            int startOffset) {

        // current (last) offset in the values array
        int curOffset = startOffset;
        
        for (int i = 0; i < props.size(); ++i) {
            SkinPropertyBase curP = (SkinPropertyBase)props.elementAt(i);

            // properties with same values share same offset,
            // so look if there already was property with same value
            int sameValueOffset = -1;
            for (int j = i - 1; j >= 0; --j) {
                SkinPropertyBase p = (SkinPropertyBase)props.elementAt(j);
                if (p.isEqualValue(curP)) {
                    sameValueOffset = p.valueOffset;
                    break;
                }
            }

            if (sameValueOffset != -1) {
                // use offset from property with same value
                curP.valueOffset = sameValueOffset;
            } else {
                // this value is new, give it current offset 
                curP.valueOffset = curOffset;
                curOffset += curP.getValueOffsetDelta();
            }
        }

        return curOffset;
    }

    /**
     * Romizes images
     *
     * @exception IOException if there was IO error during romization
     */
    private void romizeImages() 
        throws IOException {

        int maxOffset = -1;
        for (int i = 0; i < imageProps.size(); ++i) {
            ImageSkinProperty p = (ImageSkinProperty)imageProps.elementAt(i);
            if (!(p.isRomized) || !p.hasValue) {
                continue;
            }

            // this property has the same value as some other 
            // property seen before, so skip it
            if (p.valueOffset <= maxOffset) {
                continue;
            }

            romizeImage(p.value, p.valueOffset);
            maxOffset = p.valueOffset;
        }

        for (int i = 0; i < compImageProps.size(); ++i) {
            CompositeImageSkinProperty p = 
                (CompositeImageSkinProperty)compImageProps.elementAt(i);
            if (!(p.isRomized) || !p.hasValue) {
                continue;
            }

            // this property has the same value as some other 
            // property seen before, so skip it
            if (p.valueOffset <= maxOffset) {
                continue;
            }

            for (int j = 0; j < p.value.length; ++j) {
                romizeImage(p.value[j], p.valueOffset + j);
            }
            maxOffset = p.valueOffset;
        }

    }

    /**
     * Romizes single image
     *
     * @param imageName of the image to romize without extension
     * @param imageIndex romized image index
     * @exception IOException if there was IO error during romization
     */
    private void romizeImage(String imageName, int imageIndex) 
        throws IOException {

        // we romize png images only
        String imageFileName = imageName + ".png";
        imageFileName = romizationJob.skinImagesDirName + File.separator + 
            imageFileName;

        System.out.println("     " + imageFileName);
        
        // load image
        BufferedImage image = javax.imageio.ImageIO.read(
                new File(imageFileName));

        // and romize it
        RomizedImage ri = romizedImageFactory.createFromBufferedImage(image,
                imageIndex);

        romizedImages.set(imageIndex, ri);
    }

    /**
     * Writes RomizedSkin class file header
     */
    private void writeBinHeader() 
        throws IOException {

        // write magic sequence
        int magicLength = SkinResourcesConstants.CHAM_BIN_MAGIC.length;
        for (int i = 0; i < magicLength; ++i) {
            byte b = (byte)(SkinResourcesConstants.CHAM_BIN_MAGIC[i] & 0xFF);
            outputStream.writeByte(b);
        }

        // write version info as an array 
        outputStream.writeInt(1); // array size
        outputStream.writeInt(SkinResourcesConstants.CHAM_BIN_FORMAT_VERSION);
    }

    /**
     *  Writes romized images C file header
     */
    private void writeCHeader() {
        writeCopyright();

        pl("");
        pl("#include <string.h>");
        pl("#include <kni.h>");
        pl("#include <midpError.h>");
        pl("#include <midpMalloc.h>");
        pl("#include <midpServices.h>\n");
    }
    
    /**
     * Writes romized properties data
     * @exception Exception if there was an error during output
     */
    private void writeRomizedProperties() 
        throws Exception {
            
        outputStream.writeInt(allProps.size());
        for (int i = 0; i < allProps.size(); ++i) {
            SkinPropertyBase p = (SkinPropertyBase)allProps.elementAt(i);
            if (p instanceof IntSkinProperty) {
                IntSkinProperty intP = (IntSkinProperty)p;
                // for integer property the array holds actual property's 
                // value, not an offset into values array
                p.outputValue(outputStream);
            } else {
                // for all other properties the array holds an offset into
                // values array, in which actual values are stored
                outputStream.writeInt(p.valueOffset);
            }
        }

        writePropertiesValues(intSeqProps);
        writePropertiesValues(stringProps);
        writePropertiesValues(fontProps);

        Vector v = new Vector(imageProps);
        v.addAll(compImageProps);
        writePropertiesValues(v);

        writeRomizedImagesIndexes();
    }
    
    /**
     * Writes properties values array entries
     *
     * @param props properties to write entries for 
     */
    private void writePropertiesValues(Vector props) 
        throws java.io.IOException {

        int maxOffset = -1;
        int totalValues = 0;
        for (int i = 0; i < props.size(); ++i) {
            SkinPropertyBase p = (SkinPropertyBase)props.elementAt(i);

            // this property has the same value as some other 
            // property printed before, so skip it
            if (p.valueOffset <= maxOffset) {
                continue;
            }

            totalValues += p.getValueOffsetDelta();
            maxOffset = p.valueOffset;
        }
        outputStream.writeInt(totalValues);

        maxOffset = -1;
        for (int i = 0; i < props.size(); ++i) {
            SkinPropertyBase p = (SkinPropertyBase)props.elementAt(i);

            // this property has the same value as some other 
            // property outputed before, so skip it
            if (p.valueOffset <= maxOffset) {
                continue;
            }

            p.outputValue(outputStream);
            maxOffset = p.valueOffset;
        }
    }
    
    /**
     * Writes indexes for romized images. 
     */
    private void writeRomizedImagesIndexes() 
        throws java.io.IOException {

        outputStream.writeInt(romizedImages.size());
        for (int i = 0; i < romizedImages.size(); ++i) {
            Object o = romizedImages.elementAt(i);
            // image with index = i isn't romized
            if (o == null) {
                outputStream.writeInt(-1);
            } else {
                RomizedImage ri = (RomizedImage)o;
                outputStream.writeInt(ri.imageIndex);
            }
        }
    }
    
    /**
     *  Writes romized images data
     */
    private void writeRomizedImagesData() {
        int totalRomizedImages = 0;
        for (int i = 0; i < romizedImages.size(); ++i) {
            if (romizedImages.elementAt(i) != null) {
                ++totalRomizedImages;
            }
        }
        
        pl("");
        pl("static const int NUM_ROM_IMAGES = " + totalRomizedImages + ";");
        
        if (totalRomizedImages != 0) {
            // output a structure declaration used for storing 
            // romized images data in it
            pl("");
            pl("struct romized_images_data {");
            for (int i = 0; i < romizedImages.size(); ++i) {
                Object o = romizedImages.elementAt(i);
                if (o != null) {
                    RomizedImage ri = (RomizedImage)o;

                    // this field ensures proper alignment of 
                    // subsequent data array
                    pl("    " + "const int align_" + ri.imageIndex + ";");

                    String dataArrayName = "romized_image" + ri.imageIndex;
                    int dataArrayLength = ri.size();
                    pl("    " + "const unsigned char " + dataArrayName + 
                            "[" + dataArrayLength + "];");
                }
            }
            pl("};");

            pl("");
            pl("static const struct romized_images_data " + 
                    "romized_images_data = {");
            for (int i = 0; i < romizedImages.size(); ++i) {
                Object o = romizedImages.elementAt(i);
                if (o != null) {
                    // alignemnt field
                    pl("    " + "0,");

                    RomizedImage ri = (RomizedImage)o;
                    String dataArrayName = "romized_image" + ri.imageIndex;

                    // romized image data field
                    pl("    " + "/* " + dataArrayName + " */");
                    pl("    " +"{");
                    ri.printDataArray(writer, "        ", 11);
                    pl(" },");
                }
            }
            pl("};");
        }

        pl("");
        pl("static const unsigned char* image_cache[] = {");
        // if there are no romized images, print dummy value
        // to make array non empty and keep compilers happy
        if (totalRomizedImages == 0) {
            pl("    NULL");
        } else {
            for (int i = 0; i < romizedImages.size(); ++i) {
                Object o = romizedImages.elementAt(i);
                if (o != null) {
                    RomizedImage ri = (RomizedImage)o;
                    String dataArrayName = 
                        "romized_images_data.romized_image" + ri.imageIndex;
                    pl("    " + dataArrayName + ",");
                }
            }
        }
        pl("};");

        pl("");
        pl("static const int image_size[] = {"); 
        // if there are no romized images, print dummy value
        // to make array non empty and keep compilers happy
        if (totalRomizedImages == 0) {
            pl("    0");
        } else {
            for (int i = 0; i < romizedImages.size(); ++i) {
                Object o = romizedImages.elementAt(i);
                if (o != null) {
                    RomizedImage ri = (RomizedImage)o;
                    String dataArrayName = 
                        "romized_images_data.romized_image" + ri.imageIndex;
                    pl("    sizeof(" + dataArrayName + "),");
                }
            }
        }
        pl("};");
    }

    /**
     * Writes get method for obtaining romized image data
     */
    void writeGetMethod() {
        pl("");
        pl("/**");
        pl(" * Loads a native image from rom, if present."); 
        pl(" *");
        pl(" * @param imageId    The image id");
        pl(" * @param **bufPtr   Pointer where a buffer will be "
		    + "allocated and data stored");
        pl(" * @return           -1 if failed, else length of buffer");
        pl(" */");
        pl("int lfj_load_image_from_rom(int imageId, "
		    + "unsigned char** bufPtr) {\n");
        pl("    int len = -1;");
        pl("    if ((imageId < 0) || (imageId > NUM_ROM_IMAGES)) {");
        pl("        REPORT_WARN1(LC_LOWUI,"); 
        pl("            \"Warning: could not load romized" +
		    "image for index %d; \", imageId); ");
        pl("        return len;");
        pl("    }\n");
        pl("    *bufPtr = (unsigned char*)image_cache[imageId];");
        pl("    len = image_size[imageId];");
        pl("    return len;");
        pl("}");
    }

    /**
     *
     * @param data data to convert into the C array, can be null
     */
    void writeSkinDescription(byte[] data) {
        pl("");
        pl("static const unsigned char skin_description[] = {");
        if (data != null) {
            new RomizedByteArray(data).printDataArray(writer, "        ", 11);
        } else {
            pl("    0");
        }
        pl("};");

        pl("");
        pl("/**");
        pl(" * Loads a ROMized skin description from ROM, if present.");
        pl(" *");
        pl(" * @return NULL if failed, otherwise a pointer to the skin " +
                "description data");
        pl(" */");
        pl("const unsigned char* lfj_get_skin_description() {");
        if (data != null) {
            pl("    return skin_description;");
        } else {
            pl("    return NULL;");
        }
        pl("}");

        pl("");
        pl("/**");
        pl(" * Retrieves the size of the skin description data.");
        pl(" *");
        pl(" * @return -1 if failed, otherwise a size of the skin " +
                "description data");
        pl(" */");
        pl("int lfj_get_skin_description_size() {");
        if (data != null) {
            pl("    return " + data.length + ";");
        } else {
            pl("    return -1;");
        }
        pl("}");
    }

    /**
     * Creates a directory structure.
     *
     * @param fullFileName Full path to the file to be created. If directory
     * in which file is to be created doesn't exists, it will be created
     * @exception IOException is thrown if directory couldn't be created 
     */
    private void makeDirectoryTree(String fullFileName) 
        throws IOException {

        if (debug == true) {
            System.out.println("mkdir: " + fullFileName);
        }
        int index = fullFileName.lastIndexOf(File.separatorChar);
        if (index == -1) {
            // To be compatible with MKS-hosted build on win32, which
            // does not translate / to \.
            index = fullFileName.lastIndexOf('/');
        }
        File outputDirectory = new File(fullFileName.substring(0, index));

        if (!(outputDirectory).exists()) {
            if (!(outputDirectory).mkdirs()) {
                throw new IOException("failed to create output directory");
            }
        }

    }
}
