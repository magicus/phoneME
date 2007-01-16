/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * Transforms a XML file according given XSL file.
 */
package com.sun.configurator;

import java.io.*;
import java.util.*;

import javax.xml.parsers.*;
import javax.xml.transform.*;
import javax.xml.transform.dom.*;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;
import org.w3c.dom.*;
import org.xml.sax.ErrorHandler;
import org.xml.sax.SAXParseException;
import org.xml.sax.SAXException;

/**
 * Represents transformation
 */
class Transformation {
    /** XML file name to apply transformation to */
    public String xmlFileName = "";

    /** XSL files to be applied */
    public String xslFileName = null;

    /** Transformation parameters: names */
    public Vector xslParamNames = new Vector();

    /** Transformation parameters: values */
    public Vector xslparamValues = new Vector();

    /** Output file name */
    public String outFileName = "";

    /** Do input file validation */
    public boolean validate = false;
}


/**
 *  Driver class with main(). Parses command line arguments and invokes
 *  CodeTransformerImpl instance that does all the transformation work.
 */
public class CodeTransformer {
    /** Transformer that does actual transformation */
    private static CodeTransformerImpl transformer = null;

    /** Transformations to perform */
    private static Vector transformations = new Vector();

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

            transformer = new CodeTransformerImpl(debug);
            for (int i = 0; i < transformations.size(); ++i) {
                Transformation tr =
                    (Transformation)transformations.elementAt(i);
                transformer.transform(tr);
            }
        } catch (SAXException e) {
            // error was already reported
            e.printStackTrace();
            System.exit(1);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    /**
     * Parse command line arguments, adding Transformation objects to
     * <tt>transformations</tt> vector for each trnasformation specified.
     *
     * @param args command line arguments
     */
    private static void parseArgs(String[] args)
    {
        Transformation tr = new Transformation();
        for (int i = 0; i < args.length; ++i) {
            String arg = args[i];
            if (debug) {
                System.err.println("arg: " + arg);
            }
            if (arg.equals("-xml")) {
                tr.xmlFileName = args[++i];
            } else if (arg.equals("-xsl")) {
                tr.xslFileName = args[++i];
            } else if (arg.equals("-params")) {
                int j = i + 1;
                for (; j < args.length; j += 2)  {
                    if ('-' == args[j].charAt(0)) {
                        break;
                    }
                    if (debug) {
                       System.err.println("pname : " + (String)args[j]);
                       System.err.println("pvalue: " + (String)args[j + 1]);
                    }
                    tr.xslParamNames.add(args[j]);
                    tr.xslparamValues.add(args[j + 1]);
                }
                i = j - 1;
            } else if (arg.equals("-validate")) {
                tr.validate = true;
            } else if (arg.equals("-out")) {
                tr.outFileName = args[++i];
                // -out ends current transformation arguments,
                // i.e all agruments coming after it belongs
                // to next transformation(s)
                transformations.add(tr);
                tr = new Transformation();
            } else if (arg.equals("-debug")) {
                debug = true;
            } else {
                throw new IllegalArgumentException("invalid option \""
                        + args[i] + "\"");
            }
        }
    }

    /**
     * Print usage information
     */
    private static void printHelp() {
        /**
         * Following options are recognized:
         * -validate:   Do validation of input XML file.
         * -xml:        XML file to transform.
         * -xsl:        XSL file to apply to given XML file.
         * -out:        Output file. If empty, output will be to stdout.
         * -params:     Transformations parameters in form of "Name"
   *              "Value" pairs.
         * -help:       Print usage information
         * -debug:      Be verbose: print some debug info while running.
         *
         * In order to improve performance, CodeTransformer is capable of
         * doing multiple transformations per invokation. -out option marks
         * the end of arguments for single transformation. All arguments
         * after it belongs to next transformation(s).
         */
        System.err.println("Usage: java -jar "
            + "com.sun.midp.configurator.CodeTransformer "
            + "[-validate] "
            + "-xml <localXMLFile> "
            + "-xsl <localXSLFile> "
            + "-params <paramName> <paramValue>... "
            + "-out <localOutputFile> "
            + "-xml <localXMLFile> "
            + "... "
            + "[-debug] "
            + "[-help]");
    }
}


/**
 * Perform the transformation
 */
class CodeTransformerImpl {
    /** Factory constructing Transformer objects */
    private TransformerFactory transformerFactory =
        TransformerFactory.newInstance();

    /**
     * Since most of transformations are applied to the same XML file,
     * we don't want to load it on each transformation, so we cache last
     * used XML file as DOMSource
     */
    /** Last source used */
    private DOMSource lastSource = null;
    /** Last document used */
    private Document lastDoc = null;
    /** File name of the last used source */
    private String lastSourceFileName = null;

