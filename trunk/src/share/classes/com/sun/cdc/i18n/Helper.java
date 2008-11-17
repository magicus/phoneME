/*
 * @(#)Helper.java	1.21 06/10/10
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

package com.sun.cdc.i18n;

import java.io.*;

/**
 * This class general helper functions for the J2SE environment.
 * <p>
 * <em>No application code should reference this class directly.</em>
 *
 * @version 1.0 12/15/99
 */
public class Helper {

    /**
     * The name of the default character encoding
     */
    private static String defaultEncoding;

    /**
     * Default path to the J2ME classes
     */
    private static String defaultMEPath;

    /**
     * True if we are running on a J2ME system
     */
    private static boolean j2me = false;

    /**
     * Class initializer
     */
    static {
        /* Get the default encoding name */
        defaultEncoding = System.getProperty("microedition.encodingClass");
        if(defaultEncoding == null) {
            defaultEncoding = "ISO_LATIN_1";
        }

        /* Work out if we are running on a J2ME system */
        if(System.getProperty("microedition.configuration") != null) {
            j2me = true;
        } else {
            defaultEncoding = null;
        }

        /* Get the default encoding name */
        defaultMEPath = System.getProperty("microedition.i18npath");
        if(defaultMEPath == null) {
            defaultMEPath = "com.sun.cdc.i18n.j2me";
        }

//System.out.println("j2me="+j2me);
//System.out.println("defaultEncoding="+defaultEncoding);
    }

/*------------------------------------------------------------------------------*/
/*                               Character encoding                             */
/*------------------------------------------------------------------------------*/


    /**
     * Get a reader for an InputStream
     *
     * @param  is              The input stream the reader is for
     * @return                 A new reader for the stream
     */
    public static Reader getStreamReader(InputStream is) {
        try {
            return getStreamReader(is, defaultEncoding);
        } catch(UnsupportedEncodingException x) {
            throw new RuntimeException("Missing default encoding "+defaultEncoding);
        }
    }

    /**
     * Get a reader for an InputStream
     *
     * @param  is              The input stream the reader is for
     * @param  name            The name of the decoder
     * @return                 A new reader for the stream
     * @exception UnsupportedEncodingException  If the encoding is not known
     */
    public static Reader getStreamReader(InputStream is, String name) throws UnsupportedEncodingException {

        /* Test for null arguments */
        if(is == null || name == null) {
            throw new NullPointerException();
        }

        /* Get the reader from the encoding */
        StreamReader fr = getStreamReaderPrim(name);

        /* Open the connection and return*/
        return fr.open(is, name);
    }


    /**
     * Get a reader for an InputStream
     *
     * @param  is              The input stream the reader is for
     * @param  name            The name of the decoder
     * @return                 A new reader for the stream
     * @exception UnsupportedEncodingException  If the encoding is not known
     */
    private static StreamReader getStreamReaderPrim(String name) throws UnsupportedEncodingException {

        if(name == null) {
            throw new NullPointerException();
        }

        try {
             String className;

             /* Get the reader class name */
             if(j2me) {
                 className = defaultMEPath + '.' + name + "_Reader";
             } else {
                 className = "com.sun.cdc.i18n.j2me.Default_Reader";
             }

            /* Using the decoder names lookup a class to implement the reader */
            Class clazz = Class.forName(className);

            /* Return a new instance */
            return (StreamReader)clazz.newInstance();

        } catch(ClassNotFoundException x) {
            throw new UnsupportedEncodingException("Encoding "+name+" not found");
        } catch(InstantiationException x) {
            throw new RuntimeException("InstantiationException "+x.getMessage());
        } catch(IllegalAccessException x) {
            throw new RuntimeException("IllegalAccessException "+x.getMessage());
        } catch(ClassCastException x) {
            throw new RuntimeException("ClassCastException "+x.getMessage());
        }
    }


    /**
     * Get a writer for an OutputStream
     *
     * @param  os              The output stream the reader is for
     * @return                 A new writer for the stream
     */
    public static Writer getStreamWriter(OutputStream os) {
        try {
            return getStreamWriter(os, defaultEncoding);
        } catch(UnsupportedEncodingException x) {
            throw new RuntimeException("Missing default encoding "+defaultEncoding);
        }
    }


    /**
     * Get a writer for an OutputStream
     *
     * @param  os              The output stream the reader is for
     * @param  name            The name of the decoder
     * @return                 A new writer for the stream
     * @exception UnsupportedEncodingException  If the encoding is not known
     */
    public static Writer getStreamWriter(OutputStream os, String name) throws UnsupportedEncodingException {

        /* Test for null arguments */
        if(os == null || name == null) {
            throw new NullPointerException();
        }

        /* Get the writer from the encoding */
        StreamWriter sw = getStreamWriterPrim(name);

        /* Open it on the output stream and return */
        return sw.open(os, name);
    }

