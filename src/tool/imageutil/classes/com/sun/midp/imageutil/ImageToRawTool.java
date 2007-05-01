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
 * This tool takes list of input images and XML description of raw image
 * format and then converts the input images into raw format images stored
 * in the specified output directory.
 *
 * It understands following command line arguments:
 * -format <filename> : XML file with raw image format description
 * -out <dirname>     : output folder to place converted images to
 * <filename>+        : filenames of input images
 *
 */

package com.sun.midp.imageutil;

import java.io.*;

import javax.xml.parsers.*;

import org.w3c.dom.*;

import java.awt.image.*;
import java.awt.*;
import java.util.Vector;

/**
 * Represents image to raw conversion job
 */
class ImageToRawJob {
    /** XML file name with raw image format */
    public String formatXMLFileName = "";

    /** Input images file list */
    public Vector inImagesFiles = new Vector();

    /** Output images directory name */
    public String outImagesDirName = "";
}

/**
 * Main tool class
 */
public class ImageToRawTool {
    /**
     * for converting image into raw format
     */
    com.sun.midp.imageutil.ImageToRawConverter converter;

    /**
     * Romization job to perform
     */
    private static ImageToRawJob imageToRawJob = null;

    /**
     * Print debug output while running
     */
    private static boolean debug = false;

    /**
     * Print usage info and exit
     */
    private static boolean printHelp = false;

    /**
     * raw image file format
     */
    int rawFormat = ImageToRawConverter.FORMAT_INVALID;

    /**
     * raw image color format
     */
    int colorFormat = ImageToRawConverter.FORMAT_INVALID;

    /**
     * raw image data endianess
     */
    int endianFormat = ImageToRawConverter.FORMAT_INVALID;

    /**
     * array of strings used in XML file to specify raw image format
     */
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

    /**
     * array of constants corresponding to strings above
     */
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
            ImageToRawTool tool = new ImageToRawTool();
            tool.convert(imageToRawJob);

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
    private static void parseArgs(String[] args) {
        imageToRawJob = new ImageToRawJob();

        if (args.length < 1) printHelp = true;
        for (int i = 0; i < args.length; ++i) {
            String arg = args[i];
            if (arg.equals("-format")) {
                imageToRawJob.formatXMLFileName = args[++i];
            } else if (arg.equals("-out")) {
                imageToRawJob.outImagesDirName = args[++i];
            } else if (arg.equals("-debug")) {
                debug = true;
            } else if (arg.equals("-help")) {
                printHelp = true;
            } else {
                imageToRawJob.inImagesFiles.add(arg);
            }
        }
    }

    /**
     * Creates a directory structure.
     *
     * @param fullDirName Full directory path to be created.
     * @throws IOException is thrown if directory couldn't be created
     */
    private static void makeDirectoryTree(String fullDirName)
            throws IOException {

        if (debug) {
            System.out.println("mkdir: " + fullDirName);
        }
    }

