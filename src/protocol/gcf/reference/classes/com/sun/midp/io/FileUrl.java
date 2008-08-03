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
 */

package com.sun.midp.io;

/**
 * Parse path to file storage.
 * Can work like encoder: searching unsafe characters and
 * replace its on its US-ASCII hexadecimal representation.
 * Also doing back work: decodes encoded file path.
 * Based on RFC 1738. 
 * <p>
 * Checks file path for encoding unsafe characters 
 * and vice versa. 
 */
public class FileUrl {
    
    /** Prevents anyone from instantiating this class */
    private FileUrl() {
        
    }
        
    /**
     * Decode file path: if it contains US-ASCII symbols,
     * than reverse its on its literal representation.
     * 
     * @param filenamepath  path to filename
     * @return  decoded string of file path
     */
    public static String decodeFilePath(String filenamepath) {
        char[] fileChars  = new char[filenamepath.length()];
        StringBuffer buffer = new StringBuffer();
        char temp;
        char[] asciiChars = new char[3];
        
        filenamepath.getChars(0,filenamepath.length(),fileChars,0);
        
            for(int i = 0; i < fileChars.length ; i++) {
                if(fileChars[i]=='%') {
                    filenamepath.getChars(i,i+3,asciiChars,0);                    
                    temp = decodeASCIISymbol(new String(asciiChars));
                    if (temp != '0') {
                        buffer.append(temp);
                        i=i+2;   
                    } else
                        buffer.append(fileChars[i]);
                } else
                    buffer.append(fileChars[i]);
             }
        return buffer.toString();
    }
     
    /**
     * Encode file path: if it contains unsafe symbols,
     * according to RFC 1738.
     *  
     * @param filenamepath  path to filename
     * @return  encoded file path
     */
    public static String encodeFilePath(String filenamepath) {
        char[] fileChars = new char[filenamepath.length()];
        StringBuffer buffer = new StringBuffer();
        String temp;
        
        filenamepath.getChars(0,filenamepath.length(),fileChars,0);
        
            for (int i = 0; i < fileChars.length; i++)
            {
              temp = encodeUnsafeCharacter(fileChars[i]);
              
              if (temp == null) 
                  buffer.append(fileChars[i]);
              else
                  buffer.append(temp);
            }
        
       return buffer.toString();
    }
    
    /**
     * Encodes character to it US-ASCII code.
     * 
     * @param character
     * @return the code of unsafe character
     * or null if character is reserved.
     */
    private static String encodeUnsafeCharacter(char character) {
        switch(character) {
            case ' '  : return new String("%20");
            case '<'  : return new String("%3C");
            case '>'  : return new String("%3E");           
            case '{'  : return new String("%7B");
            case '}'  : return new String("%7D");
            case '|'  : return new String("%7C");
            case '\\' : return new String("%5C");
            case '^'  : return new String("%5E");
            case '~'  : return new String("%7E");
            case '['  : return new String("%5B");
            case ']'  : return new String("%5D");
            case '`'  : return new String("%60");
            case '%'  : return new String("%26");
            default   : return null;
        }
    }
    
    /**
     * Decodes ASCII code string to it
     * literal representation.
     * 
     * @param asciiCode string with ASCII code
     * @return specific character or '0', if 
     * we don`t need to decode this string
     */
    private static char decodeASCIISymbol(String asciiCode) {
        if (asciiCode.equals("%20"))
            return ' ';
        else if (asciiCode.equals("%3C"))
            return '<';
        else if (asciiCode.equals("%3E"))
            return '>';
        else if (asciiCode.equals("%7B"))
            return '{';
        else if (asciiCode.equals("%7D"))
            return '}';
        else if (asciiCode.equals("%7C"))
            return '|';
        else if (asciiCode.equals("%5C"))
            return '\\';
        else if (asciiCode.equals("%5E"))
            return '^';
        else if (asciiCode.equals("%7E"))
            return '~';
        else if (asciiCode.equals("%5B"))
            return '[';
        else if (asciiCode.equals("%5D"))
            return ']';
        else if (asciiCode.equals("%60"))
            return '`';
        else if (asciiCode.equals("%26"))
            return '%';
        else
            return '0';
        
    }         
            
}
