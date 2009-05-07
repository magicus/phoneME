/*
 * @(#)CharToByteASCII.java	1.16 06/10/10
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

package sun.io;

public class CharToByteASCII extends CharToByteConverter {

    // Return the character set ID
    public String getCharacterEncoding()
    {
        return "ASCII";
    }

    private char highHalfZoneCode;
    
    public int flush(byte[] output, int outStart, int outEnd)
	throws MalformedInputException
    {
	if (highHalfZoneCode != 0) {
	    highHalfZoneCode = 0;
	    throw new MalformedInputException
		("String ends with <High Half Zone code> of UTF16");
	}
	byteOff = charOff = 0;
	return 0;
    }

    /*
    * Character conversion
    */
    public int convert(char[] input, int inOff, int inEnd,
		       byte[] output, int outOff, int outEnd)
        throws MalformedInputException,
               UnknownCharacterException,
               ConversionBufferFullException
    {
        char    inputChar;          // Input character to be converted
	int	outputSize;	    // Size of output	

        // Record beginning offsets
        charOff = inOff;
        byteOff = outOff;

	if (highHalfZoneCode != 0) {
	    inputChar = highHalfZoneCode;
	    highHalfZoneCode = 0;
	    if (input[inOff] >= 0xdc00 && input[inOff] <= 0xdfff) {
		// This is legal UTF16 sequence.
		badInputLength = 1;
		throw new UnknownCharacterException();
	    } else {
		// This is illegal UTF16 sequence.
		badInputLength = 0;
		throw new MalformedInputException
		    ("Previous converted string ends with " +
		     "<High Half Zone Code> of UTF16 " +
		     ", but this string is not begin with <Low Half Zone>");
	    }
	}

        // Loop until we hit the end of the input
        while(charOff < inEnd) {
            // Get the input character
            inputChar = input[charOff];

            // Is this character mappable?
            if (inputChar <= '\u007F') {
                if (byteOff + 1 > outEnd)
                    throw new ConversionBufferFullException();

                output[byteOff++] = (byte)inputChar;
                charOff += 1;

            }
            // Is this a high surrogate?
            else if(inputChar >= '\uD800' && inputChar <= '\uDBFF') {
                // Is this the last character in the input?
                if (charOff + 1 == inEnd) {
		    highHalfZoneCode = inputChar;
		    break;
                }

                // Is there a low surrogate following?
                inputChar = input[charOff + 1];
                if (inputChar >= '\uDC00' && inputChar <= '\uDFFF') {
                    // We have a valid surrogate pair.  Too bad we don't map
                    //  surrogates.  Is substitution enabled?
                    if (subMode) {
			outputSize = subBytes.length;

                        if (byteOff + outputSize > outEnd)
                            throw new ConversionBufferFullException();

                        System.arraycopy(subBytes, 0, output, byteOff, outputSize);
                        byteOff += outputSize;
                        charOff += 2;

                    } else {
                        badInputLength = 2;
                        throw new UnknownCharacterException();
                    }
                } else {
                    // We have a malformed surrogate pair
                    badInputLength = 1;
                    throw new MalformedInputException();
                }
            }
            // Is this an unaccompanied low surrogate?
            else if (inputChar >= '\uDC00' && inputChar <= '\uDFFF') {
                badInputLength = 1;
                throw new MalformedInputException();
            }
            // Unmappable and not part of a surrogate
            else {
                // Is substitution enabled?
                if (subMode) {
	            outputSize = subBytes.length;

                    if (byteOff + outputSize > outEnd)
                        throw new ConversionBufferFullException();

                    System.arraycopy(subBytes, 0, output, byteOff, outputSize);
                    byteOff += outputSize;
                    charOff += 1;

	        } else {
                    badInputLength = 1;
                    throw new UnknownCharacterException();
                }
            }

        }

        // Return the length written to the output buffer
        return byteOff-outOff;
    }

    // Determine if a character is mappable or not
    public boolean canConvert(char ch)
    {
        return (ch <= '\u007F');
    }

    // Reset the converter
    public void reset()
    {
	byteOff = charOff = 0;
	highHalfZoneCode = 0;
    }

    /**
     * returns the maximum number of bytes needed to convert a char
     */
    public int getMaxBytesPerChar()
    {
        return 1;
    }
}
