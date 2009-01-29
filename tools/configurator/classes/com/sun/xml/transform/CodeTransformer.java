/*
 *
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
 * Transforms an XML file according to given XSL file.
 */
package com.sun.xml.transform;

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
     * <tt>transformations</tt> vector for each transformation specified.
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
                // i.e all agruments coming after it belong
                // to next transformation(s)
                transformations.add(tr);
                tr = new Transformation();
            } else if (arg.equals("-debug")) {
                debug = true;
            } else {
                printHelp = true;
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
         * doing multiple transformations per invocation. -out option marks
         * the end of arguments for single transformation. All arguments
         * after it belong to next transformation(s).
         */
        System.err.println("Usage: java -jar <code_transformer_jar_file>"
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

        // create Transformer that will apply stylesheet
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
}