    /**
     * Get a writer for an OutputStream
     *
     * @param  os              The output stream the reader is for
     * @param  name            The name of the decoder
     * @return                 A new writer for the stream
     * @exception UnsupportedEncodingException  If the encoding is not known
     */
    private static StreamWriter getStreamWriterPrim(String name) throws UnsupportedEncodingException {

        if(name == null) {
            throw new NullPointerException();
        }

        try {
             String className;

             /* Get the writer class name */
             if(j2me) {
                 className = defaultMEPath + '.' + name +"_Writer";
             } else {
                 className = "com.sun.cdc.i18n.j2me.Default_Writer";
             }

            /* Using the decoder names lookup a class to implement the writer */
            Class clazz = Class.forName(className);

            /* Construct a new instance */
            return (StreamWriter)clazz.newInstance();

        } catch(ClassNotFoundException x) {
            throw new UnsupportedEncodingException("Encoding "+name+" not found");
        } catch(InstantiationException x) {
            throw new RuntimeException("InstantiationException "+x.getMessage());
        } catch(IllegalAccessException x) {
            throw new RuntimeException("IllegalAccessException "+x.getMessage());
        } catch(ClassCastException x) {
            throw new RuntimeException("ClassCastException "+x.getMessage());
        }
    }


    /**
     * Convert a byte array to a char array
     *
     * @param  buffer          The byte array buffer
     * @param  offset          The offset
     * @param  length          The length
     * @return                 A new char array
     */
    public static char[] byteToCharArray(byte[] buffer, int offset, int length) {
        try {
            return byteToCharArray(buffer, offset, length, defaultEncoding);
        } catch(UnsupportedEncodingException x) {
            throw new RuntimeException("Missing default encoding "+defaultEncoding);
        }
    }

    /**
     * Convert a char array to a byte array
     *
     * @param  buffer          The char array buffer
     * @param  offset          The offset
     * @param  length          The length
     * @return                 A new byte array
     */
    public static byte[] charToByteArray(char[] buffer, int offset, int length) {
        try {
            return charToByteArray(buffer, offset, length, defaultEncoding);
        } catch(UnsupportedEncodingException x) {
            throw new RuntimeException("Missing default encoding "+defaultEncoding);
        }
    }

    /*
     * Cached variables for byteToCharArray
     */
    private static String lastReaderEncoding;
    private static StreamReader  lastReader;

    /**
     * Convert a byte array to a char array
     *
     * @param  buffer          The byte array buffer
     * @param  offset          The offset
     * @param  length          The length
     * @param  enc             The character encoding
     * @return                 A new char array
     * @exception UnsupportedEncodingException  If the encoding is not known
     */
    public static synchronized char[] byteToCharArray(byte[] buffer, int offset, int length, String enc) throws UnsupportedEncodingException {

        /* If we don't have a cached reader then make one */
        if(lastReaderEncoding == null || !lastReaderEncoding.equals(enc)) {
            lastReader = getStreamReaderPrim(enc);
            lastReaderEncoding = enc;
        }

        /* Ask the reader for the size the output will be */
        int size = lastReader.sizeOf(buffer, offset, length);

        /* Allocate a buffer of that size */
        char[] outbuf = new char[size];

        /* Open the reader on a ByteArrayInputStream */
        lastReader.open(new ByteArrayInputStream(buffer, offset, length), enc);

        try {
            /* Read the input */
            lastReader.read(outbuf, 0, size);
            /* Close the reader */
            lastReader.close();
        } catch(IOException x) {
            throw new RuntimeException("IOException reading reader "+x.getMessage());
        }

        /* And return the buffer */
        return outbuf;
    }


    /*
     * Cached variables for charToByteArray
     */
    private static String lastWriterEncoding;
    private static StreamWriter lastWriter;

    /**
     * Convert a char array to a byte array
     *
     * @param  buffer          The char array buffer
     * @param  offset          The offset
     * @param  length          The length
     * @param  enc             The character encoding
     * @return                 A new byte array
     * @exception UnsupportedEncodingException  If the encoding is not known
     */
    public static synchronized byte[] charToByteArray(char[] buffer, int offset, int length, String enc) throws UnsupportedEncodingException {

        /* If we don't have a cached writer then make one */
        if(lastWriterEncoding == null || !lastWriterEncoding.equals(enc)) {
            lastWriter = getStreamWriterPrim(enc);
            lastWriterEncoding = enc;
        }

        /* Ask the writeer for the size the output will be */
        int size = lastWriter.sizeOf(buffer, offset, length);

        /* Get the output stream */
        ByteArrayOutputStream os = new ByteArrayOutputStream(size);

        /* Open the writer */
        lastWriter.open(os, enc);

        try {
            /* Convert */
            lastWriter.write(buffer, offset, length);
            /* Close the writer */
            lastWriter.close();
        } catch(IOException x) {
            throw new RuntimeException("IOException writing writer "+x.getMessage());
        }

        /* Close the output stream */
        try {
            os.close();
        } catch(IOException x) {};

        /* Return the array */
        return os.toByteArray();
    }

}