    /** Be verbose: print some debug info while running */
    private boolean debug = false;


    /**
     * Constructor
     *
     * @param dbg print some debug output while running
     */
    public CodeTransformerImpl(boolean dbg)
    {
        debug = dbg;
    }

    /**
     * Converts errors.
     */
    class TransformerErrorHandler
        implements ErrorHandler {

  /**
   * Handles errors.
   * @param e the parsing exception
   */
        public void error(SAXParseException e)
            throws SAXParseException {
            reportError(e);
            // rethrow exception to stop processing on first error
            throw e;
        }

  /**
   * Handles fatal errors.
   * @param e the parsing exception
   */
  public void fatalError(SAXParseException e) {
            reportError(e);
        }

  /**
   * Handles warnings.
   * @param e the parsing exception
   */
        public void warning(SAXParseException e) {
            reportError(e);
        }

  /**
   * Outputs diagnostic messages.
   * @param e the parsing exception
   */
        private void reportError(SAXParseException e) {
            String msg = e.getMessage();
            String location = e.getSystemId();
            int line = e.getLineNumber();

            System.err.print("Error: URI=" + location);
            System.err.println(" Line=" + line + ": " + msg);
        }
    }

    /**
     * Do the actual transformation
     *
     * @param tr transformation to perform
     */
    public void transform(Transformation tr)
        throws Exception {

        if (debug) {
            System.err.println("xml file: " + tr.xmlFileName);
            System.err.println("out file: " + tr.outFileName);
            System.err.println("prev xml: " + lastSourceFileName);
        }

        // source XML file
        DOMSource source = null;
        Document doc;
        if (lastSource != null &&
                lastSourceFileName.equals(tr.xmlFileName)) {
            source = lastSource;
            doc = lastDoc;
        } else {
            // load XML file as DOM tree
            DocumentBuilderFactory domFactory =
                DocumentBuilderFactory.newInstance();

            if (tr.validate) {
                domFactory.setValidating(true);
            }
            DocumentBuilder domBuilder = domFactory.newDocumentBuilder();

            domBuilder.setErrorHandler(new TransformerErrorHandler());
            doc = domBuilder.parse(new File(tr.xmlFileName));

            // make source from it
            source = new DOMSource(doc);
        }

        // if output and input files are the same,
        // we can't reuse cached input file since
        // it's going to change
        if ((tr.xmlFileName).equals(tr.outFileName)) {
            lastSource = null;
            lastDoc = null;
            lastSourceFileName = null;
        } else {
            lastSource = source;
            lastDoc = doc;
            lastSourceFileName = tr.xmlFileName;
        }

        // apply XSL stylesheet
        if (debug) {
            System.err.println("xsl file: " + tr.xslFileName);
        }

        if (tr.xslFileName.equals("custom-localized-strings")) {
            generateLocalizedStrings(doc, tr.outFileName);
        } else {
            // output file
            StreamResult outStream = null;
            if (tr.outFileName.length() == 0) {
                // send transformed output to the stdout
                outStream = new StreamResult(System.out);
            } else  {
                // send transformed output to the file
                makeDirectoryTree(tr.outFileName);
                outStream = new StreamResult(new File(tr.outFileName));
            }

            // create Transformer that will aply stylesheet
            StreamSource xslFile = new
                StreamSource(new File(tr.xslFileName));
            Transformer transformer =
                transformerFactory.newTransformer(xslFile);

            // pass parameters to Transformer
            for (int j = 0; j < tr.xslParamNames.size(); ++j) {
                transformer.setParameter(
                    (String)tr.xslParamNames.elementAt(j),
                    tr.xslparamValues.elementAt(j));
            }

            // finally, apply the stylesheet
            transformer.transform(source, outStream);
        }
    }