    /**
     * Validates parsed arguments, printing error message
     * if some arg is invalid.
     *
     * @return true, if all arguments are valid
     */
    private static boolean validateParsedArgs() {
        // XML file with raw image format description
        File f = new File(imageToRawJob.formatXMLFileName);
        if (!f.isFile()) {
            System.err.println("Couldn't find raw image format XML file: "
                    + '"' + imageToRawJob.formatXMLFileName + '"');

            return false;
        }
        // input images file list
        if (imageToRawJob.inImagesFiles.isEmpty()) {
            System.err.println("No input images are specified");
            return false;
        }

        // output images directory
        try {
            File outDir = new File(imageToRawJob.outImagesDirName);
            if (!(outDir).exists()) {
                if (!(outDir).mkdirs()) {
                    System.err.println("Failed to create output directory: "
                            + '"' + imageToRawJob.outImagesDirName + '"');

                    return false;
                }
            }
            imageToRawJob.outImagesDirName = outDir.getCanonicalPath();
        } catch (IOException ioe) {
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
         * -format:     XML file describing raw image format.
         * -out:        Output images folder.
         * -help:       Print usage information
         * -debug:      Be verbose: print some debug info while running.
         */
        System.err.println("\n"
                + "  ImageToRawTool arguments:\n\n"
                + "    [-help] \n"
                + "    [-debug] \n"
                + "    -format <formatXMLFile> \n"
                + "    -out <outputImagesDirectory> \n"
                + "    <imageToConvert>+");
    }

    /**
     * Does the actual romization
     *
     * @param imageToRawJob romization to perform
     * @throws Exception if there was an error during romization
     */
    public void convert(ImageToRawJob imageToRawJob)
            throws Exception {

        // load XML file as DOM tree
        DocumentBuilderFactory domFactory =
                DocumentBuilderFactory.newInstance();

        // do not validate input XML file,
        // we assume it has been validated before
        domFactory.setValidating(false);

        DocumentBuilder domBuilder = domFactory.newDocumentBuilder();
        Document domDoc = domBuilder.parse(
                new File(imageToRawJob.formatXMLFileName));
        if (!searchImageFormat(domDoc.getDocumentElement())) {
            throw new Exception(
                "XML document contains no raw image format description");
        };
        converter = new ImageToRawConverter(
                rawFormat, colorFormat, endianFormat);

        // convert and save images
        Vector images = imageToRawJob.inImagesFiles;
        for (int i = 0;  i < images.size(); i++) {
            String inputFile = (String)images.elementAt(i);
            String fileName = new File(inputFile).getName();
            if (fileName.lastIndexOf('.') > 0) {
                fileName = fileName.replaceFirst("\\.[^.]*?$", ".raw");
            } else fileName += ".raw";
            String outputFile = imageToRawJob.outImagesDirName +
                    File.separator + fileName;
            convertImageToRaw(inputFile, outputFile);
        }
    }

    /**
     * Utility method. Gets BuffredImage data as in array.
     *
     * @param image BufferedImage to return data for
     * @return image data as byte array
     */
    private int[] getBufferedImageData(BufferedImage image) {
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
        return ((DataBufferInt) srcBuf).getData();
    }


    private void convertImageToRaw(String sourceName, String destName)
            throws Exception {

        // load image
        BufferedImage image =
                javax.imageio.ImageIO.read(new File(sourceName));
        int width = image.getWidth(null);
        int height = image.getHeight(null);
        boolean hasAlpha = image.getColorModel().hasAlpha();
        int[] imageData = getBufferedImageData(image);

        // convert image
        byte[] rawData = converter.convertToRaw(
                imageData, width, height, hasAlpha);

        // save image
        if (new File(destName).exists())
            System.out.println("Overwrite raw image: " + destName);
        else System.out.println("Produce raw image: " + destName);

        FileOutputStream writer = new FileOutputStream(destName);
        writer.write(rawData);
        writer.close();
    }

    /**
     * Walks the XML tree and search for raw image format node
     *
     * @param n current DOM node in document
     */
    private boolean searchImageFormat(Node n) throws Exception {

        if (n.getNodeName().equals("rawimage") &&
                (n instanceof Element)) {
            readRawImageFormat(n);
            return true;
        } else {
            NodeList list = n.getChildNodes();
            for (int i = 0; i < list.getLength(); i++) {
                if (searchImageFormat(list.item(i)))
                    return true;
            }
        }
        // Image format description is not found
        return false;
    }

    /**
     * Collects raw image format specification
     *
     * @param n current DOM node in document
     * @throws Exception if there was an error during romization
     */
    private void readRawImageFormat(Node n) throws Exception {
        Element e = (Element) n;

        String rawFormatStr = e.getAttribute("Format");
        rawFormat = ImageToRawConverter.FORMAT_INVALID;
        for (int i = 0; i < imageFormatStrings.length; ++i) {
            if (imageFormatStrings[i].equals(rawFormatStr)) {
                rawFormat = imageFormats[i];
                break;
            }
        }
        if (rawFormat == ImageToRawConverter.FORMAT_INVALID) {
            throw new IllegalArgumentException("Invalid raw file format "
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
            throw new IllegalArgumentException("Invalid color format "
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
            throw new IllegalArgumentException("Invalid color format "
                    + '"' + endianFormatStr + '"');
        }

        if (!ImageToRawConverter.isFormatSupported(rawFormat, colorFormat))
        {
            throw new IllegalArgumentException(
                    "Unsupported row image type: "
                            + " format " + '"' + rawFormat + '"'
                            + ", color " + '"' + colorFormat + '"');
        }
    }

}