    /**
     * Creates a directory structure.
     *
     * @param fullFileName Full path to the file to be created. If directory
     * in which file is to be created doesn't exists, it will be created
     * @exception IOException is thrown if directory couldn't be created
     */
    private void makeDirectoryTree(String fullFileName) throws IOException {
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

    /**
     * Generates localized strings.
     * @param doc the current  working file
     * @param outDir the target output directory
     */
    private void generateLocalizedStrings(Document doc, String outDir)
         throws Exception
    {
        processLocalizedStrings(doc.getDocumentElement(), null);
        for (Enumeration e = locales.keys(); e.hasMoreElements();) {
            String key = (String)e.nextElement();
            LocalizedStringSet locale = (LocalizedStringSet)locales.get(key);
            String filename = outDir + File.separatorChar + locale.className;
            makeDirectoryTree(filename);

            // Write LocalizedStrings<locale>.c file
            CSourceWriter cwriter = new CSourceWriter();
            cwriter.writeCSource(locale, filename + ".c", locale.className);

            // Write LocalizedStrings<locale>.java file
            FileOutputStream out = new FileOutputStream(filename + ".java");
            PrintWriter writer = new PrintWriter(new OutputStreamWriter(out));
            writer.println("// This file is auto-generated. Don't edit!");
            writer.println("package com.sun.midp.l10n;");
            writer.println("abstract class " + locale.className + " {");
            writer.println("    native static String getContent(int index);");
            writer.println("}");
            writer.close();
        }
    }

    /**
     * Walk the XML tree to discover all <localized_string> elements,
     * collect them and write the data tables specific for each
     * locale to a pair of Java and C source files.
     * @param n the current node to be processed
     * @param className the name of the class to handle this node
     */
    private void processLocalizedStrings(Node n, String className)
        throws Exception {
        if (n.getNodeName().equals("localized_strings") &&
            (n instanceof Element)) {
            Element e = (Element)n;
            String pkg = e.getAttribute("Package");
            String name = e.getAttribute("Name");
            className = name;
        } else if (n.getNodeName().equals("localized_string") &&
            (n instanceof Element) && className != null) {
            Element e = (Element)n;
            String value = e.getAttribute("Value");
            // we use key value as index
            int valueIndex = Integer.parseInt(e.getAttribute("KeyValue"));

            addLocalizedString(className, valueIndex, value);
        }

        NodeList list = n.getChildNodes();
        for (int i=0; i<list.getLength(); i++) {
            processLocalizedStrings(list.item(i), className);
        }
    }

    /** Supported locales table. */
    Hashtable locales = new Hashtable();

    /**
     * Add one <localized_string> Element to the LocalizedStringSet of
     * the enclosing <localized_strings>.
     * @param className the locales handler
     * @param valueIndex key for this entry
     * @param value data for this entry
     */
    private void addLocalizedString(String className,
            int valueIndex, String value)
        throws Exception
    {
        LocalizedStringSet locale = (LocalizedStringSet)locales.get(className);
        if (locale == null) {
            locale = new LocalizedStringSet(className);
            locales.put(className, locale);
        }
        locale.put(valueIndex, value);
    }
}


/**
 * Records all localized strings in for the locale represented by
 * a given Java class.
 */
class LocalizedStringSet {
    /** This Java class represents a given locale. */
    String className;

    /** All localized strings for this locale */
    Vector strings;

    /**
     * Default constructor.
     * @param className class name of locale handler
     */
    LocalizedStringSet(String className) {
        this.className = className;
        strings = new Vector(512);
    }

    /**
     * Stores the current entry.
     * @param idx the key
     * @param value the data
     */
    void put(int idx, String value) {
        if (strings.size() < idx + 1) {
            strings.setSize(idx + 1);
        }
        strings.set(idx, value);
    }
}

/**
 * This class records related information about each localized string
 */
class LocalizedString {
    /**
     * Constructor with initial data.
     * @param i the key
     * @param s the value
     */
    LocalizedString(int i, String s) {
        this.index = i;
        this.string = s;
    }
    /** The current search location. */
    int index;
    /** The offset of the current item. */
    int offset;
    /** The length of the table. */
    int length;
    /** The data contents. */
    String string;
}

/**
 * This class deals with generating the LocalizedStringsBase<locale>.c
 * files. We try to compress the footprint of the data, since we
 * don't read from the localized string tables very often
 */
class CSourceWriter {
    /** The locale used. */
    LocalizedStringSet locale;
    /** Output stream writer. */
    PrintWriter writer;
    /** Array of strings to be processed. */
    LocalizedString strings[];

    /**
     * Sort all string data so they can be more easily compressed
     * @return the sorted array
     */
    ArrayList sort() {
        ArrayList list = new ArrayList();
        Vector strTable = locale.strings;
        for (int i = 0; i < strTable.size(); ++i) {
            String value = (String)strTable.get(i);

            if (strings[i] != null) {
                System.out.println("Duplicated content definiton for index " +
                                   i);
                throw new Error();
            }
            strings[i] = new LocalizedString(i, value);
            list.add(strings[i]);
        }

  /**
   * Sorting function.
   */
        Collections.sort(list, new Comparator() {
    /**
     * Comparison method.
     * @param o1 left hand operand
     * @param o2 right hand operand
     * @return positive value if o2 is greater than o1,
     * negative is o1 is less than o2 and zero if the same
     */
            public int compare(Object o1, Object o2) {
                LocalizedString r1 = (LocalizedString)o1;
                LocalizedString r2 = (LocalizedString)o2;
                return (r1.string.length() < r2.string.length()) ? 1 : -1;
            }
        });

        return list;
    }

    /**
     * Write the C source file that stores the local string data in a
     * compact data structure
     * @param locale the target locale for comparison purposes
     * @param filename the source for processing
     * @param classname the locale handler
     */
    void writeCSource(LocalizedStringSet locale,
                      String filename, String classname)
         throws IOException
    {
        this.locale = locale;
        this.strings = new LocalizedString[locale.strings.size()];

        FileOutputStream out = new FileOutputStream(filename);
        writer = new PrintWriter(new OutputStreamWriter(out));
        pl("/* This file is auto-generated. Do not edit! */");
        pl("#include <kni.h>");
        pl("#include <string.h>");
        pl("#include <midpMalloc.h>");

        try {
            int type = writeOptimizedArray();

            pl("KNIEXPORT KNI_RETURNTYPE_OBJECT");
            pl("KNIDECL(com_sun_midp_l10n_" + classname + "_getContent) {");
            if (type != UNICODE) {
                pl("    char stackbuffer[128];");
                pl("    char *utf8; /*0-terminated*/");
            }
            pl("    int index = KNI_GetParameterAsInt(1);");
            pl("    KNI_StartHandles(1);");
            pl("    KNI_DeclareHandle(string);");
            pl("    if (index >= 0 && index <= max_index) {");
            pl("        int offset = (int)offset_data[index * 2 + 0];");
            pl("        int length = (int)offset_data[index * 2 + 1];");
            pl("        if (offset > 0) {");
            if (type == UNICODE) {
                plx("KNI_NewString(string_data+offset, length, string);");
            } else {
                plx("const char *p = string_data+offset;");
                plx("if (length < 128) {");
                plx("    utf8 = stackbuffer;");
                plx("} else {");
                plx("    utf8 = (char*)midpMalloc(length+1);");
                plx("}");
                plx("if (utf8) {");
                plx("    memcpy(utf8, p, length);");
                plx("    utf8[length] = 0;");
                plx("    KNI_NewStringUTF(utf8, string);");
                plx("}");
                plx("if (utf8 && utf8 != stackbuffer) {");
                plx("    midpFree(utf8);");
                plx("}");
            }
            pl("        }");
            pl("    }");
            pl("    KNI_EndHandlesAndReturnObject(string)");
            pl("}");
        } catch (Throwable t) {
            t.printStackTrace();
        }

        writer.close();
    }

    /**
     * Short-hand for printint a line into the output file
      * @param s the string to output
    */
    void pl(String s) {
        writer.println(s);
    }

    /**
     * Short-hand for printint a string  the output file (with a 12-space
     * prefix)
     * @param s the string to output
     */
    void plx(String s) {
        writer.print("            ");
        writer.println(s);
    }

    /**
     * Short-hand for printint a string into the output file
     * @param s the string to output
     */
    void p(String s) {
        writer.print(s);
    }

    /** Indicate that a table should be stored as an UTF8 array in C code. */
    public final int UTF8    = 1;

    /** Indicate that a table should be stored as an UNICODE array in C code.*/
    public final int UNICODE = 2;

    /**
     * Write the string data using UTF8 or UNICODE, depending on which
     * is smaller.
     * @return the storage type
     */
    int writeOptimizedArray() throws Throwable {
        // (1) Sort all strings and then merge them into a single String.
        // This allows shorter strings that are substrings of longer strings
        // to be omitted.
        ArrayList list = sort();
        StringBuffer sbuf = new StringBuffer();
        for (int i=0; i<list.size(); i++) {
            LocalizedString r = (LocalizedString)list.get(i);
            if (r != null) {
                int n = sbuf.indexOf(r.string);
                if (n >= 0) {
                    r.offset = n;
                } else {
                    r.offset = sbuf.length();
                    sbuf.append(r.string);
                }
                r.length = r.string.length();
            }
        }
        int type = getStorageType(sbuf);
        switch (type) {
        case UTF8:
            writeUTF8(sbuf, list);
            break;
        case UNICODE:
            writeUNICODE(sbuf);
            break;
        }

        writeOffsetTable();

        return type;
    }

    /**
     * Tell which one of UT8 or UNICODE is smaller
     * @param sbuf the string data to be processed
     * @return the storage type
     */
    int getStorageType(StringBuffer sbuf) throws Throwable {
        boolean hasZero = false;
        for (int i=0; i<sbuf.length(); i++) {
            char c = sbuf.charAt(i);
            if (c == 0x0) {
                hasZero = true;
            }
        }

        if (hasZero) {
            // The VM's handling of 0 is different than specified in
            // the "official" UTF8 encoding, so let's not do it.
        } else {
            String s = sbuf.toString();
            byte bytes[] = s.getBytes("UTF-8");
            if (bytes.length < sbuf.length() * 2) {
                return UTF8;
            }
        }

        return UNICODE;
    }

    /** Hex digits for output. */
    static char digits[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    /**
     * Prints a hex number for the given character.
     * @param c the character to convert
     * @param numdigits the output field width
     */
    void printHex(char c, int numdigits) {
        p("0x");
        for (int i=numdigits-1; i>= 0; i--) {
            int shift = i * 4;
            int n = ((((int)c) >> shift)) & 0x0f;
            writer.print(Character.toString(digits[n]));
        }
    }

    /**
     * Write all the data in a UNICODE array
     * @param sbuf the string data to be processed
     */
    void writeUNICODE(StringBuffer sbuf) {
        int STEP = 10;
        int MAX = sbuf.length();
        pl("static const jchar string_data[] = {");

        for (int i=0; i<MAX; i ++) {
            if ((i % STEP) == 0) {
                pl("");
                p("    ");
            }
            char c = sbuf.charAt(i);
            printHex(c, 4);
            p(",");
        }
        pl("};");
    }

    /**
     * Write all the data in a UTF8 array
     * @param sbuf the string data to be processed
     * @param list the data to be processed
     */
    void writeUTF8(StringBuffer sbuf, ArrayList list) throws Throwable {
        // Recompute the offset and length of each item
        byte array[] = new byte[sbuf.length() * 2]; // MAX possible size
        int end = 0;

        for (int i=0; i<list.size(); i++) {
            LocalizedString r = (LocalizedString)list.get(i);
            if (r != null) {
                byte bytes[] = r.string.getBytes("UTF-8");
                int n = search(bytes, array, end);
                if (n >= 0) {
                    r.offset = n;
                } else {
                    r.offset = end;
                    for (int x=0; x<bytes.length; x++) {
                        array[end++] = bytes[x];
                    }
                }
                r.length = bytes.length;
            }
        }

        // Now write the strings to C source code.
        int STEP = 10;
        int MAX = end;

        p("static const char string_data[] = {");
        for (int i=0; i<MAX; i ++) {
            if ((i % STEP) == 0) {
                pl("");
                p("    ");
            }
            char c = (char)array[i];
            printHex(c, 2);
            p(",");
        }
        pl("};");
    }

    /**
     * Equivalent of String.indexOf(), but works on a UTF8 byte array.
     * @param needle the item to be found
     * @param haystack the data to be searched
     * @param end the limit of the search
     * @return theindex of the found data or -1 if not found
     */
    int search(byte needle[], byte haystack[], int end) {
        int needleLen = needle.length;
        if (needleLen == 0) {
            return -1;
        }
        end -= needleLen;
        byte b = needle[0];

        for (int i=0; i<end; i++) {
            inner: {
                if (haystack[i] == b) {
                    for (int j=1; j<needleLen; j++) {
                        if (haystack[i+j] != needle[j]) {
                            break inner;
                        }
                    }
                    return i;
                }
            }
        }

        return -1;
    }

    /**
     * Write a table of the offset of each localized string
     */
    void writeOffsetTable() {
        int maxOffset = 0, maxLen = 0;

        for (int i=0; i<strings.length; i++) {
            LocalizedString r = strings[i];
            if (r == null) {
                System.err.println("Warning: resource index " + i +
                                   " not defined in class " +
                                   locale.className);
            } else {
                if (maxOffset < r.offset) {
                    maxOffset = r.offset;
                }
                if (maxLen < r.length) {
                    maxLen = r.length;
                }
            }
        }
        String type = "int";
        if (maxOffset < 0x7fff && maxLen < 0x7fff) {
            type = "short";
        }
        p("static const " + type + " offset_data[] = {");

        int STEP = 4;
        for (int i=0; i<strings.length; i++) {
            if ((i % STEP) == 0) {
                pl("");
                p("    ");
            }
            LocalizedString r = strings[i];
            if (r == null) {
                p("-1, -1,");
            } else {
                p(r.offset + ", " + r.length + ", ");
            }
        }
        pl("};");

        pl("static int max_index = " + strings.length + ";");
    }
}

